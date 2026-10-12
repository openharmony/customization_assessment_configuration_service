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

#include "ani_assessment.h"

#include <ability_context.h>
#include "hilog_tag_wrapper.h"
#include "assessment_service_client.h"
#include "assessment_api_error_code.h"
#include "ani_assessment_utils.h"

namespace OHOS {
namespace AbilityRuntime {
namespace {
    constexpr uint32_t MAX_ALLOWED_APPS_COUNT = 10;
    const char* CLASS_NAME_ASSESSMENTCONFIG = "@ohos.customization.assessment.assessment.AssessmentConfigInner";
} // namespace

AniAssessment::AniAssessment() { TAG_LOGI(AAFwkTag::ASSESSMENT, "AniAssessment constructor"); }
AniAssessment::~AniAssessment() { TAG_LOGI(AAFwkTag::ASSESSMENT, "AniAssessment destructor"); }

ani_status AniAssessment::Init(ani_env* env, ani_namespace nsp)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "AniAssessment Init, start");
    if (env == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AniAssessment Init, null env");
        return ANI_ERROR;
    }
    ani_function setObjFunc = nullptr;
    ani_status ret = env->Namespace_FindFunction(nsp, "setNativeObj", "l:", &setObjFunc);
    if (ret != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AniAssessment Init, find setNativeObj func failed %{public}u", ret);
        return ret;
    }
    std::unique_ptr<AniAssessment> aniAssessment = std::make_unique<AniAssessment>();
    if (aniAssessment == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AniAssessment AniAssessment::Init, aniAssessment is nullptr");
        return ANI_ERROR;
    }
    AniAssessment* releasedRef = aniAssessment.release();
    ret = env->Function_Call_Void(setObjFunc, releasedRef);
    if (ret != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AniAssessment Init, call setNativeObj func failed %{public}u", ret);
        delete releasedRef;
        return ret;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "AniAssessment AniAssessment::Init, finish");
    return ret;
}

ani_boolean AniAssessment::IsActiveAni(ani_env* env)
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "IsActiveAni called");
    bool isActive = false;
    ErrCode ret = AAFwk::AssessmentServiceClient::GetInstance()->IsActive(isActive);
    if (ret != ERR_OK) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "IsActive failed, ret: %{public}d", ret);
        return ANI_FALSE;
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "IsActive finish, result: %{public}d", isActive);
    return isActive ? ANI_TRUE : ANI_FALSE;
}

ani_object AniAssessment::GetConfigurationAni(ani_env* env)
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "GetConfigurationAni called");
    // get Configuration from Assessment Service
    uint32_t duration = 0;
    std::vector<std::string> allowedApps;
    ErrCode ret = AAFwk::AssessmentServiceClient::GetInstance()->GetConfiguration(duration, allowedApps);
    if (ret != ERR_OK) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "GetConfiguration failed, ret: %{public}d", ret);
        AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INTERNAL_ERROR, "Get Configuration failed");
        return AAFwk::CreateAniUndefined(env);
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "GetConfiguration, duration: %{public}d, allowedApps size: %{public}zu",
        duration, allowedApps.size());
    // wrap Configuration to ets object
    return AniAssessment::CreateAssessmentConfigObject(env, duration, allowedApps);
}

