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
const char* const PARAM_ASSESSMENT_IS_ACTIVE = "persist.assessment.is_active";
const char* const PARAM_ASSESSMENT_DURATION = "persist.assessment.duration";
const char* const PARAM_ASSESSMENT_ALLOWED_APPS = "persist.assessment.allowed_apps";
const char APP_DELIMITER = '|';
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
    std::string activeStr = system::GetParameter(PARAM_ASSESSMENT_IS_ACTIVE, "");
    if (!activeStr.empty() && activeStr == "true") {
        isActive_ = true;
        TAG_LOGI(AAFwkTag::DEFAULT, "Loaded state: isActive=true");
    }

    std::string durationStr = system::GetParameter(PARAM_ASSESSMENT_DURATION, "0");
    if (!durationStr.empty()) {
        currentConfig_.duration = static_cast<uint32_t>(std::stoul(durationStr));
    }

    std::string appsStr = system::GetParameter(PARAM_ASSESSMENT_ALLOWED_APPS, "");
    if (!appsStr.empty()) {
        std::stringstream ss(appsStr);
        std::string app;
        while (std::getline(ss, app, APP_DELIMITER)) {
            if (!app.empty()) {
                currentConfig_.allowedApps.push_back(app);
            }
        }
        TAG_LOGI(AAFwkTag::DEFAULT, "Loaded allowedApps size: %{public}zu", currentConfig_.allowedApps.size());
    }
}

void AssessmentService::SaveState()
{
    system::SetParameter(PARAM_ASSESSMENT_IS_ACTIVE, isActive_ ? "true" : "false");
    system::SetParameter(PARAM_ASSESSMENT_DURATION, std::to_string(currentConfig_.duration));

    std::stringstream ss;
    for (size_t i = 0; i < currentConfig_.allowedApps.size(); ++i) {
        if (i > 0) {
            ss << APP_DELIMITER;
        }
        ss << currentConfig_.allowedApps[i];
    }
    system::SetParameter(PARAM_ASSESSMENT_ALLOWED_APPS, ss.str());
    TAG_LOGD(AAFwkTag::DEFAULT, "State saved");
}

void AssessmentService::ClearState()
{
    system::SetParameter(PARAM_ASSESSMENT_IS_ACTIVE, "false");
    system::SetParameter(PARAM_ASSESSMENT_DURATION, "0");
    system::SetParameter(PARAM_ASSESSMENT_ALLOWED_APPS, "");
    TAG_LOGD(AAFwkTag::DEFAULT, "State cleared");
}

void AssessmentService::CleanupCurrentSession()
{
    if (callerToken_ != nullptr) {
        CallbackManager::GetInstance().OnEnd(callerToken_);
        CallbackManager::GetInstance().UnregisterCallback(callerToken_);
    }
    isActive_ = false;
    callerToken_ = nullptr;
    currentConfig_ = AssessmentConfig();
}

ErrCode AssessmentService::Begin(const sptr<IRemoteObject> &token, uint32_t duration,
                                 const std::vector<std::string> &allowedApps,
                                 const sptr<IRemoteObject> &callback, int32_t &errCode)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Begin called, duration: %{public}d, allowedApps size: %{public}zu",
        duration, allowedApps.size());

    if (isActive_) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Assessment already active, cleanup and restart");
        CleanupCurrentSession();
    }

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
    TAG_LOGI(AAFwkTag::DEFAULT, "End called");

    if (!isActive_) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Assessment not active");
        errCode = ERR_INVALID_VALUE;
        return ERR_OK;
    }

    sptr<IRemoteObject> caller = callerToken_;
    CleanupCurrentSession();
    ClearState();
    errCode = ERR_OK;

    if (caller != nullptr) {
        CallbackManager::GetInstance().OnEnd(caller);
    }

    auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sam != nullptr) {
        sam->UnloadSystemAbility(ASSESSMENT_SERVICE_ID);
    }

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
    TAG_LOGI(AAFwkTag::DEFAULT, "NotifyBegin called, code: %{public}d", code);
    if (callerToken_ != nullptr) {
        CallbackManager::GetInstance().OnBegin(callerToken_, code, message);
    }
}

void AssessmentService::NotifyInterrupted(int32_t reason, const std::string &message)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "NotifyInterrupted called, reason: %{public}d", reason);
    if (callerToken_ != nullptr) {
        CallbackManager::GetInstance().OnInterrupted(callerToken_, reason, message);
    }
}

void AssessmentService::NotifyEnd()
{
    TAG_LOGI(AAFwkTag::DEFAULT, "NotifyEnd called");
    if (callerToken_ != nullptr) {
        CallbackManager::GetInstance().OnEnd(callerToken_);
    }
}
}  // namespace AAFwk
}  // namespace OHOS