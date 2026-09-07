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
#include <condition_variable>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>

#include "event_runner.h"
#include "event_handler.h"
#include "assessment_error_code.h"
#include "assessment_service_stub.h"
#include "assessment_event_manager.h"

namespace OHOS {
namespace AAFwk {

struct AssessmentConfig {
    uint32_t duration = 0;
    std::vector<std::string> allowedApps;
};

enum class AssessmentExamStatus : uint32_t {
    IDLE = 0,
    CONFIRMING = 1,
    ACTIVE = 2,
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
    ErrCode IsActive(bool &isActive, int32_t &errCode) override;
    ErrCode GetConfiguration(
        uint32_t &duration, std::vector<std::string> &allowedApps, int32_t &errCode) override;
    
    void NotifyBegin(int32_t code, const std::string &message);
    void NotifyInterrupted(int32_t reason, const std::string &message);
    void NotifyEnd();

    void LoadState();
    void SaveState();
    void ClearState();

    void DoLoop();
    void DispatchEvent(const OHOS::EventFwk::CommonEventData& data);

private:
    void ConfigCurrentSession(const sptr<IRemoteObject> &token, uint32_t duration,
                              const std::vector<std::string> &allowedApps,
                              const sptr<IRemoteObject> &callback);
    void CleanupCurrentSession();
    bool InitSubsystems();

    int32_t InvokeSystemDialog();
    int32_t ComputeNextTaskTimeoutLockedUnsafe();
    void CheckEndpointAndExecuteTaskLockedUnsafe();
    void Quit();

    bool SubscribeCommonEvent();
    void UnsubscribeCommonEvent();
    void HandleBegin(const std::string &ticket, uint32_t operation);
    void ConfirmationBeginLockedUnsafe();
    void CancelBeginLockedUnsafe();
    void TimeoutLockedUnsafe();

    static std::mutex mutex_;
    static sptr<AssessmentService> instance_;
    std::shared_ptr<AppExecFwk::EventRunner> eventRunner_;
    std::shared_ptr<AppExecFwk::EventHandler> eventHandler_;

    bool isActive_ = false;
    sptr<IRemoteObject> callerToken_;
    AssessmentConfig currentConfig_;
    uint64_t endpointCheckPoint_ = 0;
    AssessmentExamStatus examStatus_ = AssessmentExamStatus::IDLE;
    std::string ticket_;

    std::mutex mutexSa_;
    std::condition_variable condSa_;
    std::atomic<bool> running_ = false;
    std::thread thread_;

    std::shared_ptr<AssessmentEventObserver> assessmentEventObserver_;

    DISALLOW_COPY_AND_MOVE(AssessmentService);
    int32_t switchId_ = -1;
};
} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_ASSESSMENT_SERVICE_H