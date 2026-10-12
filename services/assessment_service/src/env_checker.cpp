/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "env_checker.h"

#include <cinttypes>
#include <list>

#include "assessment_fail_reason.h"
#include "assessment_utils.h"
#include "call_manager_client.h"
#include "display_manager.h"
#include "enterprise_device_mgr_proxy.h"
#include "hilog_tag_wrapper.h"
#include "screen_capture_monitor.h"
#include "screen_info.h"
#include "screen_manager.h"
#include "singleton.h"
#include "system_ability_definition.h"
#include "want.h"

namespace OHOS {
namespace AAFwk {

const std::string EXTENSION_SO_PATH = "libassessment_configuration_service_ext.z.so";

EnvChecker::EnvChecker()
{
    loader_ = std::make_unique<ExtensionLoader>(EXTENSION_SO_PATH);
}

bool EnvChecker::Init()
{
    if (!loader_->InitExtensionLoader()) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Extension loader degraded, VM check will pass by default");
    }
    // CallManager may be absent on devices without telephony; IsInCall() fails
    // open on its own, so an incomplete init must not block the caller.
    return InitCallManager();
}

bool EnvChecker::CheckAll(std::string &failReason)
{
    // Recording is checked before casting: an active capture also makes IsCaptured()
    // true, and reporting it as recording is the more precise diagnosis. A paused
    // recording is visible to the monitor only, so this order catches both cases.
    if (IsScreenRecording()) {
        failReason = FAIL_REASON_SCREEN_RECORDING;
        TAG_LOGE(AAFwkTag::DEFAULT, "Screen recording detected");
        return false;
    }

    if (IsScreenCasting()) {
        failReason = FAIL_REASON_SCREEN_CASTING;
        TAG_LOGE(AAFwkTag::DEFAULT, "Screen casting detected");
        return false;
    }

    if (IsMultiScreen()) {
        failReason = FAIL_REASON_MULTI_SCREEN;
        TAG_LOGE(AAFwkTag::DEFAULT, "Multi-screen detected");
        return false;
    }

    if (IsInCall()) {
        failReason = FAIL_REASON_IN_CALL;
        TAG_LOGE(AAFwkTag::DEFAULT, "In call detected");
        return false;
    }

    if (IsPcDevice()) {
        if (IsVirtualMachine(failReason)) {
            TAG_LOGE(AAFwkTag::DEFAULT, "Virtual machine running detected");
            return false;
        }
    }

    if (IsEdmAdminPresent()) {
        failReason = FAIL_REASON_EDM_ADMIN;
        TAG_LOGE(AAFwkTag::DEFAULT, "EDM admin detected, device is managed");
        return false;
    }

    return true;
}

bool EnvChecker::IsAwakeAnco(std::string ancoState)
{
    return loader_->InvokeIsAwakeAnco(ancoState);
}

int32_t EnvChecker::RestrictAncoApp()
{
    return loader_->InvokeRestrictAncoApp();
}

bool EnvChecker::IsScreenRecording()
{
    auto *monitor = Media::ScreenCaptureMonitor::GetInstance();
    if (monitor == nullptr) {
        // Only reachable on a build compiled with UNSUPPORT_SCREEN_CAPTURE, where the
        // platform offers no screen capture at all, so there is nothing to detect.
        TAG_LOGW(AAFwkTag::DEFAULT, "ScreenCaptureMonitor unavailable, skip recording check");
        return false;
    }
    // An empty list means both "no process holds a capture" and "the monitor service
    // could not be reached"; this interface has no error channel to tell them apart.
    // Both are treated as no recording, since a device whose monitor is unavailable
    // has no screen capture to report either. When the monitor is unreachable the
    // player_framework side logs "screen capture monitor service does not exist."
    std::list<int32_t> capturePids = monitor->IsScreenCaptureWorking();
    if (capturePids.empty()) {
        return false;
    }
    TAG_LOGE(AAFwkTag::DEFAULT, "Screen recording detected, capturing process count: %{public}zu",
        capturePids.size());
    return true;
}

bool EnvChecker::IsScreenCasting()
{
    bool isCaptured = Rosen::DisplayManager::GetInstance().IsCaptured();
    if (isCaptured) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Screen casting detected, isCaptured = true");
        return true;
    }
    return false;
}

