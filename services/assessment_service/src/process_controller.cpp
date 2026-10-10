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

#include "process_controller.h"

#include <cinttypes>

#include "assessment_fail_reason.h"
#include "call_manager_client.h"
#include "hilog_tag_wrapper.h"
#include "parameter.h"
#include "service_control.h"
#include "singleton.h"
#include "system_ability_definition.h"
#include "telephony_observer_broker.h"

namespace OHOS {
namespace AAFwk {
namespace {
const char *const SOFTBUS_SERVICE_NAME = "softbus_server";
}

const std::string EXTENSION_SO_PATH = "libassessment_configuration_service_ext.z.so";

void AssessmentTelephonyObserver::OnCallStateUpdated(
    int32_t slotId, int32_t callState, const std::u16string &phoneNumber)
{
    if (controller_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "controller_ is null");
        return;
    }

    if (!controller_->IsActivated()) {
        TAG_LOGW(AAFwkTag::DEFAULT, "controller_ not activated, skip call state update");
        return;
    }

    TAG_LOGI(AAFwkTag::DEFAULT, "OnCallStateUpdated slotId: %{public}d, callState: %{public}d", slotId, callState);

    if (callState != static_cast<int32_t>(Telephony::TelCallState::CALL_STATUS_INCOMING) &&
        callState != static_cast<int32_t>(Telephony::TelCallState::CALL_STATUS_WAITING)) {
        return;
    }

    auto callClient = DelayedSingleton<Telephony::CallManagerClient>::GetInstance();
    if (callClient == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "CallManagerClient is null");
        return;
    }

    TAG_LOGI(AAFwkTag::DEFAULT, "Incoming call detected, auto-rejecting");
    int32_t ret = callClient->RejectCall(Telephony::RejectType::CALL_REJECT_MISSED_CALL);
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "RejectCall failed, ret: %{public}d", ret);
    } else {
        TAG_LOGI(AAFwkTag::DEFAULT, "RejectCall succeeded");
    }
}

void AssessmentScreenListener::OnConnect(Rosen::ScreenId screenId)
{
    if (controller_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "controller_ is null");
        return;
    }

    if (!controller_->IsActivated()) {
        TAG_LOGW(AAFwkTag::DEFAULT, "controller_ not activated, skip screen connect");
        return;
    }

    TAG_LOGI(AAFwkTag::DEFAULT, "OnConnect screenId: %{public}" PRIu64, screenId);
    controller_->NotifyScreenConnected(screenId);
}

void AssessmentScreenListener::OnDisconnect(Rosen::ScreenId screenId)
{
    // Unplugging a screen does not end the assessment; the anomaly has already
    // been reported by OnConnect if the screen was an external one.
    TAG_LOGI(AAFwkTag::DEFAULT, "OnDisconnect screenId: %{public}" PRIu64, screenId);
}

void AssessmentScreenListener::OnChange(Rosen::ScreenId screenId)
{
    // Resolution/rotation changes are not an environment anomaly.
    TAG_LOGI(AAFwkTag::DEFAULT, "OnChange screenId: %{public}" PRIu64, screenId);
}

bool ProcessController::Init()
{
    extLoader_ = std::make_unique<ExtensionLoader>(EXTENSION_SO_PATH);
    if (!extLoader_->InitExtensionLoader()) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Extension loader degraded, VM control will be skipped");
    }
    return InitCallManager();
}

bool ProcessController::Activate(const std::vector<std::string> &allowedApps, std::string &failReason)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Activate called, allowedApps size: %{public}zu", allowedApps.size());
    std::lock_guard<std::mutex> lock(mutex_);

    // Distributed softbus must be stopped before the assessment starts, otherwise
    // assessment content could still be shared with nearby devices.
    int32_t ret = ServiceControl(SOFTBUS_SERVICE_NAME, ServiceAction::STOP);
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Stop %{public}s failed, ret : %{public}d", SOFTBUS_SERVICE_NAME, ret);
        failReason = FAIL_REASON_SOFTBUS_CONTROL;
        return false;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "%{public}s stopped", SOFTBUS_SERVICE_NAME);

    // Mark activated before registering the observer so that an incoming-call
    // callback arriving during activation already sees the active state.
    activated_ = true;

    // The remaining sub-items are non-fatal: the assessment still starts, but the
    // first failure is reported so that it reaches the trace event. Keeping the
    // first one only, because a single reason string cannot carry several.
    if (!RegisterCallObserver() && failReason.empty()) {
        failReason = FAIL_REASON_CALL_OBSERVER;
    }
    if (!RegisterScreenListener() && failReason.empty()) {
        failReason = FAIL_REASON_SCREEN_LISTENER;
    }

    if (extLoader_ != nullptr && !extLoader_->InvokeActivateAll(allowedApps, failReason)) {
        TAG_LOGW(AAFwkTag::DEFAULT, "ActivateAll failed, process control degraded");
        if (failReason.empty()) {
            failReason = FAIL_REASON_ENABLE_VM;
        }
    }

    return true;
}

