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
#include "callback_manager.h"
#include "hilog_tag_wrapper.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "syspara/parameter.h"
#include "syspara/parameters.h"
#include "system_ability_definition.h"
#include "extension_manager_client.h"
#include "assessment_utils.h"
#include "assessment_api_error_code.h"
#include "common_event_subscriber.h"
#include "common_event_subscribe_info.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "assessment_service_app_state_cb.h"
#include "app_mgr_client.h"
#include "app_mgr_interface.h"
#include "singleton.h"
#include "app_mgr_util.h"
#include <input_manager.h>
#include "ipc_skeleton.h"
#include "ability_manager_client.h"
#include "power_mode_info.h"

namespace OHOS {
namespace AAFwk {
namespace {
const char* const PARAM_ASSESSMENT_IS_ACTIVE = "persist.assessment.is_active";
const char* const PARAM_ASSESSMENT_DURATION = "persist.assessment.duration";
const char* const PARAM_ASSESSMENT_ALLOWED_APPS = "persist.assessment.allowed_apps";
const char* const PARAM_ANCO_STATE = "anco_state";
const char APP_DELIMITER = '|';
const uint64_t MILLISECONDS_UNIT = 1000;
const uint64_t DEFAULT_TIME_SLICE_INTERVAL = 5 * MILLISECONDS_UNIT;
const uint32_t DEFAULT_MAX_DURATION = 8 * 60 * 60 * 1000;
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

std::mutex AssessmentService::mutex_;
sptr<AssessmentService> AssessmentService::instance_;

AssessmentService::AssessmentService()
{
    TAG_LOGE(AAFwkTag::DEFAULT, "assessment service created");
}

AssessmentService::~AssessmentService()
{
    TAG_LOGE(AAFwkTag::DEFAULT, "assessment service destroy");
}

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

    InitSubsystems();
    // Heavy dependency init (extension-loader dlopen, DeviceManager and
    // CallManager IPC) runs on the service thread so that SA OnStart is not
    // blocked on external services. Until it completes, the affected env
    // checks are skipped (fail-open, documented in EnvChecker).
    this->thread_ = std::thread([this]() {
        if (!this->envChecker_.Init()) {
            TAG_LOGE(AAFwkTag::DEFAULT, "EnvChecker init incomplete, some env checks may be skipped");
        }
        if (!this->processController_.Init()) {
            TAG_LOGE(AAFwkTag::DEFAULT, "ProcessController init failed");
        }
        this->DoLoop();
    });
    WatchParameter(
        PARAM_ANCO_STATE,
        AncoStateChangeCallback,
        this);
    return true;
}

bool AssessmentService::InitSubsystems()
{
    sptr<AAFwk::AssessmentServiceAppStateCb> appStateObserver_ = nullptr;
    appStateObserver_ = new (std::nothrow) AAFwk::AssessmentServiceAppStateCb([this](const std::string &bundleName) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AssessmentServiceAppStateCb cb: %{public}s", bundleName.c_str());
        this->AppDieHandle(bundleName);
    });
    if (appStateObserver_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AssessmentServiceAppStateCb allocation failed");
        return false;
    }
    auto appMgrClient = DelayedSingleton<OHOS::AppExecFwk::AppMgrClient>::GetInstance();
    if (appMgrClient == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AppMgrClient get instance failed");
        return false;
    }

    int32_t regResult = appMgrClient->RegisterApplicationStateObserver(appStateObserver_);
    if (regResult != 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "RegisterApplicationStateObserver failed, result = %{public}d", regResult);
        return false;
    }

    auto inputManager = MMI::InputManager::GetInstance();
    if (inputManager == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "InputManager get instance failed");
        return false;
    }
    switchId_ = inputManager->SubscribeSwitchEvent(
        std::bind(&AssessmentService::OnSwitchEvent, this, std::placeholders::_1),
        OHOS::MMI::SwitchEvent::SWITCH_LID);
    if (switchId_ < 0) {
        TAG_LOGE(AAFwkTag::DEFAULT, "SubscribeSwitchEvent failed, switchId = %{public}d", switchId_);
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

    std::string ancoState = system::GetParameter(PARAM_ANCO_STATE, "2");
    isWaittingAncoActive_ = envChecker_.IsAwakeAnco(ancoState);
    if (isActive_) {
        envChecker_.RestrictAncoApp();
    }
    TAG_LOGD(AAFwkTag::DEFAULT, "State saved");
}

