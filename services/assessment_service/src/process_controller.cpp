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

#include "accessibility_config.h"
#include "call_manager_client.h"
#include "hilog_tag_wrapper.h"
#include "parameter.h"
#include "singleton.h"
#include "system_ability_definition.h"
#include "telephony_observer_broker.h"

namespace OHOS {
namespace AAFwk {

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
    int32_t ret = callClient->RejectCall(0, false, u"");
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "RejectCall failed, ret: %{public}d", ret);
    } else {
        TAG_LOGI(AAFwkTag::DEFAULT, "RejectCall succeeded");
    }
}

bool ProcessController::Init()
{
    return InitCallManager();
}

bool ProcessController::Activate(const std::vector<std::string> &allowedApps)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Activate called, allowedApps size: %{public}zu", allowedApps.size());
    std::lock_guard<std::mutex> lock(mutex_);

    // Mark activated before registering the observer so that an incoming-call
    // callback arriving during activation already sees the active state.
    activated_ = true;
    RegisterCallObserver();

    if (!DisableScreenReader()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "DisableScreenReader failed, rollback activation");
        UnRegisterCallObserver();
        activated_ = false;
        return false;
    }

    return true;
}

void ProcessController::Deactivate()
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Deactivate called");
    std::lock_guard<std::mutex> lock(mutex_);

    if (!activated_) {
        TAG_LOGW(AAFwkTag::DEFAULT, "ProcessController not activated");
        return;
    }

    UnRegisterCallObserver();

    activated_ = false;
}

bool ProcessController::IsActivated() const
{
    return activated_;
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

void ProcessController::RegisterCallObserver()
{
    if (callObserverRegistered_) {
        return;
    }
    if (telephonyObserver_ == nullptr) {
        telephonyObserver_ = new AssessmentTelephonyObserver(this);
    }
    int32_t ret = Telephony::TelephonyObserverClient::GetInstance().AddStateObserver(
        telephonyObserver_, -1, Telephony::TelephonyObserverBroker::OBSERVER_MASK_CALL_STATE, true);
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AddStateObserver failed, ret : %{public}d", ret);
    } else {
        callObserverRegistered_ = true;
        TAG_LOGI(AAFwkTag::DEFAULT, "CallObserver registered");
    }
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

bool ProcessController::DisableScreenReader()
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Disable ScreenReader");
    const std::string screenReaderName =
        system::GetParameter("persist.assessment.screenreader_name", "");
    auto ret = AccessibilityConfig::AccessibilityConfig::GetInstance().DisableAbility(screenReaderName);
    if (ret == Accessibility::RetError::RET_OK) {
        TAG_LOGI(AAFwkTag::DEFAULT, "Screen reader disabled success");
        return true;
    } else if (ret == Accessibility::RetError::RET_ERR_NO_INJECTOR ||
               ret == Accessibility::RetError::RET_ERR_NOT_INSTALLED ||
               ret == Accessibility::RetError::RET_ERR_NOT_ENABLED) {
        TAG_LOGI(AAFwkTag::DEFAULT, "Screen reader not active, skip disable, ret: %{public}d", ret);
        return true;
    } else {
        TAG_LOGE(AAFwkTag::DEFAULT, "disable screen reader failed, ret: %{public}d", ret);
        return false;
    }
}

} // namespace AAFwk
} // namespace OHOS
