/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
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

#ifndef ANI_ASSESSMENT_UTILS_H
#define ANI_ASSESSMENT_UTILS_H

#include <string>
#include <vector>

#include "ani.h"
#include "assessment_api_error_code.h"

namespace OHOS {
namespace AAFwk {

ani_status GetAniString(ani_env* env, const std::string& str, ani_string* result);
ani_status CallAniFunctionVoid(ani_env* env, const char* ns, const char* fn, ani_ref callback);
ani_status CallAniFunctionVoidWithIntString(ani_env* env,
                                            const char* ns,
                                            const char* fn,
                                            ani_ref callback,
                                            ani_int code,
                                            const std::string& strParam);
ani_object CreateAniUndefined(ani_env* env);
ani_status GetStdString(ani_env* env, ani_string ani_str, std::string& result);
ani_status CreateBusinessError(ani_env* env, int32_t error, const std::string& message, ani_object* err);
ani_status ThrowAniError(ani_env* env, AAFwk::AssessmentApiErrCode errCode, const std::string& message = "");
void* GetContextPtr(ani_env* env, ani_object contextObj);
uint32_t GetUint32FromJs(ani_env* env, ani_object configObj, const char* fieldName);
ani_status GetStringArrayFromJs(ani_env* env,
                                ani_object configObj,
                                const char* fieldName,
                                std::vector<std::string>& result);
ani_object CreateEtsObject(ani_env* env, const char* className);

} // namespace AAFwk
} // namespace OHOS
#endif // ANI_ASSESSMENT_UTILS_H
