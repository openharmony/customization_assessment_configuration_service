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

#ifndef OHOS_ASSESSMENT_SERVICE_H
#define OHOS_ASSESSMENT_SERVICE_H

#include <mutex>
#include <string>
#include <vector>

#include "event_runner.h"
#include "event_handler.h"
#include "assessment_error_code.h"
#include "assessment_service_stub.h"

namespace OHOS {
namespace AAFwk {

struct AssessmentConfig {
    uint32_t duration = 0;
    std::vector<std::string> allowedApps;
};

class AssessmentService : public AssessmentServiceStub,
                          public std::enable_shared_from_this<AssessmentService> {
public:
    AssessmentService() = default;
    virtual ~AssessmentService() = default;

    static sptr<AssessmentService> GetInstance();

    bool Init();

    ErrCode Begin(const sptr<IRemoteObject> &token, uint32_t duration,
                  const std::vector<std::string> &allowedApps,
                  const sptr<IRemoteObject> &callback, int32_t &errCode) override;
    ErrCode End(const sptr<IRemoteObject> &token, int32_t &errCode) override;
    ErrCode IsActive(bool &isActive) override;
    ErrCode GetConfiguration(uint32_t &duration, std::vector<std::string> &allowedApps) override;
    
    void NotifyBegin(int32_t code, const std::string &message);
    void NotifyInterrupted(int32_t reason, const std::string &message);
    void NotifyEnd();

    void LoadState();
    void SaveState();
    void ClearState();

private:
    void CleanupCurrentSession();

    static std::mutex mutex_;
    static sptr<AssessmentService> instance_;
    std::shared_ptr<AppExecFwk::EventRunner> eventRunner_;
    std::shared_ptr<AppExecFwk::EventHandler> eventHandler_;

    bool isActive_ = false;
    sptr<IRemoteObject> callerToken_;
    AssessmentConfig currentConfig_;

    DISALLOW_COPY_AND_MOVE(AssessmentService);
};
} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_ASSESSMENT_SERVICE_H