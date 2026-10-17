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

#include <cstddef>
#include "assessment_event_publisher.h"
#include "assessment_utils.h"
#include "hilog_tag_wrapper.h"
#include "common_event_manager.h"
#include "want.h"
#include <hisysevent.h>

namespace OHOS {
namespace AAFwk {
namespace {
const char *const COMMON_EVENT_EXAM_MODE = "common.event.EXAM_MODE";
const char *const ENTER_EXAM_MODE = "ENTER_EXAM_MODE";
const char *const EXIT_EXAM_MODE = "EXIT_EXAM_MODE";

const std::string PNAMEID = "assessment_configuration_service";
const std::string PVERSION = "1.0";

const char *const EVENT_KEY_PNAMEID = "PNAMEID";
const char *const EVENT_KEY_PVERSION = "PVERSION";
const char *const EVENT_KEY_PACKAGE_NAME = "PACKAGE_NAME";
const char *const EVENT_KEY_EXAM_ID = "EXAM_ID";
const char *const EVENT_KEY_EXAM_START_TIME = "EXAM_START_TIME";
const char *const EVENT_KEY_DURATION = "DURATION";
const char *const EVENT_KEY_ALLOWED_APPS = "ALLOWED_APPS";
const char *const EVENT_KEY_ENV_CHECK_RESULT = "ENV_CHECK_RESULT";
const char *const EVENT_KEY_ERROR_REASON = "ERROR_REASON";

const char *const EVENT_KEY_EXAM_DURATION = "EXAM_DURATION";
const char *const EVENT_KEY_EXIT_REASON = "EXIT_REASON";
}

using HiSysEvent = OHOS::HiviewDFX::HiSysEvent;

void AssessmentEventPublisher::PublishEnterExamModeEvent(const EnterExamParam &params, const char *errorReason)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "PublishEnterExamModeEvent called");
    if (errorReason == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Error reason is empty");
        return;
    }

    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_EXAM_MODE);
    want.SetParam(EVENT_KEY_PNAMEID, PNAMEID);
    want.SetParam(EVENT_KEY_PVERSION, PVERSION);
    want.SetParam(EVENT_KEY_PACKAGE_NAME, params.bundleName);
    want.SetParam(EVENT_KEY_EXAM_ID, static_cast<long long>(params.examId));
    want.SetParam(EVENT_KEY_EXAM_START_TIME, static_cast<long long>(params.examStartTime));
    want.SetParam(EVENT_KEY_DURATION, static_cast<long long>(params.duration));
    want.SetParam(EVENT_KEY_ALLOWED_APPS, params.allowedApps);
    want.SetParam(EVENT_KEY_ENV_CHECK_RESULT, params.envCheckResult);
    want.SetParam(EVENT_KEY_ERROR_REASON, errorReason);

    TAG_LOGI(AAFwkTag::DEFAULT, "zsp: %{public}s", errorReason);

    EventFwk::CommonEventData data;
    data.SetWant(want);

    WriteHiSysEnterExamModeEvent(ENTER_EXAM_MODE, params, errorReason);

    if (!EventFwk::CommonEventManager::PublishCommonEvent(data)) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Fail to publish enter exam mode event");
        return;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "Success to publish enter exam mode event");
    TAG_LOGI(AAFwkTag::ASSESSMENT, "[EnterExamMode] error reason: %{public}s", errorReason);
}

