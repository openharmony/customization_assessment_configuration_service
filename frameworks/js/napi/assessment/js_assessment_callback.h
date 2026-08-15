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

#ifndef OHOS_JS_ASSESSMENT_CALLBACK_H
#define OHOS_JS_ASSESSMENT_CALLBACK_H

#include <string>

#include "assessment_callback_code.h"
#include "hilog_tag_wrapper.h"
#include "js_runtime_utils.h"
#include "ipc_object_stub.h"
#include "message_option.h"
#include "message_parcel.h"

namespace OHOS {
namespace AAFwk {

class JsAssessmentCallback : public IPCObjectStub {
public:
    JsAssessmentCallback(napi_env env, napi_ref onBeginRef, napi_ref onInterruptedRef, napi_ref onEndRef);
    virtual ~JsAssessmentCallback();

    virtual int OnRemoteRequest(uint32_t code, MessageParcel& data,
                                MessageParcel& reply, MessageOption& option);

    napi_ref GetOnBeginRef() const;
    napi_ref GetOnInterruptedRef() const;
    napi_ref GetOnEndRef() const;

private:
    void CallJsCallback(napi_ref callbackRef, napi_value* argv, size_t argc, const std::string& callbackName);

    napi_env env_ = nullptr;
    napi_ref onBeginRef_ = nullptr;
    napi_ref onInterruptedRef_ = nullptr;
    napi_ref onEndRef_ = nullptr;
};

} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_JS_ASSESSMENT_CALLBACK_H