ani_object AniAssessment::CreateAssessmentConfigObject(ani_env* env,
                                                       uint32_t duration,
                                                       const std::vector<std::string>& allowedApps)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "CreateAssessmentConfigObject start");
    // create ets object of Configuration
    ani_object configObj = AAFwk::CreateEtsObject(env, CLASS_NAME_ASSESSMENTCONFIG);

    // set duration of ets object of Configuration
    ani_status status = env->Object_SetFieldByName_Int(configObj, "duration", static_cast<ani_int>(duration));
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "failed to set duration, status:%{public}d", status);
        AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INTERNAL_ERROR,
            "Failed to set duration");
        return AAFwk::CreateAniUndefined(env);
    }

    // set allowedApps of ets object of Configuration
    ani_array allowedAppsArray = nullptr;
    status = env->Array_New(static_cast<ani_size>(allowedApps.size()),
                            AAFwk::CreateAniUndefined(env), &allowedAppsArray);
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "failed to create array, status:%{public}d", status);
        AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INTERNAL_ERROR,
            "Failed to create allowedApps array");
        return AAFwk::CreateAniUndefined(env);
    }

    for (size_t i = 0; i < allowedApps.size(); i++) {
        ani_string aniStr = nullptr;
        status = AAFwk::GetAniString(env, allowedApps[i], &aniStr);
        if (status != ANI_OK || aniStr == nullptr) {
            TAG_LOGE(AAFwkTag::DEFAULT, "failed to convert string at index %{public}zu", i);
            env->Array_Delete(allowedAppsArray);
            env->Object_Delete(configObj);
            AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INTERNAL_ERROR, "Failed to convert allowedApp");
            return AAFwk::CreateAniUndefined(env);
        }
        env->Array_Set(allowedAppsArray, static_cast<ani_size>(i), static_cast<ani_ref>(aniStr));
    }

    status = env->Object_SetFieldByName_Ref(configObj, "allowedApps", static_cast<ani_ref>(allowedAppsArray));
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "failed to set allowedApps, status:%{public}d", status);
        AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INTERNAL_ERROR,
            "Failed to set allowedApps");
        return AAFwk::CreateAniUndefined(env);
    }

    TAG_LOGI(AAFwkTag::DEFAULT, "CreateAssessmentConfigObject finish");
    return configObj;
}

ani_status AniAssessment::EndAni(ani_env* env, ani_object context)
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "EndAni called");

    if (context == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "EndAni, ets context is nullptr");
        return AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS, "ets context is nullptr");
    }

    void* contextPtr = AAFwk::GetContextPtr(env, context);
    if (contextPtr == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "EndAni, contextPtr is nullptr");
        return AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS, "contextPtr is nullptr");
    }

    auto contextWeakPtr = static_cast<std::weak_ptr<AbilityRuntime::Context>*>(contextPtr);
    if (contextWeakPtr == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "EndAni, contextWeakPtr is nullptr");
        return AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS, "contextWeakPtr is nullptr");
    }

    auto abilityContext =
        AbilityRuntime::Context::ConvertTo<AbilityRuntime::AbilityContext>(contextWeakPtr->lock());
    if (abilityContext == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "EndAni, abilityContext is nullptr");
        return AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS, "abilityContext is nullptr");
    }

    auto token = abilityContext->GetToken();
    if (token == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "EndAni, token is nullptr");
        return AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS, "Invalid token");
    }

    TAG_LOGI(AAFwkTag::ASSESSMENT, "EndAni, try to call AssessmentServiceClient::End");
    ErrCode ret = AAFwk::AssessmentServiceClient::GetInstance()->End(token);
    if (ret != ERR_OK) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "AssessmentServiceClient::End failed, ret: %{public}d", ret);
        return AAFwk::ThrowAniError(env, static_cast<AAFwk::AssessmentApiErrCode>(ret),
            AAFwk::AssessmentApiErrCodeToErrMsg(static_cast<int32_t>(ret)));
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "EndAni finish");
    return ANI_OK;
}

ani_status AniAssessment::BeginAni(ani_env* env,
                                   ani_long nativeObj,
                                   ani_object context,
                                   ani_object config,
                                   ani_object beginCallback,
                                   ani_object interruptedCallback,
                                   ani_object endCallback)
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "BeginAni start");
    AniAssessment* aniAssessment = reinterpret_cast<AniAssessment*>(nativeObj);
    if (aniAssessment == nullptr) {
        return AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS,
                                    "BeginAni, cannot get AniAssessment object");
    }
    if (!beginCallback || !interruptedCallback || !endCallback) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "OnBeginAni callback is nullptr");
        return AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS);
    }
    return aniAssessment->OnBeginAni(env, context, config, beginCallback, interruptedCallback, endCallback);
}

