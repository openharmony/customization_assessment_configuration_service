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

#include "ani_assessment_callback.h"

#include "event_runner.h"
#include "hilog_tag_wrapper.h"
#include "assessment_callback_code.h"
#include "ani_assessment_utils.h"

namespace OHOS {
namespace AAFwk {

AniAssessmentCallback::AniAssessmentCallback(ani_env* env,
                                             ani_ref onBeginRef,
                                             ani_ref onInterruptedRef,
                                             ani_ref onEndRef)
    : IPCObjectStub(u"AniAssessmentCallback", false), env_(env),
      onBeginRef_(onBeginRef), onInterruptedRef_(onInterruptedRef), onEndRef_(onEndRef)
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "AniAssessmentCallback constructor");
    // set vm
    ani_vm* vm = nullptr;
    ani_status aniRet = env->GetVM(&vm);
    if (aniRet != ANI_OK || vm == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "AniAssessmentCallback, Get VM failed, ret: %{public}u", aniRet);
    }
    vm_ = vm;
    // GET mainRunner
    auto mainRunner = AppExecFwk::EventRunner::GetMainEventRunner();
    if (mainRunner == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AniAssessmentCallback, GetMainEventRunner failed");
        return;
    }
    // set eventHandler of main Thread
    eventHandler_ = std::make_shared<AppExecFwk::EventHandler>(mainRunner);
    TAG_LOGI(AAFwkTag::DEFAULT, "eventHandler_ created successfully");
}

AniAssessmentCallback::~AniAssessmentCallback()
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "AniAssessmentCallback destructor start");
    if (vm_ == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "AniAssessmentCallback destructor, vm_ is nullptr");
        return;
    }
    ani_env* env = nullptr;
    ani_status ret = vm_->GetEnv(ANI_VERSION_1, &env);
    if (ret != ANI_OK || env == nullptr) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "destructor get env failed, ret: %{public}u, try to use saved env_", ret);
        if (env_ == nullptr) {
            TAG_LOGE(AAFwkTag::ASSESSMENT, "destructor saved env_ is also nullptr, cannot cleanup global references");
            return;
        }
    }
    if (onBeginRef_ != nullptr && env_->GlobalReference_Delete(onBeginRef_) != ANI_OK) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "AniAssessmentCallback clear global ref onBeginRef_ failed");
    }
    if (onInterruptedRef_ != nullptr && env_->GlobalReference_Delete(onInterruptedRef_) != ANI_OK) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "AniAssessmentCallback clear global ref onInterruptedRef_ failed");
    }
    if (onEndRef_ != nullptr && env_->GlobalReference_Delete(onEndRef_) != ANI_OK) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "AniAssessmentCallback clear global ref onEndRef_ failed");
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "AniAssessmentCallback destructor finish");
}

int AniAssessmentCallback::OnRemoteRequest(uint32_t code,
                                           MessageParcel& data,
                                           MessageParcel& reply,
                                           MessageOption& option)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "OnRemoteRequest called, code: %{public}d", code);
    switch (static_cast<AssessmentCallbackCode>(code)) {
        case AssessmentCallbackCode::ON_BEGIN: {
            int32_t jsCode = data.ReadInt32();
            std::string message = data.ReadString();
            this->CallJsOnBegin(jsCode, message);
            break;
        }
        case AssessmentCallbackCode::ON_INTERRUPTED: {
            int32_t reason = data.ReadInt32();
            std::string message = data.ReadString();
            this->CallJsOnInterrupted(reason, message);
            break;
        }
        case AssessmentCallbackCode::ON_END: {
            this->CallJsOnEnd();
            break;
        }
        default:
            TAG_LOGW(AAFwkTag::DEFAULT, "OnRemoteRequest, unknown code: %{public}d", code);
            return -1;
    }
    return 0;
}

void AniAssessmentCallback::CallJsOnBegin(int32_t code, const std::string& message)
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "CallJsOnBegin in, code: %{public}d, message: %{public}s", code, message.c_str());
    if (onBeginRef_ == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "CallJsOnBegin, onBeginRef_ is null");
        return;
    }
    // get env
    ani_env* env = nullptr;
    ani_status ret = vm_->GetEnv(ANI_VERSION_1, &env);
    if (ret != ANI_OK || env == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "CallJsOnBegin, Get env failed, ret:%{public}u", ret);
        return;
    }
    // working
    CallAniFunctionVoidWithIntString(env, "@ohos.customization.assessment.assessment", "runBeginEvent",
                                     onBeginRef_, code, message);
}

void AniAssessmentCallback::CallJsOnInterrupted(int32_t code, const std::string& message)
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "CallJsOnInterrupted start, code: %{public}d, message: %{public}s",
             code, message.c_str());
    if (onInterruptedRef_ == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "CallJsOnInterrupted, onInterruptedRef_ is null");
        return;
    }
    // check eventHandler_
    if (eventHandler_ == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "eventHandler_ is nullptr, cannot post CallJsOnInterrupted to main Thread");
        return;
    }
    auto task = [selfRef_ = sptr<AniAssessmentCallback>(this), vm = vm_, code, message]() {
        // check this AniAssessmentCallback object
        if (selfRef_ == nullptr) {
            TAG_LOGE(AAFwkTag::ASSESSMENT, "CallJsOnInterrupted task, this AniAssessmentCallback object is nullptr");
            return;
        }
        // check vm
        if (vm == nullptr) {
            TAG_LOGE(AAFwkTag::ASSESSMENT, "CallJsOnInterrupted task, this AniAssessmentCallback vm is nullptr");
            return;
        }
        // get env
        ani_env* env = nullptr;
        ani_status ret = vm->GetEnv(ANI_VERSION_1, &env);
        if (ret != ANI_OK || env == nullptr) {
            TAG_LOGE(AAFwkTag::ASSESSMENT, "this IPC thread not attached to VM, cannot post task to main thread");
            return;
        }
        TAG_LOGI(AAFwkTag::ASSESSMENT, "CallJsOnInterrupted task, start to CallAniFunctionVoidWithIntString");
        // working
        CallAniFunctionVoidWithIntString(env, "@ohos.customization.assessment.assessment", "runInterruptedEvent",
                                         selfRef_->onInterruptedRef_, code, message);
    };
    // send task to main thread
    eventHandler_->PostTask(task, "ani_onInterrupted", 0, AppExecFwk::EventQueue::Priority::IMMEDIATE);
}

void AniAssessmentCallback::CallJsOnEnd()
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "CallJsOnEnd start");
    if (onEndRef_ == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "CallJsOnEnd, onEndRef_ is null");
        return;
    }
    // get env
    ani_env* env = nullptr;
    ani_status ret = vm_->GetEnv(ANI_VERSION_1, &env);
    if (ret != ANI_OK || env == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "CallJsOnEnd, Get env failed, ret: %{public}u", ret);
        return;
    }
    // working
    CallAniFunctionVoid(env, "@ohos.customization.assessment.assessment", "runEndEvent", onEndRef_);
}

} // namespace AAFwk
} // namespace OHOS
