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

#include "ani_assessment_utils.h"

#include "hilog_tag_wrapper.h"


namespace OHOS {
namespace AAFwk {

ani_status GetAniString(ani_env* env, const std::string& str, ani_string* result)
{
    return env->String_NewUTF8(str.c_str(), static_cast<ani_size>(str.size()), result);
}

ani_status CallAniFunctionVoid(ani_env* env, const char* ns, const char* fn, ani_ref callback)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "CallAniFunctionVoid start");
    ani_namespace aniNamespace{};
    ani_status ret = env->FindNamespace(ns, &aniNamespace);
    if (ret != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "cannot find namespace %{public}s", ns);
        return ret;
    }
    ani_function func{};
    ret = env->Namespace_FindFunction(aniNamespace, fn, nullptr, &func);
    if (ret != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "cannot find function %{public}s", fn);
        return ret;
    }
    ret = env->Function_Call_Void(func, callback);
    if (ret != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "cannot run callback %{public}s", fn);
        return ret;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "CallAniFunctionVoid finish");
    return ANI_OK;
}

ani_status CallAniFunctionVoidWithIntString(ani_env* env,  // NOLINT(readability-function-size)
                                            const char* ns,
                                            const char* fn,
                                            ani_ref callback,
                                            ani_int code,
                                            const std::string& str)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "CallAniFunctionVoidWithIntString start");
    ani_namespace aniNamespace{};
    ani_status ret = env->FindNamespace(ns, &aniNamespace);
    if (ret != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "cannot find namespace %{public}s", ns);
        return ret;
    }
    ani_function func{};
    ret = env->Namespace_FindFunction(aniNamespace, fn, nullptr, &func);
    if (ret != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "cannot find function %{public}s", fn);
        return ret;
    }
    ani_string aniStr;
    if ((ret = GetAniString(env, str, &aniStr)) != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "cannot create ani string");
        return ret;
    }
    ret = env->Function_Call_Void(func, callback, code, aniStr);
    if (ret != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "cannot run callback with string %{public}s", fn);
        return ret;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "CallAniFunctionVoidWithIntString finish");
    return ANI_OK;
}

ani_object CreateAniUndefined(ani_env* env)
{
    ani_ref aniRef;
    env->GetUndefined(&aniRef);
    return static_cast<ani_object>(aniRef);
}

ani_status GetStdString(ani_env* env, ani_string ani_str, std::string& result)
{
    ani_size strSize;
    ani_status ret = env->String_GetUTF8Size(ani_str, &strSize);
    if (ret != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "GetStdString, String_GetUTF8Size failed");
        return ret;
    }
    std::vector<char> buffer(strSize + 1);
    char* utf8Buffer = buffer.data();
    ani_size bytes_written = 0;
    ret = env->String_GetUTF8(ani_str, utf8Buffer, strSize + 1, &bytes_written);
    if (ret != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "GetStdString, String_GetUTF8 failed");
        return ret;
    }
    utf8Buffer[bytes_written] = '\0';
    result = std::string(utf8Buffer);
    return ret;
}

ani_status CreateBusinessError(ani_env* env, int32_t error, const std::string& message, ani_object* err)
{
    ani_class aniClass = nullptr;
    ani_status status = env->FindClass("@ohos.base.BusinessError", &aniClass);
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "BusinessError class not found, status:%{public}d", static_cast<int32_t>(status));
        return status;
    }
    ani_method aniCtor = nullptr;
    status = env->Class_FindMethod(aniClass, "<ctor>", "C{std.core.String}C{escompat.ErrorOptions}:", &aniCtor);
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "BusinessError ctor not found, status:%{public}d", static_cast<int32_t>(status));
        return status;
    }
    ani_string aniMsg;
    GetAniString(env, message, &aniMsg);
    status = env->Object_New(aniClass, aniCtor, err, aniMsg, CreateAniUndefined(env));
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "failed to new error, status:%{public}d", static_cast<int32_t>(status));
        return status;
    }
    status = env->Object_SetFieldByName_Int(*err, "code_", static_cast<ani_int>(error));
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "failed to set error code, status:%{public}d", static_cast<int32_t>(status));
        return status;
    }
    return ANI_OK;
}