void AssessmentService::ClearState()
{
    system::SetParameter(PARAM_ASSESSMENT_IS_ACTIVE, "false");
    system::SetParameter(PARAM_ASSESSMENT_DURATION, "0");
    system::SetParameter(PARAM_ASSESSMENT_ALLOWED_APPS, "");
    std::string ancoState = system::GetParameter(PARAM_ANCO_STATE, "2");
    isWaittingAncoActive_ = envChecker_.IsAwakeAnco(ancoState);
    TAG_LOGD(AAFwkTag::DEFAULT, "State cleared");
}

void AssessmentService::ConfigCurrentSession(const sptr<IRemoteObject> &token, uint32_t duration,
                                             const std::vector<std::string> &allowedApps,
                                             const sptr<IRemoteObject> &callback)
{
    ticket_ = AssessmentServiceUtils::GenerateRandomString(TICKET_LEN);
    callerToken_ = token;
    CallbackManager::GetInstance().RegisterCallback(token, callback);
    duration = std::min(duration, DEFAULT_MAX_DURATION);
    currentConfig_.duration = (duration == 0) ? DEFAULT_MAX_DURATION : duration;
    currentConfig_.examId = AssessmentServiceUtils::GenerateRandomExamId();
    currentConfig_.examStartTime = AssessmentServiceUtils::GetCurrentAssessmentTimeStamp();
    currentConfig_.allowedApps = allowedApps;
    bundleName_ = allowedApps.back();
    endpointCheckPoint_ = 0;
    isActive_ = false;
    examStatus_ = AssessmentExamStatus::CONFIRMING;
    callingUid_ = IPCSkeleton::GetCallingUid();
}

void AssessmentService::CleanupCurrentSession()
{
    processController_.Deactivate();
    if (callerToken_ != nullptr) {
        CallbackManager::GetInstance().UnregisterCallback(callerToken_);
    }
    isActive_ = false;
    examStatus_ = AssessmentExamStatus::IDLE;
    callerToken_ = nullptr;
    currentConfig_ = AssessmentConfig();
    bundleName_ = "";
    endpointCheckPoint_ = 0;
    callingUid_ = 0;
}

bool AssessmentService::CheckBeginPreconditions(const sptr<IRemoteObject> &token,
    const std::vector<std::string> &allowedApps, const sptr<IRemoteObject> &callback, int32_t &errCode)
{
    if (token == nullptr || callback == nullptr || allowedApps.empty()) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment invalid params");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_PARAMS);
        return false;
    }
    if (!AssessmentServiceUtils::CheckDeviceTypeSupported()) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment device not supported");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_CAPABILITY_NOT_SUPPORT);
        return false;
    }
    if (!AssessmentServiceUtils::VerifyCallingPermission(PERMISSION_ASSESSMENT_CONFIGURATION)) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "no permission: ohos.permission.ASSESSMENT_CONFIGURATION");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_PERMISSION_DENIED);
        return false;
    }
    // Environment check runs without holding mutexSa_: it may block on
    // device-manager / call-manager / closed-source extension operations.
    if (!envChecker_.CheckAll()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Environment check failed, cannot start exam mode");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_OPERATION);
        return false;
    }
    return true;
}

ErrCode AssessmentService::Begin(const sptr<IRemoteObject> &token,
                                 uint32_t duration,
                                 const std::vector<std::string> &allowedApps,
                                 const sptr<IRemoteObject> &callback,
                                 int32_t &errCode)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "Begin called, duration: %{public}d, allowedApps size: %{public}zu",
        duration, allowedApps.size());

    if (!CheckBeginPreconditions(token, allowedApps, callback, errCode)) {
        return ERR_OK;
    }

    std::unique_lock<std::mutex> lock(this->mutexSa_);
    if (isActive_) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "Assessment already active");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_ASSESSMENT_ALREADY_ACTIVE);
        return ERR_OK;
    }

    if (examStatus_ == AssessmentExamStatus::CONFIRMING) {
        if (callerToken_ != nullptr) {
            CallbackManager::GetInstance().OnBegin(
                callerToken_, static_cast<int32_t>(AssessmentErrorCode::SYSTEM_ERROR),
                AssessmentErrCodeToErrMsg(AssessmentErrorCode::SYSTEM_ERROR));
            TAG_LOGI(AAFwkTag::ASSESSMENT, "assessment app exam chance be occupied");
        }
    }
    CleanupCurrentSession();
    ConfigCurrentSession(token, duration, allowedApps, callback);

    errCode = InvokeSystemDialog();
    if (errCode != ERR_OK) {
        errCode = ERR_OK;
        ConfirmationBeginLockedUnsafe();
        condSa_.notify_all();
        // Copy under the lock, then activate process control outside it.
        std::vector<std::string> confirmedApps = currentConfig_.allowedApps;
        lock.unlock();
        ActivateProcessControl(confirmedApps);
        return ERR_OK;
    }

    TAG_LOGI(AAFwkTag::ASSESSMENT, "Begin over, invoke system dialog, ret: %{public}d", errCode);
    return ERR_OK;
}

