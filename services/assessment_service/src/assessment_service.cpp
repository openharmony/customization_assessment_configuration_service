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
#include <sys/time.h>
#include <random>

#include "callback_manager.h"
#include "hilog_tag_wrapper.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "syspara/parameters.h"
#include "system_ability_definition.h"
#include "extension_manager_client.h"

namespace OHOS {
namespace AAFwk {
namespace {
const char* const PARAM_ASSESSMENT_IS_ACTIVE = "persist.assessment.is_active";
const char* const PARAM_ASSESSMENT_DURATION = "persist.assessment.duration";
const char* const PARAM_ASSESSMENT_ALLOWED_APPS = "persist.assessment.allowed_apps";
const char APP_DELIMITER = '|';
const uint64_t MILLISECONDS_UNIT = 1000;
const uint64_t DEFAULT_TIME_SLICE_INTERVAL = 5 * MILLISECONDS_UNIT;
const int32_t DEFAULT_MAX_DURATION = 8 * 3600;
const int32_t TICKET_LEN = 16;

const char* const ASSESSMENT_COMMON_EVENT_CONFIRMATION = "assessment.event.confirmation";
const std::string SCENEBOARD_BUNDLE_NAME = "com.ohos.sceneboard";
const std::string SCENEBOARD_ABILITY_NAME = "com.ohos.sceneboard.systemdialog";
const std::string SYSTEM_UI_BUNDLE_NAME = "com.test.demo";
const std::string SYSTEM_UI_ABILITY_NAME = "CustomDialogAbility";

enum AssessmentConfirmationOperation : uint32_t {
    CANCEL = 0,
    CONFIRM = 1
};
}

std::string GenerateRandomString(size_t length)
{
    const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);

    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; i++) {
        result += chars[dis(gen)];
    }

    return result;
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

    if (!SubscribeCommonEvent()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "assessment subscribe fail");
        return false;
    }

    this->thread_ = std::thread([this]() {
        this->DoLoop();
    });
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
    endpointCheckPoint_ = 0;
}

ErrCode AssessmentService::Begin(const sptr<IRemoteObject> &token, uint32_t duration,
                                 const std::vector<std::string> &allowedApps,
                                 const sptr<IRemoteObject> &callback, int32_t &errCode)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Begin called, duration: %{public}d, allowedApps size: %{public}zu",
        duration, allowedApps.size());

    std::unique_lock<std::mutex> lock(this->mutexSa_);
    if (isActive_) {
        TAG_LOGW(AAFwkTag::DEFAULT, "Assessment already active, cleanup and restart");
        CleanupCurrentSession();
    }

    OHOS::AAFwk::Want want;
    want.SetElementName(SCENEBOARD_BUNDLE_NAME, SCENEBOARD_ABILITY_NAME);

    std::string ticket = GenerateRandomString(TICKET_LEN);
    std::string parameters =
        "{\"ability.want.params.uiExtensionType\":\"sysDialog/common\",\"ticket\":\"" + ticket + "\"}";
    sptr<AssessmentAbilityConnection> connection = sptr<AssessmentAbilityConnection> (
        new (std::nothrow)AssessmentAbilityConnection(
            SYSTEM_UI_BUNDLE_NAME, SYSTEM_UI_ABILITY_NAME, parameters));
    if (connection == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "connection is nullptr.");
        return ERR_OK;
    }

    constexpr int32_t DEFAULT_VALUE = -1;
    auto ret = OHOS::AAFwk::ExtensionManagerClient::GetInstance().ConnectServiceExtensionAbility(
        want, connection, nullptr, DEFAULT_VALUE);
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment ConnectServiceExtensionAbility ret is:%{public}d.", ret);

    callerToken_ = token;
    CallbackManager::GetInstance().RegisterCallback(token, callback);
    currentConfig_.duration = (duration == 0) ? DEFAULT_MAX_DURATION : duration;
    currentConfig_.allowedApps = allowedApps;

    auto pt = std::chrono::system_clock::now() + std::chrono::seconds(duration);
    endpointCheckPoint_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        pt.time_since_epoch()).count();
    isActive_ = true;
    errCode = ERR_OK;

    SaveState();
    CallbackManager::GetInstance().OnBegin(token, 0, "");
    condSa_.notify_all();

    return ERR_OK;
}

ErrCode AssessmentService::End(const sptr<IRemoteObject> &token, int32_t &errCode)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "End called");

    std::unique_lock<std::mutex> lock(this->mutexSa_);
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
    std::unique_lock<std::mutex> lock(this->mutexSa_);
    isActive = isActive_;
    return ERR_OK;
}

