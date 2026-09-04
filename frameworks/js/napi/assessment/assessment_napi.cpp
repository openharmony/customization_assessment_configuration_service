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

#include "assessment_napi.h"

#include "assessment_api_error_code.h"
#include "assessment_service_client.h"
#include "hilog_tag_wrapper.h"
#include "js_assessment_callback.h"
#include "napi/native_node_api.h"
#include "napi_common_util.h"
#include "js_runtime_utils.h"
#include "napi_base_context.h"
#include "errors.h"

namespace OHOS {
namespace AbilityRuntime {
namespace {
constexpr size_t ARGC_THREE = 3;
constexpr size_t ARGC_ONE = 1;
constexpr size_t INDEX_ZERO = 0;
constexpr size_t INDEX_ONE = 1;
constexpr size_t INDEX_TWO = 2;
constexpr size_t MAX_ALLOWED_APPS_COUNT = 10;
}

static napi_value GetNapiUndefined(napi_env env)
{
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value CreateError(napi_env env, int32_t err, const std::string& msg)
{
    napi_value businessError = nullptr;
    napi_value errorCode = nullptr;
    NAPI_CALL(env, napi_create_int32(env, err, &errorCode));
    napi_value errorMessage = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, msg.c_str(), NAPI_AUTO_LENGTH, &errorMessage));
    napi_create_error(env, nullptr, errorMessage, &businessError);
    napi_set_named_property(env, businessError, "code", errorCode);
    return businessError;
}

static napi_value ThrowError(napi_env env, int32_t err, const std::string &msg)
{
    napi_value error = CreateError(env, err, msg);
    napi_throw(env, error);
    return GetNapiUndefined(env);
}

static napi_value ThrowError(napi_env env, OHOS::AAFwk::AssessmentApiErrCode errCode)
{
    return ThrowError(env, static_cast<int32_t>(errCode),
        OHOS::AAFwk::AssessmentErrCodeToErrMsg(static_cast<int32_t>(errCode)));
}

static bool NapiIsCallable(napi_env env, napi_value value)
{
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, value, &valueType) != napi_ok) {
        return false;
    }
    return valueType == napi_function;
}

static napi_value GetUint32FromJs(napi_env env, napi_value value, uint32_t &result)
{
    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, value, &valueType);
    if (valueType != napi_number) {
        TAG_LOGE(AAFwkTag::DEFAULT, "value is not number");
        return nullptr;
    }

    napi_get_value_uint32(env, value, &result);
    return value;
}

static napi_value GetStringArrayFromJs(napi_env env, napi_value value, std::vector<std::string> &result)
{
    bool isArray = false;
    napi_is_array(env, value, &isArray);
    if (!isArray) {
        TAG_LOGE(AAFwkTag::DEFAULT, "value is not array");
        return nullptr;
    }

    uint32_t length = 0;
    napi_get_array_length(env, value, &length);

    for (uint32_t i = 0; i < length; i++) {
        napi_value item = nullptr;
        napi_get_element(env, value, i, &item);

        napi_valuetype itemType = napi_undefined;
        napi_typeof(env, item, &itemType);
        if (itemType != napi_string) {
            continue;
        }

        size_t strLength = 0;
        napi_get_value_string_utf8(env, item, nullptr, 0, &strLength);

        std::vector<char> buffer(strLength + 1);
        napi_get_value_string_utf8(env, item, buffer.data(), strLength + 1, &strLength);
        result.push_back(buffer.data());
    }

    return value;
}