ErrCode AssessmentService::End(const sptr<IRemoteObject> &token, int32_t &errCode)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "End called");
    if (token == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment invalid params");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_PARAMS);
        return ERR_OK;
    }
    
    if (!AssessmentServiceUtils::CheckDeviceTypeSupported()) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment device not supported");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_CAPABILITY_NOT_SUPPORT);
        return ERR_OK;
    }
    if (!AssessmentServiceUtils::VerifyCallingPermission(PERMISSION_ASSESSMENT_CONFIGURATION)) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "no permission: ohos.permission.ASSESSMENT_CONFIGURATION");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_PERMISSION_DENIED);
        return ERR_OK;
    }

    std::unique_lock<std::mutex> lock(this->mutexSa_);
    if (!isActive_) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "Assessment not active");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_ASSESSMENT_NOT_ACTIVE);
        return ERR_OK;
    }
    int32_t callingUid = IPCSkeleton::GetCallingUid();
    if (callingUid_ != callingUid) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "Assessment be called for other app");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_OPERATION);
        return ERR_OK;
    }

    ErrCode ret = ExitKioskModeLockedUnsafe();
    if (ret != ERR_OK) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assessment exitKioskMode for end fail");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INTERNAL_ERROR);
        return ERR_OK;
    }

    sptr<IRemoteObject> caller = callerToken_;
    if (caller != nullptr) {
        CallbackManager::GetInstance().OnEnd(caller);
    }
    CleanupCurrentSession();
    ClearState();
    errCode = ERR_OK;

    if (!isWaittingAncoActive_) {
        Destroy();
    }
    
    return ERR_OK;
}

ErrCode AssessmentService::IsActive(bool &isActive, int32_t &errCode)
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "IsActive called");
    errCode = ERR_OK;
    if (!AssessmentServiceUtils::CheckDeviceTypeSupported()) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment device not supported");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_CAPABILITY_NOT_SUPPORT);
        return ERR_OK;
    }
    if (!AssessmentServiceUtils::VerifyCallingPermission(PERMISSION_ASSESSMENT_CONFIGURATION)) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "no permission: ohos.permission.ASSESSMENT_CONFIGURATION");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_PERMISSION_DENIED);
        return ERR_OK;
    }

    std::unique_lock<std::mutex> lock(this->mutexSa_);
    isActive = isActive_;
    return ERR_OK;
}

ErrCode AssessmentService::GetConfiguration(
    uint32_t &duration, std::vector<std::string> &allowedApps, int32_t &errCode)
{
    TAG_LOGI(AAFwkTag::ASSESSMENT, "GetConfiguration called");
    errCode = ERR_OK;
    if (!AssessmentServiceUtils::CheckDeviceTypeSupported()) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment device not supported");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_CAPABILITY_NOT_SUPPORT);
        return ERR_OK;
    }
    if (!AssessmentServiceUtils::VerifyCallingPermission(PERMISSION_ASSESSMENT_CONFIGURATION)) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "no permission: ohos.permission.ASSESSMENT_CONFIGURATION");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_PERMISSION_DENIED);
        return ERR_OK;
    }

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
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (now >= endpointCheckPoint_) {
        this->TimeoutLockedUnsafe();
        this->Destroy();
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

void AssessmentService::Destroy()
{
    auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sam != nullptr) {
        sam->UnloadSystemAbility(ASSESSMENT_SERVICE_ID);
    }
}