ErrCode AssessmentService::GetConfiguration(uint32_t &duration, std::vector<std::string> &allowedApps)
{
    std::unique_lock<std::mutex> lock(this->mutexSa_);
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

void AssessmentService::DoLoop()
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Assessment execute enter");
    this->running_ = true;
    while (running_) {
        std::unique_lock<std::mutex> lock(this->mutexSa_);
        if (!running_) {
            break;
        }

        int32_t timeout = this->ComputeNextTaskTimeoutLockedUnsafe();
        TAG_LOGD(AAFwkTag::DEFAULT, "Assessment execute compute next round: %{public}d", timeout);

        auto waitUntil = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);
        condSa_.wait_until(lock, waitUntil);

        this->CheckEndpointAndExecuteTaskLockedUnsafe();

        TAG_LOGI(AAFwkTag::DEFAULT, "Assessment execute once");
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "Assessment execute exit");
}

int32_t AssessmentService::ComputeNextTaskTimeoutLockedUnsafe()
{
    if (!isActive_ || callerToken_ == nullptr) {
        return DEFAULT_TIME_SLICE_INTERVAL;
    }

    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (now >= endpointCheckPoint_) {
        return 0;
    }

    uint64_t delta = endpointCheckPoint_ - now;
    if (delta > DEFAULT_TIME_SLICE_INTERVAL) {
        return static_cast<int32_t>(DEFAULT_TIME_SLICE_INTERVAL);
    }
    return static_cast<int32_t>(delta);
}

void AssessmentService::CheckEndpointAndExecuteTaskLockedUnsafe()
{
    if (!isActive_) {
        return;
    }
    if (callerToken_ == nullptr) {
        return;
    }
    if (currentConfig_.duration == 0) {
        TAG_LOGD(AAFwkTag::DEFAULT, "assessment task no time limited");
        return;
    }

    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    TAG_LOGI(AAFwkTag::DEFAULT,
        "assessment task timeout, end: %{public}lu, point:%{public}lu", now, endpointCheckPoint_);
    if (now >= endpointCheckPoint_) {
        this->CleanupCurrentSession();
        auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (sam != nullptr) {
            sam->UnloadSystemAbility(ASSESSMENT_SERVICE_ID);
        }
    }
}

void AssessmentService::Quit()
{
    if (this->running_) {
        this->running_ = false;
        condSa_.notify_all();
        if (thread_.joinable()) {
            thread_.join();
        }
    }
}

bool AssessmentService::SubscribeCommonEvent()
{
    OHOS::EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(ASSESSMENT_COMMON_EVENT_CONFIRMATION);
    OHOS::EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);

    this->assessmentEventObserver_ = AssessmentEventObserver::Create(subscribeInfo,
        std::bind(&AssessmentService::DispatchEvent, this, std::placeholders::_1));
    if (this->assessmentEventObserver_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "assessment subscribe null systemEventObserver_");
        return false;
    }
    if (!this->assessmentEventObserver_->Subscribe()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "assessment subscribe fail");
        return false;
    }

    TAG_LOGI(AAFwkTag::DEFAULT, "assessment subscribe success");
    return true;
}

void AssessmentService::UnsubscribeCommonEvent()
{
    if (this->assessmentEventObserver_ == nullptr) {
        return;
    }
    this->assessmentEventObserver_->Unsubscribe();
}

void AssessmentService::HandleBegin(const std::string &ticket, uint32_t operation)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment handle begin %{public}s, %{public}d", ticket.c_str(), operation);
    if (operation == AssessmentConfirmationOperation::CANCEL) {
    } else if (operation == AssessmentConfirmationOperation::CONFIRM) {
    } else {
    }
}

void AssessmentService::DispatchEvent(const OHOS::EventFwk::CommonEventData& eventData)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment dispatch_event");
    const OHOS::AAFwk::Want& want = eventData.GetWant();
    std::string action = want.GetAction();
    if (action == ASSESSMENT_COMMON_EVENT_CONFIRMATION) {
        std::string data = eventData.GetData();
        TAG_LOGI(AAFwkTag::DEFAULT, "assessment receive data:%{public}s for handle begin", data.c_str());

        std::string ticket;
        int operation = 0;

        std::replace(data.begin(), data.end(), ':', ' ');
        std::istringstream is(data);
        if ((is >> ticket >> operation)) {
            HandleBegin(ticket, operation);
        }
    }
}
}  // namespace AAFwk
}  // namespace OHOS