sptr<AAFwk::JsAssessmentCallback> CreateJsAssessmentCallback(napi_env env, napi_value callbackValue)
{
    sptr<AAFwk::JsAssessmentCallback> jsCallback = nullptr;
    if (callbackValue == nullptr) {
        return jsCallback;
    }
    napi_valuetype valueType = napi_valuetype::napi_undefined;
    napi_typeof(env, callbackValue, &valueType);
    if (valueType == napi_valuetype::napi_object) {
        napi_ref onBeginRef = nullptr;
        napi_ref onInterruptedRef = nullptr;
        napi_ref onEndRef = nullptr;

        napi_value onBeginValue = nullptr;
        napi_get_named_property(env, callbackValue, "onBegin", &onBeginValue);
        if (onBeginValue != nullptr && NapiIsCallable(env, onBeginValue)) {
            napi_create_reference(env, onBeginValue, 1, &onBeginRef);
        }

        napi_value onInterruptedValue = nullptr;
        napi_get_named_property(env, callbackValue, "onInterrupted", &onInterruptedValue);
        if (onInterruptedValue != nullptr && NapiIsCallable(env, onInterruptedValue)) {
            napi_create_reference(env, onInterruptedValue, 1, &onInterruptedRef);
        }

        napi_value onEndValue = nullptr;
        napi_get_named_property(env, callbackValue, "onEnd", &onEndValue);
        if (onEndValue != nullptr && NapiIsCallable(env, onEndValue)) {
            napi_create_reference(env, onEndValue, 1, &onEndRef);
        }

        if (onBeginRef != nullptr || onInterruptedRef != nullptr || onEndRef != nullptr) {
            jsCallback = new (std::nothrow) AAFwk::JsAssessmentCallback(
                env, onBeginRef, onInterruptedRef, onEndRef);
            TAG_LOGI(AAFwkTag::DEFAULT, "jsCallback created");
        }
    }
    return jsCallback;
}

napi_value AssessmentNapiBegin(napi_env env, napi_callback_info info)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "AssessmentNapiBegin called");

    size_t argc = ARGC_THREE;
    napi_value argv[ARGC_THREE] = {nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (argc < ARGC_THREE) {
        TAG_LOGE(AAFwkTag::DEFAULT, "missing parameters");
        return ThrowError(env, OHOS::AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS);
    }

    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, argv[INDEX_ZERO]);
    if (context == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "null context");
        return ThrowError(env, OHOS::AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS);
    }
    auto uiAbilityContext = AbilityRuntime::Context::ConvertTo<AbilityRuntime::AbilityContext>(context);
    if (uiAbilityContext == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "null UIAbilityContext");
        return ThrowError(env, OHOS::AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS);
    }
    auto token = uiAbilityContext->GetToken();
    napi_value configValue = argv[INDEX_ONE];
    napi_valuetype valueType = napi_valuetype::napi_undefined;
    napi_typeof(env, configValue, &valueType);
    if (valueType != napi_valuetype::napi_object) {
        TAG_LOGE(AAFwkTag::DEFAULT, "config is not object");
        return ThrowError(env, OHOS::AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS);
    }

    uint32_t duration = 0;
    napi_value durationValue = nullptr;
    napi_get_named_property(env, configValue, "duration", &durationValue);
    if (durationValue != nullptr) {
        GetUint32FromJs(env, durationValue, duration);
    }

    std::vector<std::string> allowedApps;
    napi_value allowedAppsValue = nullptr;
    napi_get_named_property(env, configValue, "allowedApps", &allowedAppsValue);
    if (allowedAppsValue != nullptr) {
        GetStringArrayFromJs(env, allowedAppsValue, allowedApps);
    }
    std::string bundleName = uiAbilityContext->GetBundleName();
    TAG_LOGD(AAFwkTag::ASSESSMENT, "assessment bundleName: %{public}s", bundleName.c_str());
    if (std::find(allowedApps.begin(), allowedApps.end(), bundleName) == allowedApps.end()) {
        allowedApps.push_back(bundleName);
    }

    napi_value callbackValue = argv[INDEX_TWO];
    sptr<AAFwk::JsAssessmentCallback> jsCallback = CreateJsAssessmentCallback(env, callbackValue);
    if (jsCallback == nullptr) {
        return ThrowError(env, OHOS::AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS);
    }

    TAG_LOGI(AAFwkTag::DEFAULT, "duration: %{public}d, allowedApps size: %{public}zu",
        duration, allowedApps.size());

    if (allowedApps.size() > MAX_ALLOWED_APPS_COUNT) {
        TAG_LOGE(AAFwkTag::DEFAULT, "too many allowedApps, size: %{public}zu, max: %{public}zu",
            allowedApps.size(), MAX_ALLOWED_APPS_COUNT);
        return ThrowError(env, OHOS::AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS);
    }

    sptr<IRemoteObject> callbackObj = jsCallback;
    ErrCode ret = OHOS::AAFwk::AssessmentServiceClient::GetInstance()->Begin(token, duration, allowedApps, callbackObj);
    if (ret != ERR_OK) {
        return ThrowError(env, ret, OHOS::AAFwk::AssessmentErrCodeToErrMsg(ret));
    }

    return GetNapiUndefined(env);
}