bool AssessmentService::SubscribeCommonEvent()
{
    OHOS::EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(ASSESSMENT_COMMON_EVENT_CONFIRMATION);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_ENTER_HIBERNATE);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_EXIT_HIBERNATE);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SHUTDOWN);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_POWER_SAVE_MODE_CHANGED);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_BOOT_COMPLETED);
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

int32_t AssessmentService::InvokeSystemDialog()
{
    OHOS::AAFwk::Want want;
    want.SetElementName(SCENEBOARD_BUNDLE_NAME, SCENEBOARD_ABILITY_NAME);
    std::string parameters =
        "{\"ability.want.params.uiExtensionType\":\"sysDialog/common\",\"ticket\":\"" + ticket_ + "\"}";
    sptr<AssessmentAbilityConnection> connection = sptr<AssessmentAbilityConnection>(
        new (std::nothrow)AssessmentAbilityConnection(
            SYSTEM_UI_BUNDLE_NAME, SYSTEM_UI_ABILITY_NAME, parameters));
    if (connection == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "connection is nullptr.");
        return static_cast<int32_t>(AssessmentApiErrCode::ERR_INTERNAL_ERROR);
    }
    constexpr int32_t DEFAULT_VALUE = -1;
    auto ret = OHOS::AAFwk::ExtensionManagerClient::GetInstance().ConnectServiceExtensionAbility(
        want, connection, nullptr, DEFAULT_VALUE);
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment ConnectServiceExtensionAbility ret is:%{public}d.", ret);
    if (ret != ERR_OK) {
        return static_cast<int32_t>(AssessmentApiErrCode::ERR_INTERNAL_ERROR);
    }
    return ERR_OK;
}

void AssessmentService::HandleBegin(const std::string &ticket, uint32_t operation)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment handle begin %{public}s, %{public}d", ticket.c_str(), operation);
    bool confirmed = false;
    std::vector<std::string> allowedApps;
    {
        std::unique_lock<std::mutex> lock(this->mutexSa_);
        if (this->examStatus_ != AssessmentExamStatus::CONFIRMING) {
            TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment is not confirmation state, ignore");
            return;
        }
        if (ticket_ != ticket) {
            TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment replay ticket: %{public}s, ignore", ticket.c_str());
            return;
        }

        if (operation == AssessmentConfirmationOperation::CANCEL) {
            CancelBeginLockedUnsafe();
        } else if (operation == AssessmentConfirmationOperation::CONFIRM) {
            ConfirmationBeginLockedUnsafe();
            condSa_.notify_all();
            confirmed = true;
            // Copy under the lock: a concurrent End() may clear the member
            // as soon as the lock is released.
            allowedApps = currentConfig_.allowedApps;
        } else {
            TAG_LOGI(AAFwkTag::ASSESSMENT, "assessment invalid operation: %{public}d, ignore", operation);
        }
    }

    if (confirmed) {
        ActivateProcessControl(allowedApps);
    }
}

void AssessmentService::ActivateProcessControl(const std::vector<std::string> &allowedApps)
{
    // Runs without mutexSa_ held: registering the call observer and disabling
    // the screen reader may block on external services.
    if (!processController_.Activate(allowedApps)) {
        // Environment could not be locked down (e.g. screen reader disable
        // failed): roll back was done in Activate, interrupt the assessment.
        TAG_LOGE(AAFwkTag::ASSESSMENT, "process control activation failed, interrupt assessment");
        std::unique_lock<std::mutex> lock(this->mutexSa_);
        if (callerToken_ != nullptr) {
            CallbackManager::GetInstance().OnInterrupted(
                callerToken_, static_cast<int32_t>(AssessmentErrorCode::SYSTEM_ERROR),
                AssessmentErrCodeToErrMsg(AssessmentErrorCode::SYSTEM_ERROR));
        }
        CleanupCurrentSession();
        ClearState();
        return;
    }

    // The session may have been ended (End/timeout) while activation was in
    // progress; deactivate so that no lockdown outlives the assessment.
    std::unique_lock<std::mutex> lock(this->mutexSa_);
    if (this->examStatus_ != AssessmentExamStatus::ACTIVE) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "session ended during activation, deactivate process control");
        processController_.Deactivate();
    }
}

