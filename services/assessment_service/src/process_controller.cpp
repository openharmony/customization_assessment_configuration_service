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

    // Each stage that succeeds appends how to undo itself, so a later failure can be
    // rolled back without this function keeping track of which stages ran.
    std::vector<std::function<void()>> undoActions;

    // Distributed softbus must be stopped before the assessment starts, otherwise
    // assessment content could still be shared with nearby devices.
    int32_t ret = ServiceControl(SOFTBUS_SERVICE_NAME, ServiceAction::STOP);
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Stop %{public}s failed, ret : %{public}d", SOFTBUS_SERVICE_NAME, ret);
        failReason = FAIL_REASON_SOFTBUS_CONTROL;
        return false;
    }
    undoActions.push_back([this]() { StartSoftBusForRollback(); });
    TAG_LOGI(AAFwkTag::DEFAULT, "%{public}s stopped", SOFTBUS_SERVICE_NAME);

    // Mark activated before registering the observer so that an incoming-call
    // callback arriving during activation already sees the active state.
    activated_ = true;

    // Every sub-item is fatal: an assessment must not run without a restriction it
    // asked for, so a failure rolls back the stages already completed.
    if (!RegisterCallObserver()) {
        failReason = FAIL_REASON_CALL_OBSERVER;
        RollbackActivateLockedUnsafe(undoActions);
        return false;
    }
    undoActions.push_back([this]() { UnRegisterCallObserver(); });

    if (!RegisterScreenListener()) {
        failReason = FAIL_REASON_SCREEN_LISTENER;
        RollbackActivateLockedUnsafe(undoActions);
        return false;
    }
    undoActions.push_back([this]() { UnregisterScreenListener(); });

    if (!ActivateExtensionLockedUnsafe(allowedApps, failReason, undoActions)) {
        RollbackActivateLockedUnsafe(undoActions);
        return false;
    }

    return true;
}

bool ProcessController::ActivateExtensionLockedUnsafe(const std::vector<std::string> &allowedApps,
    std::string &failReason, std::vector<std::function<void()>> &undoActions)
{
    if (extLoader_ == nullptr) {
        // No closed-source extension on this device: nothing to apply, nothing to undo.
        return true;
    }
    // ActivateAll fails fast internally, so it may already have applied some of its
    // sub-items by the time it reports a failure: append the undo before calling it.
    undoActions.push_back([this]() { RevertExtensionForRollback(); });
    if (extLoader_->InvokeActivateAll(allowedApps, failReason)) {
        return true;
    }
    TAG_LOGE(AAFwkTag::DEFAULT, "ActivateAll failed");
    if (failReason.empty()) {
        failReason = FAIL_REASON_ENABLE_VM;
    }
    return false;
}

void ProcessController::RollbackActivateLockedUnsafe(std::vector<std::function<void()>> &undoActions)
{
    TAG_LOGW(AAFwkTag::DEFAULT, "Activate failed, rolling back %{public}zu stage(s)", undoActions.size());
    // Clear the active flag first so that a callback firing during the rollback, for
    // instance a screen hotplug event, already sees the assessment as inactive.
    activated_ = false;
    while (!undoActions.empty()) {
        undoActions.back()();
        undoActions.pop_back();
    }
}

void ProcessController::StartSoftBusForRollback()
{
    if (ServiceControl(SOFTBUS_SERVICE_NAME, ServiceAction::START) != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Start %{public}s failed during rollback", SOFTBUS_SERVICE_NAME);
    }
}

void ProcessController::RevertExtensionForRollback()
{
    if (extLoader_ == nullptr) {
        return;
    }
    std::string discard;
    extLoader_->InvokeDeactivateAll(discard);
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
