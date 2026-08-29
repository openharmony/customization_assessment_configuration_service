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

#ifndef OHOS_ASSESSMENT_EVENT_MANAGER_H
#define OHOS_ASSESSMENT_EVENT_MANAGER_H

#include <thread>

#include "common_event_manager.h"
#include "ability_connection.h"

namespace OHOS {
namespace AAFwk {

using AssessmentEventReceiver = std::function<void(const OHOS::EventFwk::CommonEventData&)>;

class AssessmentEventObserver : public OHOS::EventFwk::CommonEventSubscriber,
    public std::enable_shared_from_this<AssessmentEventObserver> {
public:
    AssessmentEventObserver(
        const OHOS::EventFwk::CommonEventSubscribeInfo &subscribeInfo,
        AssessmentEventReceiver receiver);
    ~AssessmentEventObserver();
    static std::shared_ptr<AssessmentEventObserver> Create(
        const OHOS::EventFwk::CommonEventSubscribeInfo &subscribeInfo,
        AssessmentEventReceiver receiver);
    std::shared_ptr<AssessmentEventObserver> GetPtr()
    {
        return shared_from_this();
    }
    void OnReceiveEvent(const OHOS::EventFwk::CommonEventData &eventData) override;
    bool Subscribe();
    bool Unsubscribe();

private:
    enum SubscribeState {
        IDLE = 0,
        SUBSCRIBED = 1,
        UNSUBSCRIBED = 2,
    };

private:
    SubscribeState state_ = IDLE;
    std::mutex mutex_;
    AssessmentEventReceiver receiver_;
};

class AssessmentAbilityConnection : public AAFwk::AbilityConnectionStub {
public:
    AssessmentAbilityConnection(
        const std::string &bundleName, const std::string &abilityName, const std::string &commandStr)
        : bundleName_(bundleName), abilityName_(abilityName), commandStr_(commandStr) {}
    virtual ~AssessmentAbilityConnection() = default;
    void OnAbilityConnectDone(const AppExecFwk::ElementName& element,
        const sptr<IRemoteObject>& remoteObject, int32_t resultCode) override;
    void OnAbilityDisconnectDone(const AppExecFwk::ElementName& element, int32_t resultCode) override;
private:
    std::string bundleName_;
    std::string abilityName_;
    std::string commandStr_;
};
} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_ASSESSMENT_EVENT_MANGER_H