ani_status AniAssessment::OnBeginAni(ani_env* env,
                                     ani_object context,
                                     ani_object config,
                                     ani_object beginCallback,
                                     ani_object interruptedCallback,
                                     ani_object endCallback)
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "OnBeginAni called");
    // check params
    std::string selfBundleName = "";
    sptr<IRemoteObject> token = nullptr;
    if (CheckParamsOfBegin(env, context, config, selfBundleName, token) != ANI_OK) {
        return AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS);
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "OnBeginAni selfBundleName: %{public}s", selfBundleName.c_str());
    // get duration
    uint32_t duration = AAFwk::GetUint32FromJs(env, config, "duration");
    // get allowedApps
    std::vector<std::string> allowedApps;
    ani_status status = AAFwk::GetStringArrayFromJs(env, config, "allowedApps", allowedApps);
    if (status != ANI_OK) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "OnBeginAni get allowedApps failed, status: %{public}d", status);
        return AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS, "Failed to get allowedApps");
    }
    // add self bundleName to allowedApps
    auto it = std::find(allowedApps.begin(), allowedApps.end(), selfBundleName);
    if (it != allowedApps.end()) {
        allowedApps.erase(it);
    }
    allowedApps.push_back(selfBundleName);
    // check the size of allowedApps
    if (allowedApps.size() > MAX_ALLOWED_APPS_COUNT) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "OnBeginAni too many allowedApps, size: %{public}zu, max: %{public}d",
            allowedApps.size(), MAX_ALLOWED_APPS_COUNT);
        return AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS, "too many allowedApps");
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "OnBeginAni duration: %{public}d, allowedApps size: %{public}zu",
        duration, allowedApps.size());
    // create AniAssessmentCallback
    sptr<AAFwk::AniAssessmentCallback> callbackRef =
        CreateAniAssessmentCallback(env, beginCallback, interruptedCallback, endCallback);
    if (callbackRef == nullptr) {
        return AAFwk::ThrowAniError(env, AAFwk::AssessmentApiErrCode::ERR_INVALID_PARAMS,
            "CreateAniAssessmentCallback failed");
    }
    // use AssessmentServiceClient::Begin
    sptr<IRemoteObject> callbackObj = callbackRef;
    ErrCode ret = AAFwk::AssessmentServiceClient::GetInstance()->Begin(token, duration, allowedApps, callbackObj);
    if (ret != ERR_OK) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "OnBeginAni Begin failed, ret: %{public}d", ret);
        return AAFwk::ThrowAniError(env, static_cast<AAFwk::AssessmentApiErrCode>(ret),
            AAFwk::AssessmentApiErrCodeToErrMsg(static_cast<int32_t>(ret)));
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "OnBeginAni finish");
    return ANI_OK;
}

ani_status AniAssessment::CheckParamsOfBegin(ani_env* env,
                                             ani_object context,
                                             ani_object config,
                                             std::string& selfBundleName,
                                             sptr<IRemoteObject>& token)
{
    if (context == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "OnBeginAni context is nullptr");
        return ANI_ERROR;
    }
    if (config == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "OnBeginAni config is nullptr");
        return ANI_ERROR;
    }
    void* contextPtr = AAFwk::GetContextPtr(env, context);
    if (contextPtr == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "OnBeginAni ets context is nullptr");
        return ANI_ERROR;
    }
    auto contextWeakPtr = static_cast<std::weak_ptr<AbilityRuntime::Context>*>(contextPtr);
    if (contextWeakPtr == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "OnBeginAni contextWeakPtr is nullptr");
        return ANI_ERROR;
    }

    auto abilityContext =
        AbilityRuntime::Context::ConvertTo<AbilityRuntime::AbilityContext>(contextWeakPtr->lock());
    if (abilityContext == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "OnBeginAni abilityContext is nullptr");
        return ANI_ERROR;
    }
    selfBundleName = abilityContext->GetBundleName();

    token = abilityContext->GetToken();
    if (token == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "OnBeginAni token is nullptr");
        return ANI_ERROR;
    }
    return ANI_OK;
}