void AssessmentService::ConfirmationBeginLockedUnsafe()
{
    auto cleanUp = [this]() {
        CallbackManager::GetInstance().OnBegin(callerToken_,
            static_cast<int32_t>(AssessmentErrorCode::SYSTEM_ERROR),
            AssessmentErrCodeToErrMsg(AssessmentErrorCode::SYSTEM_ERROR));
        CleanupCurrentSession();
    };
 
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> abilityManagerClient
        = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    ErrCode retSetAppList = abilityManagerClient->AddKioskApplicationList(currentConfig_.allowedApps);
    if (retSetAppList != ERR_OK) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assessment set application list fail, %{public}d", retSetAppList);
        cleanUp();
        return;
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "assessment set application list succcessfully");
    ErrCode retEnterKioskMode = abilityManagerClient->EnterKioskMode(callerToken_, 1);
    if (retEnterKioskMode != ERR_OK) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assessment EnterKioskMode fail, %{public}d", retEnterKioskMode);
        cleanUp();
        return;
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "assessment EnterKioskMode succcessfully");

    auto pt = std::chrono::system_clock::now() + std::chrono::milliseconds(currentConfig_.duration);
    endpointCheckPoint_
        = std::chrono::duration_cast<std::chrono::milliseconds>(pt.time_since_epoch()).count();
    isActive_ = true;
    examStatus_ = AssessmentExamStatus::ACTIVE;
    SaveState();
    CallbackManager::GetInstance().OnBegin(callerToken_,
        static_cast<int32_t>(AssessmentErrorCode::OK),
        AssessmentErrCodeToErrMsg(AssessmentErrorCode::OK));
}

void AssessmentService::CancelBeginLockedUnsafe()
{
    if (callerToken_ != nullptr) {
        CallbackManager::GetInstance().OnBegin(
            callerToken_, static_cast<int32_t>(AssessmentErrorCode::USER_CANCEL),
            AssessmentErrCodeToErrMsg(AssessmentErrorCode::USER_CANCEL));
    }
    CleanupCurrentSession();
    ClearState();
}

void AssessmentService::TimeoutLockedUnsafe()
{
    ErrCode ret = ExitKioskModeLockedUnsafe();
    if (ret != ERR_OK) {
        TAG_LOGI(AAFwkTag::ASSESSMENT, "assessment exitKioskMode for timeout fail");
        return;
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "assessment exitKioskMode for timeout successfully");
    if (callerToken_ != nullptr) {
        CallbackManager::GetInstance().OnInterrupted(
            callerToken_, static_cast<int32_t>(AssessmentErrorCode::TIMEOUT),
            AssessmentErrCodeToErrMsg(AssessmentErrorCode::TIMEOUT));
    }
    CleanupCurrentSession();
    ClearState();
}

ErrCode AssessmentService::ExitKioskModeLockedUnsafe()
{
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> abilityManagerClient
        = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    ErrCode retExitKioskMode = abilityManagerClient->ExitKioskMode(callerToken_);
    if (retExitKioskMode != ERR_OK) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assessment exitKioskMode failed, %{public}d", retExitKioskMode);
        return retExitKioskMode;
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "assessment exitKioskMode successfully");
    ErrCode retDelAppList = abilityManagerClient->DeleteKioskApplicationList(currentConfig_.allowedApps);
    if (retDelAppList != ERR_OK) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assement deleteKioskApplicationList failed, %{public}d", retDelAppList);
    }
    return ERR_OK;
}

void AssessmentService::AppDieHandle(const std::string &bundleName)
{
    std::unique_lock<std::mutex> lock(this->mutexSa_);
    if (!isActive_) {
        return;
    }
    if (bundleName_ != bundleName) {
        return;
    }
    ErrCode ret = ExitKioskModeLockedUnsafe();
    if (ret != ERR_OK) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assement exit kiosk failed, %{public}d", ret);
    }

    sptr<IRemoteObject> caller = callerToken_;
    if (caller != nullptr) {
        CallbackManager::GetInstance().OnEnd(caller);
    }
    CleanupCurrentSession();
    ClearState();
    TAG_LOGI(AAFwkTag::ASSESSMENT, "assement clear app: %{public}s for app died", bundleName.c_str());
    Destroy();
}

