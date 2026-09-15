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

#include "assessment_api_error_code.h"

namespace OHOS {
namespace AAFwk {

namespace {
const std::string ERR_INVALID_PARAMS_DESC = "Invalid parameter";
const std::string ERR_PERMISSION_DENIED_DESC = "Permission denied";
const std::string ERR_CAPABILITY_NOT_SUPPORT_DESC = "Capability not supported";
const std::string ERR_INTERNAL_ERROR_DESC = "Assessment Internal error";
const std::string ERR_ASSESSMENT_ALREADY_ACTIVE_DESC = "Assessment configuration service is already active";
const std::string ERR_ASSESSMENT_NOT_ACTIVE_DESC = "Assessment configuration service is not active";
const std::string ERR_INVALID_OPERATION_DESC = "Invalid operation";

const std::string ASSESSMENT_EVENT_CODE_OK_DESC = "OK";
const std::string ASSESSMENT_EVENT_CODE_USER_CANCEL_DESC = "User Cancel";
const std::string ASSESSMENT_EVENT_CODE_TIMEOUT_DESC = "Timeout";
const std::string ASSESSMENT_EVENT_CODE_SYSTEM_ERROR_DESC = "System error";
const std::string ASSESSMENT_EVENT_CODE_ENV_ANOMALY_DESC = "Environment Anomaly";

const std::unordered_map<int32_t, std::string> ERR_CODE_TO_MSG_MAP = {
    { static_cast<int32_t>(AssessmentApiErrCode::ERR_PERMISSION_DENIED), ERR_PERMISSION_DENIED_DESC },
    { static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_PARAMS), ERR_INVALID_PARAMS_DESC },
    { static_cast<int32_t>(AssessmentApiErrCode::ERR_CAPABILITY_NOT_SUPPORT), ERR_CAPABILITY_NOT_SUPPORT_DESC },
    { static_cast<int32_t>(AssessmentApiErrCode::ERR_ASSESSMENT_ALREADY_ACTIVE), ERR_ASSESSMENT_ALREADY_ACTIVE_DESC },
    { static_cast<int32_t>(AssessmentApiErrCode::ERR_ASSESSMENT_NOT_ACTIVE), ERR_ASSESSMENT_NOT_ACTIVE_DESC },
    { static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_OPERATION), ERR_INVALID_OPERATION_DESC },
};

const std::unordered_map<AssessmentErrorCode, std::string> EVENT_CODE_TO_MSG_MAP = {
    { AssessmentErrorCode::OK, ASSESSMENT_EVENT_CODE_OK_DESC },
    { AssessmentErrorCode::USER_CANCEL, ASSESSMENT_EVENT_CODE_USER_CANCEL_DESC },
    { AssessmentErrorCode::TIMEOUT, ASSESSMENT_EVENT_CODE_TIMEOUT_DESC },
    { AssessmentErrorCode::SYSTEM_ERROR, ASSESSMENT_EVENT_CODE_SYSTEM_ERROR_DESC },
    { AssessmentErrorCode::ENV_ANOMALY, ASSESSMENT_EVENT_CODE_ENV_ANOMALY_DESC },
};
}

std::string AssessmentApiErrCodeToErrMsg(int32_t errCode)
{
    auto it = ERR_CODE_TO_MSG_MAP.find(errCode);
    if (it != ERR_CODE_TO_MSG_MAP.end()) {
        return it->second;
    }
    return ERR_INTERNAL_ERROR_DESC;
}

std::string AssessmentErrCodeToErrMsg(AssessmentErrorCode eventCode)
{
    auto it = EVENT_CODE_TO_MSG_MAP.find(eventCode);
    if (it != EVENT_CODE_TO_MSG_MAP.end()) {
        return it->second;
    }
    return ASSESSMENT_EVENT_CODE_SYSTEM_ERROR_DESC;
}
} // namespace AAFwk
} // namespace OHOS