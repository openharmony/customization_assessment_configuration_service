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

#ifndef OHOS_ASSESSMENT_EVENT_PUBLISHER_H
#define OHOS_ASSESSMENT_EVENT_PUBLISHER_H

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS {
namespace AAFwk {
namespace {
const char* const TRACE_ERR_REASON_NO_ERROR = "NO_ERROR";
const char* const TRACE_ERR_REASON_INVALID_PARAMS = "INVALID_PARAMS";
const char* const TRACE_ERR_REASON_PERMISSION_DENIED = "PERMISSION_DENIED";
const char* const TRACE_ERR_REASON_DEVICE_NOT_SUPPORTED = "DEVICE_NOT_SUPPORTED";
const char* const TRACE_ERR_REASON_EXAM_ALREADY_STARTED = "EXAM_ALREADY_STARTED";
const char* const TRACE_ERR_REASON_USER_CANCEL = "USER_CANCEL";
const char* const TRACE_ERR_REASON_CALL_KIOSK_FAIL = "CALL_KIOSK_FAIL";
const char* const TRACE_ERR_REASON_SAVE_SYSPARAM_FAIL = "SAVE_SYSPARAM_FAIL";
const char* const TRACE_ERR_REASON_NO_EXAM = "NO_EXAM";
const char* const TRACE_ERR_REASON_RESET_SYSPARAM_FAIL = "RESET_SYSPARAM_FAIL";
const char* const TRACE_ERR_REASON_NON_OWNER_END = "NON_OWNER_END";
const char* const TRACE_ERR_REASON_LOCK_SCREENOFF_FAILED = "LOCK_SCREENOFF_FAILED";
const char* const TRACE_ERR_REASON_KILL_ANCO_APP_FAILED = "KILL_ANCO_APP_FAILED";
const char* const TRACE_ERR_REASON_APP_DIED = "APP_DIED";
}

namespace {
const char* const TRACE_EXIT_REASON_TIMEOUT = "TIMEOUT";
const char* const TRACE_EXIT_REASON_USER_INITIATED_EXIT = "USER_INITIATED_EXIT";
const char* const TRACE_EXIT_REASON_ENTER_HIBERNATE = "ENTER_HIBERNATE";
const char* const TRACE_EXIT_REASON_ENTER_POWER_SAVE_MODE = "ENTER_POWER_SAVE_MODE";
const char* const TRACE_EXIT_REASON_SHUTDOWN = "SHUTDOWN";
const char* const TRACE_EXIT_REASON_DEVICE_REBOOT = "DEVICE_REBOOT";
const char* const TRACE_EXIT_REASON_LID_CLOSED = "LID_CLOSED";
}

struct EnterExamParam {
    std::string bundleName;
    std::uint32_t examId = 0;
    std::uint64_t examStartTime = 0;
    std::uint64_t duration = 0;
    std::vector<std::string> allowedApps;
    bool envCheckResult = false;
};

struct ExitExamParam {
    std::string bundleName;
    std::uint32_t examId = 0;
    std::uint64_t examStartTime = 0;
};

class AssessmentEventPublisher {
public:
    static void PublishEnterExamModeEvent(const EnterExamParam &params, const char *errorReason);
    static void PublishExitExamModeEvent(const ExitExamParam &params,
        const char *exitReason, const char *errorReason);
private:
    static void WriteHiSysEnterExamModeEvent(const char *eventName, const EnterExamParam &params,
                                             const char *errorReason);
    static void WriteHiSysExitExamModeEvent(const char *eventName, const ExitExamParam &params,
                                            uint64_t examDuration, const char *exitReason,
                                            const char *errorReason);
};

} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_ASSESSMENT_EVENT_PUBLISHER_H
