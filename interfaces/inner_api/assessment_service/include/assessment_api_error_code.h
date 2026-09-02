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

#ifndef OHOS_ASSESSMENT_API_ERROR_CODE_H
#define OHOS_ASSESSMENT_API_ERROR_CODE_H

#include <string>

namespace OHOS {
namespace AAFwk {

enum class AssessmentApiErrCode : int32_t {
    ERR_OK = 0,
    ERR_PERMISSION_DENIED = 201,
    ERR_INVALID_PARAMS = 401,
    ERR_CAPABILITY_NOT_SUPPORT = 801,

    ERR_INTERNAL_ERROR = 86600001,
    ERR_ASSESSMENT_ALREADY_ACTIVE = 86600002,
    ERR_INVALID_CONFIG = 86600003,
    ERR_ENV_DETECTION_FAILED = 86600004,
    ERR_ASSESSMENT_NOT_ACTIVED = 86600005,
    ERR_USER_CANCELLED = 86600006,
};

std::string AssessmentErrCodeToErrMsg(int32_t errCode);

} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_ASSESSMENT_API_ERROR_CODE_H