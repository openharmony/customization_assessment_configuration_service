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

#ifndef OHOS_ASSESSMENT_NAPI_H
#define OHOS_ASSESSMENT_NAPI_H

#include "js_native_api.h"
#include <node_api.h>

namespace OHOS {
namespace AbilityRuntime {

napi_value AssessmentNapiBegin(napi_env env, napi_callback_info info);
napi_value AssessmentNapiEnd(napi_env env, napi_callback_info info);
napi_value AssessmentNapiIsActive(napi_env env, napi_callback_info info);
napi_value AssessmentNapiGetConfiguration(napi_env env, napi_callback_info info);

} // namespace AbilityRuntime
} // namespace OHOS
#endif // OHOS_ASSESSMENT_NAPI_H