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

struct OnBeginCallbackParam {
    JsAssessmentCallback *callback = nullptr;
    int32_t code = 0;
    std::string message;
    napi_async_work work = nullptr;
};

struct OnInterruptedCallbackParam {
    JsAssessmentCallback *callback = nullptr;
    int32_t reason = 0;
    std::string message;
    napi_async_work work = nullptr;
};

struct OnEndCallbackParam {
    JsAssessmentCallback *callback = nullptr;
    napi_async_work work = nullptr;
};

void ExecuteOnBeginCallback(napi_env env, napi_status status, void* data)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment onBegin executeCallback called");
    OnBeginCallbackParam *callbackParam = static_cast<OnBeginCallbackParam*>(data);
    JsAssessmentCallback *callback = callbackParam->callback;
    callback->CallJsOnBegin(callbackParam->code, callbackParam->message);
    napi_delete_async_work(env, callbackParam->work);
    delete callbackParam;
}

void ExecuteOnInterruptedCallback(napi_env env, napi_status status, void* data)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment onInterrupted executeCallback called");
    OnInterruptedCallbackParam *callbackParam = static_cast<OnInterruptedCallbackParam*>(data);
    JsAssessmentCallback *callback = callbackParam->callback;
    callback->CallJsOnInterrupted(callbackParam->reason, callbackParam->message);
    napi_delete_async_work(env, callbackParam->work);
    delete callbackParam;
}

void ExecuteOnEndCallback(napi_env env, napi_status status, void* data)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment onEnd executeCallback called");
    OnEndCallbackParam *callbackParam = static_cast<OnEndCallbackParam*>(data);
    JsAssessmentCallback *callback = callbackParam->callback;
    callback->CallJsOnEnd();
    napi_delete_async_work(env, callbackParam->work);
    delete callbackParam;
}

JsAssessmentCallback::JsAssessmentCallback(napi_env env, napi_ref onBeginRef,
                                           napi_ref onInterruptedRef, napi_ref onEndRef)
    : IPCObjectStub(u"JsAssessmentCallback", false), env_(env), onBeginRef_(onBeginRef),
      onInterruptedRef_(onInterruptedRef), onEndRef_(onEndRef)
{
    TAG_LOGD(AAFwkTag::DEFAULT, "constructor");
}

JsAssessmentCallback::~JsAssessmentCallback()
{
    TAG_LOGD(AAFwkTag::DEFAULT, "destructor");
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
    OnBeginCallbackParam *params = new (std::nothrow) OnBeginCallbackParam {
        .callback = this,
        .code = code,
        .message = message,
        .work = nullptr
    };
    if (params == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "malloc fail");
        return;
    }
    napi_value resourceName;
    napi_create_string_utf8(env_, "ExecuteOnBeginCallback", NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(
        env_, nullptr, resourceName,
        [](napi_env env, void* data) { TAG_LOGD(AAFwkTag::DEFAULT, "assessment onBegin complete called"); },
        ExecuteOnBeginCallback,
        params,
        &params->work);
    napi_queue_async_work(env_, params->work);
}

void JsAssessmentCallback::PostOnInterrupted(int32_t reason, const std::string &message)
{
    OnInterruptedCallbackParam *params = new (std::nothrow) OnInterruptedCallbackParam {
        .callback = this,
        .reason = reason,
        .message = message,
        .work = nullptr
    };
    if (params == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "malloc fail");
        return;
    }
    napi_value resourceName;
    napi_create_string_utf8(env_, "ExecuteOnInterruptedCallback", NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(
        env_, nullptr, resourceName,
        [](napi_env env, void* data) { TAG_LOGD(AAFwkTag::DEFAULT, "assessment onInterrupted complete called"); },
        ExecuteOnInterruptedCallback,
        params,
        &params->work);
    napi_queue_async_work(env_, params->work);
}

void JsAssessmentCallback::PostOnEnd()
{
    OnEndCallbackParam *params = new (std::nothrow) OnEndCallbackParam {
        .callback = this
    };
    if (params == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "malloc fail");
        return;
    }
    napi_value resourceName;
    napi_create_string_utf8(env_, "ExecuteOnEndCallback", NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(
        env_, nullptr, resourceName,
        [](napi_env env, void* data) { TAG_LOGD(AAFwkTag::DEFAULT, "assessment onEnd complete called"); },
        ExecuteOnEndCallback,
        params,
        &params->work);
    napi_queue_async_work(env_, params->work);
}

void JsAssessmentCallback::CallJsOnBegin(int32_t code, const std::string &message)
{
    napi_value errorObj = nullptr;
    napi_create_object(env_, &errorObj);
    napi_set_named_property(env_, errorObj, "code", CreateJsValue(env_, code));
    napi_set_named_property(env_, errorObj, "message", CreateJsValue(env_, message));
    napi_value argv[] = { errorObj };
    CallJsCallback(onBeginRef_, argv, 1, ON_BEGIN);
}

void JsAssessmentCallback::CallJsOnInterrupted(int32_t reason, const std::string &message)
{
    napi_value infoObj = nullptr;
    napi_create_object(env_, &infoObj);
    napi_set_named_property(env_, infoObj, "reason", CreateJsValue(env_, reason));
    napi_set_named_property(env_, infoObj, "message", CreateJsValue(env_, message));
    napi_value argv[] = { infoObj };
    CallJsCallback(onInterruptedRef_, argv, 1, ON_INTERRUPTED);
}

void JsAssessmentCallback::CallJsOnEnd()
{
    napi_value argv[] = {};
    CallJsCallback(onEndRef_, argv, 0, ON_END);
}
} // namespace AAFwk
} // namespace OHOS