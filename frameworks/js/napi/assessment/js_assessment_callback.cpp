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

#include "js_assessment_callback.h"

namespace OHOS {
namespace AAFwk {
using namespace AbilityRuntime;

namespace {
constexpr const char* ON_BEGIN = "OnBegin";
constexpr const char* ON_INTERRUPTED = "OnInterrupted";
constexpr const char* ON_END = "OnEnd";
}

JsAssessmentCallback::JsAssessmentCallback(napi_env env, napi_ref onBeginRef,
                                           napi_ref onInterruptedRef, napi_ref onEndRef)
    : IPCObjectStub(u"JsAssessmentCallback", false), env_(env), onBeginRef_(onBeginRef),
      onInterruptedRef_(onInterruptedRef), onEndRef_(onEndRef)
{
    TAG_LOGD(AAFwkTag::DEFAULT, "constructor");
    auto mainRunner = AppExecFwk::EventRunner::GetMainEventRunner();
    if (mainRunner == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "JsAssessmentCallback, GetMainEventRunner failed");
        return;
    }
    eventHandler_ = std::make_shared<AppExecFwk::EventHandler>(mainRunner);
    TAG_LOGI(AAFwkTag::DEFAULT, "eventHandler_ created successfully");
}

JsAssessmentCallback::~JsAssessmentCallback()
{
    TAG_LOGD(AAFwkTag::DEFAULT, "destructor");
    if (env_ == nullptr) {
        return;
    }
    napi_env env = env_;
    napi_ref onBeginRef = onBeginRef_;
    napi_ref onInterruptedRef = onInterruptedRef_;
    napi_ref onEndRef = onEndRef_;
    onBeginRef_ = nullptr;
    onInterruptedRef_ = nullptr;
    onEndRef_ = nullptr;

    if (eventHandler_ != nullptr) {
        auto task = [env, onBeginRef, onInterruptedRef, onEndRef]() {
            if (onBeginRef != nullptr) {
                napi_delete_reference(env, onBeginRef);
            }
            if (onInterruptedRef != nullptr) {
                napi_delete_reference(env, onInterruptedRef);
            }
            if (onEndRef != nullptr) {
                napi_delete_reference(env, onEndRef);
            }
        };
        eventHandler_->PostTask(task, "napi_delete_ref", 0, AppExecFwk::EventQueue::Priority::IMMEDIATE);
    } else {
        TAG_LOGW(AAFwkTag::DEFAULT, "eventHandler_ is null, napi_ref may leak");
    }
}

napi_ref JsAssessmentCallback::GetOnBeginRef() const
{
    return onBeginRef_;
}

napi_ref JsAssessmentCallback::GetOnInterruptedRef() const
{
    return onInterruptedRef_;
}

napi_ref JsAssessmentCallback::GetOnEndRef() const
{
    return onEndRef_;
}

int JsAssessmentCallback::OnRemoteRequest(uint32_t code, MessageParcel& data,
                                          MessageParcel& reply, MessageOption& option)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "OnRemoteRequest code: %{public}d", code);

    switch (static_cast<AssessmentCallbackCode>(code)) {
        case AssessmentCallbackCode::ON_BEGIN: {
            int32_t jsCode = data.ReadInt32();
            std::string message = data.ReadString();
            this->PostOnBegin(jsCode, message);
            break;
        }
        case AssessmentCallbackCode::ON_INTERRUPTED: {
            int32_t reason = data.ReadInt32();
            std::string message = data.ReadString();
            this->PostOnInterrupted(reason, message);
            break;
        }
        case AssessmentCallbackCode::ON_END: {
            this->PostOnEnd();
            break;
        }
        default:
            TAG_LOGW(AAFwkTag::DEFAULT, "unknown code: %{public}d", code);
            return -1;
    }
    return 0;
}