napi_value AssessmentNapiEnd(napi_env env, napi_callback_info info)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "AssessmentNapiEnd called");

    size_t argc = ARGC_ONE;
    napi_value argv[ARGC_ONE] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (argc < ARGC_ONE) {
        TAG_LOGE(AAFwkTag::DEFAULT, "missing parameters");
        return ThrowError(env, OHOS::AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS);
    }

    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, argv[INDEX_ZERO]);
    if (context == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "null context");
        return ThrowError(env, OHOS::AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS);
    }
    auto uiAbilityContext = AbilityRuntime::Context::ConvertTo<AbilityRuntime::AbilityContext>(context);
    if (uiAbilityContext == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "null UIAbilityContext");
        return ThrowError(env, OHOS::AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS);
    }
    auto token = uiAbilityContext->GetToken();

    ErrCode ret = OHOS::AAFwk::AssessmentServiceClient::GetInstance()->End(token);
    if (ret != ERR_OK) {
        return ThrowError(env, ret, OHOS::AAFwk::AssessmentErrCodeToErrMsg(ret));
    }

    return GetNapiUndefined(env);
}

napi_value AssessmentNapiIsActive(napi_env env, napi_callback_info info)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "AssessmentNapiIsActive called");

    size_t argc = ARGC_ONE;
    napi_value argv[ARGC_ONE] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    bool isActive = false;
    ErrCode ret = OHOS::AAFwk::AssessmentServiceClient::GetInstance()->IsActive(isActive);
    if (ret != ERR_OK) {
        return ThrowError(env, ret, OHOS::AAFwk::AssessmentErrCodeToErrMsg(ret));
    }

    TAG_LOGI(AAFwkTag::DEFAULT, "IsActive result: %{public}d", isActive);

    napi_value result = nullptr;
    napi_get_boolean(env, isActive, &result);
    return result;
}

napi_value AssessmentNapiGetConfiguration(napi_env env, napi_callback_info info)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "AssessmentNapiGetConfiguration called");

    napi_value result = nullptr;
    napi_create_object(env, &result);

    uint32_t duration = 0;
    std::vector<std::string> allowedApps;
    ErrCode ret = OHOS::AAFwk::AssessmentServiceClient::GetInstance()->GetConfiguration(duration, allowedApps);
    if (ret != ERR_OK) {
        return ThrowError(env, ret, OHOS::AAFwk::AssessmentErrCodeToErrMsg(ret));
    }
    napi_set_named_property(env, result, "duration", CreateJsValue(env, duration));

    napi_value jsArray = nullptr;
    napi_create_array(env, &jsArray);
    for (size_t i = 0; i < allowedApps.size(); i++) {
        napi_set_element(env, jsArray, i, CreateJsValue(env, allowedApps[i]));
    }
    napi_set_named_property(env, result, "allowedApps", jsArray);

    return result;
}

EXTERN_C_START
static napi_value Export(napi_env env, napi_value exports)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Export called");

    napi_property_descriptor descriptors[] = {
        DECLARE_NAPI_FUNCTION("begin", AssessmentNapiBegin),
        DECLARE_NAPI_FUNCTION("end", AssessmentNapiEnd),
        DECLARE_NAPI_FUNCTION("isActive", AssessmentNapiIsActive),
        DECLARE_NAPI_FUNCTION("getConfiguration", AssessmentNapiGetConfiguration),
    };

    napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    return exports;
}
EXTERN_C_END

static napi_module module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Export,
    .nm_modname = "assessment",
    .nm_priv = nullptr,
    .reserved = {nullptr},
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&module);
}
} // namespace AbilityRuntime
} // namespace OHOS