bool EnvChecker::IsMultiScreen()
{
    std::vector<sptr<Rosen::Screen>> screens;
    Rosen::DMError ret = Rosen::ScreenManager::GetInstance().GetAllScreens(screens);
    if (ret != Rosen::DMError::DM_OK) {
        TAG_LOGW(AAFwkTag::DEFAULT, "GetAllScreens failed, ret: %{public}d", static_cast<int32_t>(ret));
        return false;
    }
    // Only check screen type when more than one screen is present.
    // Some devices have no built-in screen; a single external screen is allowed.
    if (screens.size() <= 1) {
        return false;
    }
    for (const auto &screen : screens) {
        if (screen == nullptr) {
            continue;
        }
        sptr<Rosen::ScreenInfo> screenInfo = screen->GetScreenInfo();
        if (screenInfo == nullptr) {
            // Cannot confirm the screen is built-in: treat it as an anomaly.
            TAG_LOGE(AAFwkTag::DEFAULT, "GetScreenInfo null, screenId: %{public}" PRIu64, screen->GetId());
            return true;
        }
        Rosen::ScreenTypeInfo screenType = screenInfo->GetScreenTypeInfo();
        if (screenType != Rosen::ScreenTypeInfo::BUILT_IN) {
            TAG_LOGE(AAFwkTag::DEFAULT, "Multi screen check failed, non built-in screen detected,"
                " screenId: %{public}" PRIu64 ", screenType: %{public}d",
                screen->GetId(), static_cast<int32_t>(screenType));
            return true;
        }
    }
    return false;
}

bool EnvChecker::InitCallManager()
{
    if (callManagerInited_) {
        return true;
    }
    auto callClient = DelayedSingleton<Telephony::CallManagerClient>::GetInstance();
    if (callClient == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "CallManagerClient instance is null");
        return false;
    }
    callClient->Init(TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID);
    callManagerInited_ = true;
    TAG_LOGI(AAFwkTag::DEFAULT, "CallManagerClient init done");
    return true;
}

bool EnvChecker::IsInCall()
{
    if (!callManagerInited_) {
        TAG_LOGW(AAFwkTag::DEFAULT, "CallManager not initialized, skip in-call check");
        return false;
    }
    auto callClient = DelayedSingleton<Telephony::CallManagerClient>::GetInstance();
    if (callClient == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "CallManagerClient instance is null");
        return false;
    }
    bool hasCall = callClient->HasCall(true);
    if (hasCall) {
        TAG_LOGE(AAFwkTag::DEFAULT, "has call detected, in call check failed");
        return true;
    }
    return false;
}

bool EnvChecker::IsPcDevice()
{
    const std::string& deviceType = AssessmentServiceUtils::GetDeviceType();
    if (deviceType == "2in1") {
        TAG_LOGI(AAFwkTag::DEFAULT, "PC device detected, deviceType: %{public}s", deviceType.c_str());
        return true;
    }
    return false;
}

bool EnvChecker::IsVirtualMachine(std::string &failReason)
{
    if (loader_->IsDegrade()) {
        TAG_LOGI(AAFwkTag::DEFAULT, "VM check degraded, skip (pass by default)");
        return false;
    }
    // The extension fills failReason with the precise sub-item that failed. Fall
    // back to the generic virtual-machine reason when it reports a failure but
    // leaves the reason empty, so the callback message is never blank.
    bool safe = loader_->InvokeCheckAll(std::vector<std::string>{}, failReason);
    if (!safe && failReason.empty()) {
        failReason = FAIL_REASON_VIRTUAL_MACHINE;
    }
    return !safe;
}

bool EnvChecker::IsEdmAdminPresent()
{
    auto proxy = OHOS::EDM::EnterpriseDeviceMgrProxy::GetInstance();
    if (proxy == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "EDM proxy is null, skip EDM admin check");
        return false;
    }
    std::vector<std::shared_ptr<OHOS::AAFwk::Want>> wants;
    ErrCode err = proxy->GetAdmins(wants);
    if (err != ERR_OK) {
        TAG_LOGW(AAFwkTag::DEFAULT, "GetAdmins failed, err: %{public}d, skip EDM admin check", err);
        return false;
    }
    if (!wants.empty()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "EDM admin count: %{public}d", static_cast<int32_t>(wants.size()));
        return true;
    }
    return false;
}

} // namespace AAFwk
} // namespace OHOS
