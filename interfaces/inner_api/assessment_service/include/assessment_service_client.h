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

#ifndef OHOS_ASSESSMENT_SERVICE_CLIENT_H
#define OHOS_ASSESSMENT_SERVICE_CLIENT_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>

#include "iremote_object.h"
#include "system_ability_definition.h"
#include "iassessment_service.h"

namespace OHOS {
namespace AAFwk {
class AssessmentServiceClient : public std::enable_shared_from_this<AssessmentServiceClient> {
public:
    AssessmentServiceClient() = default;
    virtual ~AssessmentServiceClient() = default;

    static std::shared_ptr<AssessmentServiceClient> GetInstance();

    ErrCode Begin(const sptr<IRemoteObject> &token, uint32_t duration,
                  const std::vector<std::string> &allowedApps,
                  const sptr<IRemoteObject> &callback);
    ErrCode End(const sptr<IRemoteObject> &token);
    ErrCode IsActive(bool &isActive);
    ErrCode GetConfiguration(uint32_t &duration, std::vector<std::string> &allowedApps);

    void OnLoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject);
    void OnLoadSystemAbilityFail();

private:
    sptr<IAssessmentService> GetAssessmentProxy();
    sptr<IAssessmentService> GetAssessmentProxyWithCheck();
    void ClearProxy();
    bool LoadAssessmentService();
    void SetAssessmentService(const sptr<IRemoteObject> &remoteObject);
    sptr<IAssessmentService> GetAssessmentService();

    class AssessmentDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit AssessmentDeathRecipient(const std::weak_ptr<AssessmentServiceClient> &client,
                                          const std::function<void(const wptr<IRemoteObject>&)> &proxy)
            : client_(client), proxy_(proxy) {}
        virtual ~AssessmentDeathRecipient() = default;
        void OnRemoteDied([[maybe_unused]] const wptr<IRemoteObject> &remote) override;

    private:
        std::weak_ptr<AssessmentServiceClient> client_;
        std::function<void(const wptr<IRemoteObject>&)> proxy_;
    };

private:
    std::condition_variable loadSaCondation_;
    std::mutex loadSaMutex_;
    bool loadSaFinished_ = false;
    std::mutex mutex_;
    sptr<IAssessmentService> assessmentService_ = nullptr;
};
} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_ASSESSMENT_SERVICE_CLIENT_H