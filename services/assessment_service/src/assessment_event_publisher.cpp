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

#include "assessment_event_publisher.h"
#include "assessment_service.h"
#include "assessment_utils.h"
#include "hilog_tag_wrapper.h"
#include "common_event_manager.h"
#include "want.h"
#include <cstddef>

namespace OHOS {
namespace AAFwk {
namespace {
const char *const ENTER_EXAM_MODE = "ENTER_EXAM_MODE";
const char *const EXIT_EXAM_MODE = "EXIT_EXAM_MODE";
const char *const PACKAGE_NAME_ID = "assessment_configuration_service";
const char *const PACKAGE_VERSION = "1.0";
}

void AssessmentEventPublisher::PublishEnterExamModeEvent(const char *errorReason)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "PublishEnterExamModeEvent called");
    if (errorReason == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Invalid errorReason");
        return;
    }

    OHOS::AAFwk::Want want;
    want.SetAction(ENTER_EXAM_MODE);
    want.SetParam("PNAMEID", PACKAGE_NAME_ID);
    want.SetParam("PVERSION", PACKAGE_VERSION);
    want.SetParam("PACKAGE_NAME", AssessmentService::GetInstance()->GetAssessmentBundleName());
    want.SetParam("EXAM_ID",
                  static_cast<long long>(AssessmentService::GetInstance()->GetAssessmentCurrentConfig().examId));
    want.SetParam(
        "EXAM_START_TIME",
        static_cast<long long>(AssessmentService::GetInstance()->GetAssessmentCurrentConfig().examStartTime));
    want.SetParam("DURATION",
                  static_cast<long long>(AssessmentService::GetInstance()->GetAssessmentCurrentConfig().duration));
    want.SetParam("ALLOWED_APPS", AssessmentService::GetInstance()->GetAssessmentCurrentConfig().allowedApps);
    want.SetParam("ERROR_REASON", errorReason);

    EventFwk::CommonEventData data;
    data.SetWant(want);

    if (!EventFwk::CommonEventManager::PublishCommonEvent(data)) {
        TAG_LOGW(AAFwkTag::DEFAULT, "fail to publish enter exam mode event");
        return;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "success to publish enter exam mode event");
}

void AssessmentEventPublisher::PublishExitExamModeEvent(const char *exitReason, const char *errorReason)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "PublishExitExamModeEvent called");
    if (errorReason == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Invalid errorReason");
        return;
    }
    
    uint64_t examStartTime = AssessmentService::GetInstance()->GetAssessmentCurrentConfig().examStartTime;
    uint64_t examEndTime = AssessmentServiceUtils::GetCurrentAssessmentTimeStamp();
    uint64_t EXAM_DURATION = examEndTime - examStartTime;

    OHOS::AAFwk::Want want;
    want.SetAction(EXIT_EXAM_MODE);
    want.SetParam("PNAMEID", PACKAGE_NAME_ID);
    want.SetParam("PVERSION", PACKAGE_VERSION);
    want.SetParam("PACKAGE_NAME", AssessmentService::GetInstance()->GetAssessmentBundleName());
    want.SetParam("EXAM_ID",
                  static_cast<long long>(AssessmentService::GetInstance()->GetAssessmentCurrentConfig().examId));
    want.SetParam("EXAM_DURATION", static_cast<long long>(EXAM_DURATION));
    want.SetParam("EXIT_REASON", exitReason);
    want.SetParam("ERROR_REASON", errorReason);

    EventFwk::CommonEventData data;
    data.SetWant(want);

    if (!EventFwk::CommonEventManager::PublishCommonEvent(data)) {
        TAG_LOGW(AAFwkTag::DEFAULT, "fail to publish exit exam mode event");
        return;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "success to publish exit exam mode event");
}
}  // namespace AAFwk
}  // namespace OHOS
