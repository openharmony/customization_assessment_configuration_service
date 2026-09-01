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
#include "singleton.h"
#include "system_ability_definition.h"
#include "telephony_observer_broker.h"

namespace OHOS {
namespace AAFwk {

void AssessmentTelephonyObserver::OnCallStateUpdated(
    int32_t slotId, int32_t callState, const std::u16string &phoneNumber)
{
    if (controller_ == nullptr || !controller_->IsActivated()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "controller_ is null");
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
    // TODO: replace after merges new api
    int32_t ret = callClient->RejectCall(0, false, u"");
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "RejectCall failed, ret: %{public}d", ret);
    } else {
        TAG_LOGI(AAFwkTag::DEFAULT, "RejectCall succeeded");
    }
}

void ProcessController::Activate(const std::vector<std::string> &allowedApps)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Activate called, allowedApps size: %{public}zu", allowedApps.size());

    InitCallManager();
    RegisterCallObserver();
    DisableScreenReader();

    activated_ = true;
}

void ProcessController::Deactivate()
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Deactivate called");

    if (!activated_) {
        TAG_LOGW(AAFwkTag::DEFAULT, "ProcessController not activated");
        return;
    }

    UnRegisterCallObserver();

    activated_ = false;
    callManagerInited_ = false;
}

bool ProcessController::IsActivated() const
{
    return activated_;
}

void ProcessController::InitCallManager()
{
    if (callManagerInited_) {
        return;
    }
    auto callClient = DelayedSingleton<Telephony::CallManagerClient>::GetInstance();
    if (callClient == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "CallManagerClient instance is null");
        return;
    }
    callClient->Init(TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID);
    callManagerInited_ = true;
    TAG_LOGI(AAFwkTag::DEFAULT, "CallManagerClient init done");
}

void ProcessController::RegisterCallObserver()
{
    if (callObserverRegistered_) {
        return;
    }
    if (telephonyObserver_ == nullptr) {
        telephonyObserver_ = std::make_unique<AssessmentTelephonyObserver>(this).release();
    }
    if (telephonyObserver_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "create AssessmentTelephonyObserver failed");
        return;
    }
    int32_t ret = Telephony::TelephonyObserverClient::GetInstance().AddStateObserver(
        telephonyObserver_, -1, Telephony::TelephonyObserverBroker::OBSERVER_MASK_CALL_STATE, true);
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AddStateObserver failed, ret : %{public}d", ret);
    } else {
        callObserverRegistered_ = true
        TAG_LOGI(AAFwkTag::DEFAULT, "CallObserver registered");
    }
}

void ProcessController::UnRegisterCallObserver()
{
    if (!callObserverRegistered_) {
        return;
    }
    int32_t ret = Telephony::TelephonyObsereverClient::GetInstance().RemoveStateObserver(
        -1, Telephony::TelephonyObserverBroker::OBSERVER_MASK_CALL_STATE);
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "RemoveTelephonyStateObserver failed, ret : %{public}d", ret);
    } else {
        telephonyObserver_ = nullptr;
        callObserverRegistered_ = false;
        TAG_LOGI(AAFwkTag::DEFAULT, "CallObserver unregistered");
    }
}

void ProcessController::DisableScreenReader()
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Disable ScreenReader");
    const std::string screenReaderName = "com.huawei.hmos.screenreader/AccessibilityExtAbility";
    auto ret = AccessibilityConfig::AccessibilityConfig::GetInstance().DisableAbility(screenReaderName);
    if (ret != Accessibility::RetError::RET_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "diable screen reader failed, ret: %{public}d", ret);
    }
}

} // namespace AAFwk
} // namespace OHOS
