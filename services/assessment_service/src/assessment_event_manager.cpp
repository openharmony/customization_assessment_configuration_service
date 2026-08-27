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

#include "assessment_event_manager.h"

#include "hilog_tag_wrapper.h"

namespace OHOS {
namespace AAFwk {

AssessmentEventObserver::AssessmentEventObserver(
    const OHOS::EventFwk::CommonEventSubscribeInfo &subscribeInfo,
    AssessmentEventReceiver receiver) : EventFwk::CommonEventSubscriber(subscribeInfo), receiver_(receiver)
{
}

AssessmentEventObserver::~AssessmentEventObserver()
{
}

std::shared_ptr<AssessmentEventObserver> AssessmentEventObserver::Create(
    const OHOS::EventFwk::CommonEventSubscribeInfo &subscribeInfo, AssessmentEventReceiver receiver)
{
    return std::make_shared<AssessmentEventObserver>(subscribeInfo, receiver);
}

bool AssessmentEventObserver::Subscribe()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!OHOS::EventFwk::CommonEventManager::SubscribeCommonEvent(GetPtr())) {
        TAG_LOGE(AAFwkTag::DEFAULT, "assessment SubscribeCommonEvent fail");
        return false;
    }
    state_ = SUBSCRIBED;
    return true;
}

bool AssessmentEventObserver::Unsubscribe()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != SUBSCRIBED) {
        TAG_LOGE(AAFwkTag::DEFAULT, "state(%{public}d) is not subscribe", state_);
        return true;
    }
    if (!OHOS::EventFwk::CommonEventManager::UnSubscribeCommonEvent(GetPtr())) {
        TAG_LOGE(AAFwkTag::DEFAULT, "UnsubscribeCommonEvent occur exception.");
    }
    state_ = UNSUBSCRIBED;
    return true;
}

void AssessmentEventObserver::OnReceiveEvent(const OHOS::EventFwk::CommonEventData &eventData)
{
    TAG_LOGD(AAFwkTag::DEFAULT, "assessment OnReceiveEvent a message");
    if (receiver_ != nullptr) {
        receiver_(eventData);
    }
}

void AssessmentAbilityConnection::OnAbilityConnectDone(
    const AppExecFwk::ElementName& element, const sptr<IRemoteObject>& remoteObject, int32_t resultCode)
{
    if (remoteObject == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "assessment, remoteObject is nullptr");
        return;
    }
    TAG_LOGD(AAFwkTag::DEFAULT, "assessment, resultCode: %{public}d", resultCode);
    constexpr int32_t PARAM_NUM = 3;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInt32(PARAM_NUM)) {
        return;
    }
    if (!data.WriteString16(u"bundleName")) {
        return;
    }
    if (!data.WriteString16(Str8ToStr16(bundleName_))) {
        return;
    }
    if (!data.WriteString16(u"abilityName")) {
        return;
    }
    if (!data.WriteString16(Str8ToStr16(abilityName_))) {
        return;
    }
    if (!data.WriteString16(u"parameters")) {
        return;
    }
    if (!data.WriteString16(Str8ToStr16(commandStr_))) {
        return;
    }
    if (!data.WriteParcelable(&element)) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Connect done element error.");
        return;
    }
    if (!data.WriteRemoteObject(remoteObject)) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Connect done remote object error.");
        return;
    }
    if (!data.WriteInt32(resultCode)) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Connect done result code error.");
        return;
    }
    int32_t errCode =
        remoteObject->SendRequest(AAFwk::IAbilityConnection::ON_ABILITY_CONNECT_DONE, data, reply, option);
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment remoteObject->SendRequest result %{public}d", errCode);
}

void AssessmentAbilityConnection::OnAbilityDisconnectDone(const AppExecFwk::ElementName& element, int32_t resultCode)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment, resultCode: %{public}d", resultCode);
}
}  // namespace AAFwk
}  // namespace OHOS