sptr<AAFwk::AniAssessmentCallback> AniAssessment::CreateAniAssessmentCallback(ani_env* env,
                                                                              ani_object beginCallback,
                                                                              ani_object interruptedCallback,
                                                                              ani_object endCallback)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "CreateAniAssessmentCallback start");

    ani_ref onBeginRef = nullptr;
    if (beginCallback == nullptr ||
        env->GlobalReference_Create(beginCallback, &onBeginRef) != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AniAssessmentCallback, create global onBeginRef fail");
        return nullptr;
    }

    ani_ref onInterruptedRef = nullptr;
    if (interruptedCallback == nullptr ||
        env->GlobalReference_Create(interruptedCallback, &onInterruptedRef) != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AniAssessmentCallback, create global onInterruptedRef fail");
        env->GlobalReference_Delete(onBeginRef);
        return nullptr;
    }

    ani_ref onEndRef = nullptr;
    if (endCallback == nullptr ||
        env->GlobalReference_Create(endCallback, &onEndRef) != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AniAssessmentCallback, create global onEndRef fail");
        env->GlobalReference_Delete(onBeginRef);
        env->GlobalReference_Delete(onInterruptedRef);
        return nullptr;
    }

    sptr<AAFwk::AniAssessmentCallback> aniAssessmentCallback =
        new (std::nothrow) AAFwk::AniAssessmentCallback(env, onBeginRef, onInterruptedRef, onEndRef);

    TAG_LOGI(AAFwkTag::DEFAULT, "CreateAniAssessmentCallback finish");
    return aniAssessmentCallback;
}

} // namespace AbilityRuntime
} // namespace OHOS

extern "C" {
ANI_EXPORT ani_status ANI_Constructor(ani_vm* vm, uint32_t* result)
{
    using namespace OHOS::AAFwk;
    ani_status ret;
    ani_env* env = nullptr;

    TAG_LOGI(AAFwkTag::DEFAULT, "AniAssessment ANI_Constructor start");

    if ((ret = vm->GetEnv(ANI_VERSION_1, &env)) != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "ANI_Constructor get env failed");
        return ANI_NOT_FOUND;
    }

    ani_namespace nsp = nullptr;
    if ((ret = env->FindNamespace("@ohos.customization.assessment.assessment", &nsp)) != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "ANI_Constructor failed to find namespace assessment, %{public}u", ret);
        return ANI_NOT_FOUND;
    }

    std::array funcs = {
        ani_native_function{"isActive", ":z",
            reinterpret_cast<void*>(OHOS::AbilityRuntime::AniAssessment::IsActiveAni)},
        ani_native_function{"end", "C{application.UIAbilityContext.UIAbilityContext}:",
            reinterpret_cast<void*>(OHOS::AbilityRuntime::AniAssessment::EndAni)},
        ani_native_function{"getConfiguration", ":C{@ohos.customization.assessment.assessment.AssessmentConfig}",
            reinterpret_cast<void*>(OHOS::AbilityRuntime::AniAssessment::GetConfigurationAni)},
        ani_native_function{"beginSync", nullptr,
            reinterpret_cast<void*>(OHOS::AbilityRuntime::AniAssessment::BeginAni)},
    };

    for (const auto& func : funcs) {
        TAG_LOGI(AAFwkTag::DEFAULT, "bind function: %{public}s, signature: %{public}s", func.name, func.signature);
        if ((ret = env->Namespace_BindNativeFunctions(nsp, &func, 1u)) != ANI_OK) {
            TAG_LOGE(AAFwkTag::DEFAULT, "bind function failed, ret: %{public}u, name:%{public}s, signature:%{public}s",
                ret, func.name, func.signature);
            return ANI_NOT_FOUND;
        }
    }

    // Init
    if ((ret = OHOS::AbilityRuntime::AniAssessment::Init(env, nsp)) != ANI_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AniAssessment Init failed");
        return ANI_ERROR;
    }

    *result = ANI_VERSION_1;
    TAG_LOGI(AAFwkTag::DEFAULT, "AniAssessment ANI_Constructor finish");
    return ANI_OK;
}  // ANI_Constructor
} // extern "C"