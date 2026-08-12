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

#include "assessment_service.h"

#include <sstream>

#include "callback_manager.h"
#include "hilog_tag_wrapper.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "syspara/parameters.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace AAFwk {
namespace {
}

std::mutex AssessmentService::mutex_;
sptr<AssessmentService> AssessmentService::instance_;

sptr<AssessmentService> AssessmentService::GetInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (instance_ == nullptr) {
        instance_ = new AssessmentService();
    }
    return instance_;
}

bool AssessmentService::Init()
{
    eventRunner_ = AppExecFwk::EventRunner::Create("AssessmentSvrMain");
    if (eventRunner_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "null eventRunner_");
        return false;
    }

    eventHandler_ = std::make_shared<AppExecFwk::EventHandler>(eventRunner_);
    if (eventHandler_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "null eventHandler_");
        return false;
    }

    return true;
}

void AssessmentService::LoadState()
{
}

void AssessmentService::SaveState()
{
}

void AssessmentService::ClearState()
{
}

void AssessmentService::CleanupCurrentSession()
{
}

ErrCode AssessmentService::Begin(const sptr<IRemoteObject> &token, uint32_t duration,
                                 const std::vector<std::string> &allowedApps,
                                 const sptr<IRemoteObject> &callback, int32_t &errCode)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Begin called, duration: %{public}d, allowedApps size: %{public}zu",
        duration, allowedApps.size());

    callerToken_ = token;
    CallbackManager::GetInstance().RegisterCallback(token, callback);
    currentConfig_.duration = duration;
    currentConfig_.allowedApps = allowedApps;
    isActive_ = true;
    errCode = ERR_OK;

    SaveState();
    CallbackManager::GetInstance().OnBegin(token, 0, "");
    return ERR_OK;
}

ErrCode AssessmentService::End(const sptr<IRemoteObject> &token, int32_t &errCode)
{
    return ERR_OK;
}

ErrCode AssessmentService::IsActive(bool &isActive)
{
    isActive = isActive_;
    return ERR_OK;
}

ErrCode AssessmentService::GetConfiguration(uint32_t &duration, std::vector<std::string> &allowedApps)
{
    duration = currentConfig_.duration;
    allowedApps = currentConfig_.allowedApps;
    return ERR_OK;
}

void AssessmentService::NotifyBegin(int32_t code, const std::string &message)
{
}

void AssessmentService::NotifyInterrupted(int32_t reason, const std::string &message)
{
}

void AssessmentService::NotifyEnd()
{
}
}  // namespace AAFwk
}  // namespace OHOS