void ProcessController::Deactivate(std::string &failReason)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Deactivate called");
    std::lock_guard<std::mutex> lock(mutex_);

    if (!activated_) {
        TAG_LOGW(AAFwkTag::DEFAULT, "ProcessController not activated");
        return;
    }

    // Clear the flag before unregistering, so that a screen-connect callback
    // already in flight on the RenderService thread bails out at IsActivated()
    // instead of re-entering Deactivate() through the anomaly callback while
    // mutex_ is held here.
    activated_ = false;

    UnregisterScreenListener();
    UnRegisterCallObserver();

    // Restoring the device is best-effort: a failure here must not abort the
    // remaining restore steps, but it is reported so that the caller can publish
    // a trace event for it.
    if (extLoader_ != nullptr && !extLoader_->InvokeDeactivateAll(failReason)) {
        if (failReason.empty()) {
            failReason = FAIL_REASON_ENABLE_VM;
        }
    }

    int32_t ret = ServiceControl(SOFTBUS_SERVICE_NAME, ServiceAction::START);
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Start %{public}s failed, ret : %{public}d", SOFTBUS_SERVICE_NAME, ret);
        if (failReason.empty()) {
            failReason = FAIL_REASON_SOFTBUS_CONTROL;
        }
    } else {
        TAG_LOGI(AAFwkTag::DEFAULT, "%{public}s started", SOFTBUS_SERVICE_NAME);
    }
}

bool ProcessController::IsActivated() const
{
    return activated_;
}

void ProcessController::SetEnvAnomalyCallback(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    envAnomalyCallback_ = std::move(callback);
}

void ProcessController::NotifyScreenConnected(Rosen::ScreenId screenId)
{
    // No mutex_ here on purpose: the callback ends up calling Deactivate(),
    // which takes mutex_. See the header comment on NotifyScreenConnected().
    if (!activated_) {
        TAG_LOGW(AAFwkTag::DEFAULT, "not activated, skip screen connect %{public}" PRIu64, screenId);
        return;
    }
    if (envAnomalyCallback_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "envAnomalyCallback_ is null, cannot report screen connect");
        return;
    }
    TAG_LOGE(AAFwkTag::DEFAULT, "screen connected during assessment, screenId: %{public}" PRIu64, screenId);
    envAnomalyCallback_();
}

bool ProcessController::InitCallManager()
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

bool ProcessController::RegisterCallObserver()
{
    if (callObserverRegistered_) {
        return true;
    }
    if (telephonyObserver_ == nullptr) {
        telephonyObserver_ = new AssessmentTelephonyObserver(this);
    }
    int32_t ret = Telephony::TelephonyObserverClient::GetInstance().AddStateObserver(
        telephonyObserver_, -1, Telephony::TelephonyObserverBroker::OBSERVER_MASK_CALL_STATE, true);
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AddStateObserver failed, ret : %{public}d", ret);
        return false;
    }
    callObserverRegistered_ = true;
    TAG_LOGI(AAFwkTag::DEFAULT, "CallObserver registered");
    return true;
}

void ProcessController::UnRegisterCallObserver()
{
    if (!callObserverRegistered_) {
        return;
    }
    int32_t ret = Telephony::TelephonyObserverClient::GetInstance().RemoveStateObserver(
        -1, Telephony::TelephonyObserverBroker::OBSERVER_MASK_CALL_STATE);
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "RemoveTelephonyStateObserver failed, ret : %{public}d", ret);
    } else {
        telephonyObserver_ = nullptr;
        callObserverRegistered_ = false;
        TAG_LOGI(AAFwkTag::DEFAULT, "CallObserver unregistered");
    }
}

bool ProcessController::RegisterScreenListener()
{
    if (screenListenerRegistered_) {
        return true;
    }
    if (screenListener_ == nullptr) {
        screenListener_ = new AssessmentScreenListener(this);
    }
    Rosen::DMError ret = Rosen::ScreenManager::GetInstance().RegisterScreenListener(screenListener_);
    if (ret != Rosen::DMError::DM_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "RegisterScreenListener failed, ret : %{public}d", static_cast<int32_t>(ret));
        return false;
    }
    screenListenerRegistered_ = true;
    TAG_LOGI(AAFwkTag::DEFAULT, "ScreenListener registered");
    return true;
}

void ProcessController::UnregisterScreenListener()
{
    if (!screenListenerRegistered_) {
        return;
    }
    Rosen::DMError ret = Rosen::ScreenManager::GetInstance().UnregisterScreenListener(screenListener_);
    if (ret != Rosen::DMError::DM_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "UnregisterScreenListener failed, ret : %{public}d", static_cast<int32_t>(ret));
    } else {
        screenListener_ = nullptr;
        screenListenerRegistered_ = false;
        TAG_LOGI(AAFwkTag::DEFAULT, "ScreenListener unregistered");
    }
}

} // namespace AAFwk
} // namespace OHOS
