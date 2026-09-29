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

#include "assessment_utils.h"
#include "call_manager_client.h"
#include "device_manager.h"
#include "display_manager.h"
#include "hilog_tag_wrapper.h"
#include "screen_info.h"
#include "screen_manager.h"
#include "singleton.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace AAFwk {

const std::string EXTENSION_SO_PATH = "libassessment_ext.z.so";

void AssessmentDmInitCallback::OnRemoteDied()
{
    TAG_LOGI(AAFwkTag::DEFAULT, "DeviceManager service remote died");
}

EnvChecker::EnvChecker()
{
    dmInitCallback_ = std::make_shared<AssessmentDmInitCallback>();
    loader_ = std::make_unique<ExtensionLoader>(EXTENSION_SO_PATH);
}

bool EnvChecker::Init()
{
    if (!loader_->InitExtensionLoader()) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Extension loader degraded, VM check will pass by default");
    }
    bool dmOk = InitDeviceManager();
    bool cmOk = InitCallManager();
    return dmOk && cmOk;
}

bool EnvChecker::InitDeviceManager()
{
    if (deviceManagerInited_) {
        return true;
    }
    int32_t ret = DistributedHardware::DeviceManager::GetInstance().InitDeviceManager(
        "assessment_service", dmInitCallback_);
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "InitDeviceManager failed, ret: %{public}d", ret);
        return false;
    }
    deviceManagerInited_ = true;
    TAG_LOGI(AAFwkTag::DEFAULT, "DeviceManager Init done");
    return true;
}

bool EnvChecker::CheckAll()
{
    if (IsScreenRecording()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Screen recording detected");
        return false;
    }

    if (IsScreenCasting()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Screen casting detected");
        return false;
    }

    if (IsMultiScreen()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Multi-screen detected");
        return false;
    }

    if (IsInCall()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "In call detected");
        return false;
    }

    if (IsPcDevice()) {
        if (IsVirtualMachine()) {
            TAG_LOGE(AAFwkTag::DEFAULT, "Virtual machine running detected");
            return false;
        }
    }

    return true;
}

bool EnvChecker::IsAwakeAnco(std::string ancoState)
{
    return loader_->InvokeIsAwakeAnco(ancoState);
}

void EnvChecker::RestrictAncoApp()
{
    return loader_->InvokeRestrictAncoApp();
}

bool EnvChecker::IsScreenRecording()
{
    bool isCaptured = Rosen::DisplayManager::GetInstance().IsCaptured();
    if (isCaptured) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Screen recording detected: isCaptured = true");
        return true;
    }
    return false;
}

bool EnvChecker::IsScreenCasting()
{
    if (!deviceManagerInited_) {
        TAG_LOGW(AAFwkTag::DEFAULT, "DeviceManager not init, skip screen casting check");
        return false;
    }
    std::vector<DistributedHardware::DmDeviceBasicInfo> devList;
    int32_t ret = DistributedHardware::DeviceManager::GetInstance().GetAvailableDeviceList(
        "assessment_service", devList);
    if (ret != 0) {
        TAG_LOGW(AAFwkTag::DEFAULT, "GetAvailableDeviceList failed, ret: %{public}d", ret);
        return false;
    }

    // Exclude the local device: GetAvailableDeviceList may include self.
    DistributedHardware::DmDeviceInfo localDeviceInfo;
    ret = DistributedHardware::DeviceManager::GetInstance().GetLocalDeviceInfo(
        "assessment_service", localDeviceInfo);
    std::string localDeviceId = (ret == 0) ? std::string(localDeviceInfo.deviceId) : "";

    for (const auto &device : devList) {
        if (!localDeviceId.empty() && std::string(device.deviceId) == localDeviceId) {
            continue;
        }
        TAG_LOGE(AAFwkTag::DEFAULT, "Remote device detected on soft bus, device count: %{public}d",
            static_cast<int32_t>(devList.size()));
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
            TAG_LOGE(AAFwkTag::DEFAULT, "Non built-in screen detected, screenId: %{public}" PRIu64
                ", screenType: %{public}d", screen->GetId(), static_cast<int32_t>(screenType));
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
        TAG_LOGE(AAFwkTag::DEFAULT, "has call detected");
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

bool EnvChecker::IsVirtualMachine()
{
    if (loader_->IsDegrade()) {
        TAG_LOGI(AAFwkTag::DEFAULT, "VM check degraded, skip (pass by default)");
        return false;
    }
    return !loader_->InvokeCheckAll(std::vector<std::string>{});
}

} // namespace AAFwk
} // namespace OHOS