void JsAssessmentCallback::CallJsCallback(napi_ref callbackRef, napi_value* argv,
                                          size_t argc, const std::string& callbackName)
{
    if (callbackRef == nullptr) {
        TAG_LOGD(AAFwkTag::DEFAULT, "js callback ref is null, callbackName: %{public}s", callbackName.c_str());
        return;
    }

    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env_, callbackRef, &jsCallback);
    if (status != napi_ok || jsCallback == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "get reference value failed or js callback is null, callbackName: %{public}s",
            callbackName.c_str());
        return;
    }

    napi_value result = nullptr;
    napi_value callResult = nullptr;
    napi_get_undefined(env_, &result);
    napi_call_function(env_, result, jsCallback, argc, argv, &callResult);
    TAG_LOGD(AAFwkTag::DEFAULT, "called, callbackName: %{public}s", callbackName.c_str());
}

void JsAssessmentCallback::PostOnBegin(int32_t code, const std::string &message)
{
    if (eventHandler_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "eventHandler_ is nullptr, cannot post CallJsOnBegin to main thread");
        return;
    }
    auto task = [selfRef = sptr<JsAssessmentCallback>(this), code, message]() {
        if (selfRef == nullptr) {
            TAG_LOGE(AAFwkTag::DEFAULT, "PostOnBegin task, this JsAssessmentCallback object is nullptr");
            return;
        }
        selfRef->CallJsOnBegin(code, message);
    };
    eventHandler_->PostTask(task, "napi_onBegin", 0, AppExecFwk::EventQueue::Priority::IMMEDIATE);
}

void JsAssessmentCallback::PostOnInterrupted(int32_t reason, const std::string &message)
{
    if (eventHandler_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "eventHandler_ is nullptr, cannot post CallJsOnInterrupted to main thread");
        return;
    }
    auto task = [selfRef = sptr<JsAssessmentCallback>(this), reason, message]() {
        if (selfRef == nullptr) {
            TAG_LOGE(AAFwkTag::DEFAULT, "PostOnInterrupted task, this JsAssessmentCallback object is nullptr");
            return;
        }
        selfRef->CallJsOnInterrupted(reason, message);
    };
    eventHandler_->PostTask(task, "napi_onInterrupted", 0, AppExecFwk::EventQueue::Priority::IMMEDIATE);
}

void JsAssessmentCallback::PostOnEnd()
{
    if (eventHandler_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "eventHandler_ is nullptr, cannot post CallJsOnEnd to main thread");
        return;
    }
    auto task = [selfRef = sptr<JsAssessmentCallback>(this)]() {
        if (selfRef == nullptr) {
            TAG_LOGE(AAFwkTag::DEFAULT, "PostOnEnd task, this JsAssessmentCallback object is nullptr");
            return;
        }
        selfRef->CallJsOnEnd();
    };
    eventHandler_->PostTask(task, "napi_onEnd", 0, AppExecFwk::EventQueue::Priority::IMMEDIATE);
}

void JsAssessmentCallback::CallJsOnBegin(int32_t code, const std::string &message)
{
    TAG_LOGI(AAFwkTag::DEFAULT,
        "CallJsOnBegin in, code: %{public}d, message: %{public}s", code, message.c_str());
    napi_value errorObj = nullptr;
    napi_create_object(env_, &errorObj);
    napi_set_named_property(env_, errorObj, "code", CreateJsValue(env_, code));
    napi_set_named_property(env_, errorObj, "message", CreateJsValue(env_, message));
    napi_value argv[] = { errorObj };
    CallJsCallback(onBeginRef_, argv, 1, ON_BEGIN);
}

void JsAssessmentCallback::CallJsOnInterrupted(int32_t code, const std::string &message)
{
    TAG_LOGI(AAFwkTag::DEFAULT,
        "CallJsOnInterrupted in, code: %{public}d, message: %{public}s", code, message.c_str());
    napi_value infoObj = nullptr;
    napi_create_object(env_, &infoObj);
    napi_set_named_property(env_, infoObj, "code", CreateJsValue(env_, code));
    napi_set_named_property(env_, infoObj, "message", CreateJsValue(env_, message));
    napi_value argv[] = { infoObj };
    CallJsCallback(onInterruptedRef_, argv, 1, ON_INTERRUPTED);
}

void JsAssessmentCallback::CallJsOnEnd()
{
    TAG_LOGI(AAFwkTag::DEFAULT, "CallJsOnEnd in");
    napi_value argv[] = {};
    CallJsCallback(onEndRef_, argv, 0, ON_END);
}
} // namespace AAFwk
} // namespace OHOS