ani_status ThrowAniError(ani_env* env, AAFwk::AssessmentApiErrCode errCode, const std::string& message)
{
    std::string errorMessage = message.empty() ?
                               AAFwk::AssessmentApiErrCodeToErrMsg(static_cast<int32_t>(errCode)) : message;
    ani_object aniError;
    ani_status status = CreateBusinessError(env, static_cast<int32_t>(errCode), errorMessage, &aniError);
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "failed to new aniError, status:%{public}d", static_cast<int32_t>(status));
        return status;
    }
    status = env->ThrowError(static_cast<ani_error>(aniError));
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "failed to ThrowAniError, status:%{public}d", static_cast<int32_t>(status));
    }
    return status;
}

void* GetContextPtr(ani_env* env, ani_object contextObj)
{
    ani_long nativeContextLong = 0;
    ani_class cls = nullptr;
    ani_field contextField = nullptr;
    ani_status status = ANI_ERROR;
    if ((status = env->FindClass("application.UIAbilityContext.UIAbilityContext", &cls)) != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "GetContextPtr, find class fail, status : %{public}d", status);
        return nullptr;
    }
    if ((status = env->Class_FindField(cls, "nativeContext", &contextField)) != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "GetContextPtr, find field fail, status : %{public}d", status);
        return nullptr;
    }
    if ((status = env->Object_GetField_Long(contextObj, contextField, &nativeContextLong)) != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "GetContextPtr, get field fail, status : %{public}d", status);
        return nullptr;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "GetContextPtr success");
    return reinterpret_cast<void*>(nativeContextLong);
}

uint32_t GetUint32FromJs(ani_env* env, ani_object configObj, const char* fieldName)
{
    ani_int intValue = 0;
    ani_status status = env->Object_GetPropertyByName_Int(configObj, fieldName, &intValue);
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT,
                 "GetUint32FromJs, get %{public}s int failed, status: %{public}d", fieldName, status);
        return 0;
    }
    return static_cast<uint32_t>(intValue);
}

ani_status GetStringArrayFromJs(ani_env* env,
                                ani_object configObj,
                                const char* fieldName,
                                std::vector<std::string>& result)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "GetStringArrayFromJs start");
    ani_ref fieldValue = nullptr;
    ani_status status = env->Object_GetPropertyByName_Ref(configObj, fieldName, &fieldValue);
    if (status != ANI_OK || fieldValue == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "get %{public}s failed", fieldName);
        return ANI_OK;
    }
    ani_boolean isUndefined = ANI_FALSE;
    env->Reference_IsUndefined(fieldValue, &isUndefined);
    if (isUndefined) {
        TAG_LOGW(AAFwkTag::DEFAULT, "fieldValue %{public}s is undefined", fieldName);
        return ANI_OK;
    }
    ani_object arrayObj = static_cast<ani_object>(fieldValue);
    ani_array array = reinterpret_cast<ani_array>(arrayObj);
    ani_size length = 0;
    status = env->Array_GetLength(array, &length);
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "GetStringArrayFromJs, get array length failed, status: %{public}d", status);
        return status;
    }
    for (ani_size i = 0; i < length; i++) {
        ani_ref itemRef = nullptr;
        status = env->Array_Get(array, i, &itemRef);
        if (status != ANI_OK || itemRef == nullptr) {
            continue;
        }
        ani_string aniStr = reinterpret_cast<ani_string>(itemRef);
        std::string strValue;
        if (GetStdString(env, aniStr, strValue) == ANI_OK) {
            result.push_back(strValue);
        }
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "GetStringArrayFromJs finish");
    return ANI_OK;
}

ani_object CreateEtsObject(ani_env* env, const char* className)
{
    ani_class cls = nullptr;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "failed to find class %{public}s, status:%{public}d", className, status);
        return nullptr;
    }

    ani_method ctor = nullptr;
    status = env->Class_FindMethod(cls, "<ctor>", ":", &ctor);
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "get default ctor failed, status:%{public}d", status);
        return nullptr;
    }

    ani_object configObj = nullptr;
    if ((status = env->Object_New(cls, ctor, &configObj)) != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "failed to create configObj, status:%{public}d", status);
        return nullptr;
    }

    return configObj;
}

} // namespace AAFwk
} // namespace OHOS