void AssessmentEventPublisher::PublishExitExamModeEvent(const ExitExamParam &params,
                                                        const char *exitReason,
                                                        const char *errorReason)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "PublishExitExamModeEvent called");
    if (exitReason == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Exit Reason is empty");
        return;
    }
    if (errorReason == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Error Reason is empty");
        return;
    }
    
    uint64_t examStartTime = params.examStartTime;
    uint64_t examEndTime = AssessmentServiceUtils::GetCurrentAssessmentTimeStamp();
    uint64_t EXAM_DURATION = (examEndTime > examStartTime) ? (examEndTime - examStartTime) : 0;

    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_EXAM_MODE);
    want.SetParam(EVENT_KEY_PNAMEID, PNAMEID);
    want.SetParam(EVENT_KEY_PVERSION, PVERSION);
    want.SetParam(EVENT_KEY_PACKAGE_NAME, params.bundleName);
    want.SetParam(EVENT_KEY_EXAM_ID, static_cast<long long>(params.examId));
    want.SetParam(EVENT_KEY_EXAM_DURATION, static_cast<long long>(EXAM_DURATION));
    want.SetParam(EVENT_KEY_EXIT_REASON, exitReason);
    want.SetParam(EVENT_KEY_ERROR_REASON, errorReason);

    TAG_LOGI(AAFwkTag::DEFAULT, "zsp: %{public}s, %{public}s", exitReason, errorReason);

    EventFwk::CommonEventData data;
    data.SetWant(want);

    WriteHiSysExitExamModeEvent(EXIT_EXAM_MODE, params, EXAM_DURATION, exitReason, errorReason);

    if (!EventFwk::CommonEventManager::PublishCommonEvent(data)) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Fail to publish exit exam mode event");
        return;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "Success to publish exit exam mode event");
    TAG_LOGI(AAFwkTag::ASSESSMENT,
             "[ExitExamMode] exit reason: %{public}s, error reason: %{public}s", exitReason, errorReason);
}

void AssessmentEventPublisher::WriteHiSysEnterExamModeEvent(const char* eventName,
                                                            const EnterExamParam& params,
                                                            const char *errorReason)
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "WriteHiSysEnterExamModeEvent, eventName:%{public}s", eventName);
    
    std::string errorReasonStr(errorReason ? errorReason : "");

    int32_t ret = HiSysEventWrite(
        HiSysEvent::Domain::ASSESSMENT_UE, eventName,
        HiSysEvent::EventType::BEHAVIOR,
        EVENT_KEY_PNAMEID, PNAMEID,
        EVENT_KEY_PVERSION, PVERSION,
        EVENT_KEY_PACKAGE_NAME, params.bundleName,
        EVENT_KEY_EXAM_ID, params.examId,
        EVENT_KEY_EXAM_START_TIME, params.examStartTime,
        EVENT_KEY_DURATION, params.duration,
        EVENT_KEY_ALLOWED_APPS, params.allowedApps,
        EVENT_KEY_ENV_CHECK_RESULT, params.envCheckResult,
        EVENT_KEY_ERROR_REASON, errorReasonStr);
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "Write HiSysEnterExamModeEvent error, eventName:%{public}s, errCode:%{public}d",
                 eventName, ret);
    }
}

void AssessmentEventPublisher::WriteHiSysExitExamModeEvent(const char* eventName,
                                                           const ExitExamParam& params,
                                                           uint64_t examDuration,
                                                           const char *exitReason,
                                                           const char *errorReason)
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "WriteHiSysExitExamModeEvent, eventName:%{public}s", eventName);

    std::string exitReasonStr(exitReason ? exitReason : "");
    std::string errorReasonStr(errorReason ? errorReason : "");

    int32_t ret = HiSysEventWrite(
        HiSysEvent::Domain::ASSESSMENT_UE, eventName,
        HiSysEvent::EventType::BEHAVIOR,
        EVENT_KEY_PNAMEID, PNAMEID,
        EVENT_KEY_PVERSION, PVERSION,
        EVENT_KEY_PACKAGE_NAME, params.bundleName,
        EVENT_KEY_EXAM_ID, params.examId,
        EVENT_KEY_EXAM_DURATION, examDuration,
        EVENT_KEY_EXIT_REASON, exitReasonStr,
        EVENT_KEY_ERROR_REASON, errorReasonStr);
    if (ret != 0) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "Write HiSysExitExamModeEvent error, eventName:%{public}s, errCode:%{public}d",
                 eventName, ret);
    }
}
}  // namespace AAFwk
}  // namespace OHOS
