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
const std::string ERR_INTERNAL_ERROR_DESC = "Internal error";
const std::string ERR_ASSESSMENT_ALREADY_ACTIVE_DESC = "Assessment already active";
const std::string ERR_INVALID_CONFIGURATION_DESC = "Invalid configuration";
const std::string ERR_ENV_DETECTION_FAILED_DESC = "Environment check failed";
const std::string ERR_ASSESSMENT_NOT_ACTIVED_DESC = "Assessment not actived";
const std::string ERR_USER_CANCELLED_DESC = "User Cancelled";
}

std::string AssessmentErrCodeToErrMsg(int32_t errCode)
{
    switch (errCode) {
        case static_cast<int32_t>(AssessmentApiErrCode::ERR_PERMISSION_DENIED):
            return ERR_PERMISSION_DENIED_DESC;
        case static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_PARAMS):
            return ERR_INVALID_PARAMS_DESC;
        case static_cast<int32_t>(AssessmentApiErrCode::ERR_CAPABILITY_NOT_SUPPORT):
            return ERR_CAPABILITY_NOT_SUPPORT_DESC;
        case static_cast<int32_t>(AssessmentApiErrCode::ERR_ASSESSMENT_ALREADY_ACTIVE):
            return ERR_ASSESSMENT_ALREADY_ACTIVE_DESC;
        case static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_CONFIG):
            return ERR_INVALID_CONFIGURATION_DESC;
        case static_cast<int32_t>(AssessmentApiErrCode::ERR_ENV_DETECTION_FAILED):
            return ERR_ENV_DETECTION_FAILED_DESC;
        case static_cast<int32_t>(AssessmentApiErrCode::ERR_ASSESSMENT_NOT_ACTIVED):
            return ERR_ASSESSMENT_NOT_ACTIVED_DESC;
        case static_cast<int32_t>(AssessmentApiErrCode::ERR_USER_CANCELLED):
            return ERR_USER_CANCELLED_DESC;
        default:
            return ERR_INTERNAL_ERROR_DESC;
    }
}
} // namespace AAFwk
} // namespace OHOS