void AssessmentService::EnvAnomalyLockedUnsafe()
{
    if (!isActive_ || callerToken_ == nullptr) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "Assessment not active");
        return;
    }

    ErrCode ret = ExitKioskModeLockedUnsafe();
    if (ret != ERR_OK) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assessment exitKioskMode for end fail");
        return;
    }
    CallbackManager::GetInstance().OnInterrupted(
        callerToken_, static_cast<int32_t>(AssessmentErrorCode::ENV_ANOMALY),
        AssessmentErrCodeToErrMsg(AssessmentErrorCode::ENV_ANOMALY));
    CleanupCurrentSession();
    ClearState();
    Destroy();
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
    } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_ENTER_HIBERNATE) {
        TAG_LOGI(AAFwkTag::DEFAULT, "System entered HIBERNATE");
    } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_EXIT_HIBERNATE) {
        TAG_LOGI(AAFwkTag::DEFAULT, "System exited HIBERNATE");
    } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SHUTDOWN) {
        TAG_LOGI(AAFwkTag::DEFAULT, "System shutdown");
    } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_POWER_SAVE_MODE_CHANGED) {
        uint32_t code = eventData.GetCode();
        TAG_LOGI(AAFwkTag::ASSESSMENT,
            "assessment POWER_SAVE_MODE_CHANGED, mode: %{public}u", code);
        if (code == static_cast<uint32_t>(OHOS::PowerMgr::PowerMode::EXTREME_POWER_SAVE_MODE)) {
            std::unique_lock<std::mutex> lock(this->mutexSa_);
            this->EnvAnomalyLockedUnsafe();
        }
    } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_BOOT_COMPLETED) {
        TAG_LOGI(AAFwkTag::DEFAULT, "System BOOT_COMPLETED");
    }
}

void AssessmentService::AncoStateChangeCallback(const char *key, const char *value, void *context)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "AncoStateChangeCallback called, key: %{public}s, value: %{public}s", key, value);
    AssessmentService* service = reinterpret_cast<AssessmentService*>(context);
    if (service == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "AncoStateChangeCallback called, invalid service");
        return;
    }
    TAG_LOGI(AAFwkTag::DEFAULT,
             "AncoStateChangeCallback called, isWaittingAncoActive_: %{public}s",
             service->isWaittingAncoActive_ ? "true" : "false");
    bool isAncoStateKey = (strcmp(key, PARAM_ANCO_STATE) == 0);
    bool isAncoActive = (strcmp(value, "0") == 0);
    if (service->isWaittingAncoActive_ && isAncoStateKey && isAncoActive) {
        system::SetParameter(PARAM_ASSESSMENT_IS_ACTIVE, service->isActive_ ? "true" : "false");
        TAG_LOGI(AAFwkTag::DEFAULT,
                 "AncoStateChangeCallback called, sync anco state success, assessment status: %{public}s",
                 service->isActive_ ? "true" : "fasle");
        if (!service->isActive_) {
            service->Destroy();
        }
        service->isWaittingAncoActive_ = false;
    }
}

void AssessmentService::OnSwitchEvent(std::shared_ptr<OHOS::MMI::SwitchEvent> event)
{
    if (event == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "SwitchEvent is null");
        return;
    }
    if (event -> GetSwitchType() != OHOS::MMI::SwitchEvent::SWITCH_LID) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Not LID Event");
        return;
    }
    int32_t switchValue = event->GetSwitchValue();
    if (switchValue == OHOS::MMI::SwitchEvent::SWITCH_ON) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Lid_Open");
    } else {
        TAG_LOGE(AAFwkTag::DEFAULT, "Lid_Close");
        std::unique_lock<std::mutex> lock(this->mutexSa_);
        this->EnvAnomalyLockedUnsafe();
    }
}

std::string AssessmentService::GetAssessmentBundleName()
{
    std::lock_guard<std::mutex> lock(this->mutexSa_);
    TAG_LOGI(AAFwkTag::ASSESSMENT, "GetAssessmentBundleName called");
    return bundleName_;
}

AssessmentConfig AssessmentService::GetAssessmentCurrentConfig()
{
    std::lock_guard<std::mutex> lock(this->mutexSa_);
    TAG_LOGI(AAFwkTag::ASSESSMENT, "GetAssessmentCurrentConfig called");
    return currentConfig_;
}
}  // namespace AAFwk
}  // namespace OHOS
