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

#include <sstream>
#include <sys/time.h>
#include <input_manager.h>
#include "assessment_service.h"
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
#include "app_mgr_client.h"
#include "app_mgr_interface.h"
#include "singleton.h"
#include "app_mgr_util.h"
#include "ipc_skeleton.h"
#include "ability_manager_client.h"
#include "power_mode_info.h"
#include "power_mgr_client.h"

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
const uint64_t SA_IDLE_MAX_INTERVAL = 5 * 60 * 1000;
const int32_t TICKET_LEN = 16;

const char* const ASSESSMENT_COMMON_EVENT_CONFIRMATION = "assessment.event.confirmation";
const char* const ASSESSMENT_COMMON_EVENT_ACK = "assessment.event.ack";
const std::string SCENEBOARD_BUNDLE_NAME = "com.ohos.sceneboard";
const std::string SCENEBOARD_ABILITY_NAME = "com.ohos.sceneboard.systemdialog";
const std::string SYSTEM_UI_BUNDLE_NAME = "com.ohos.commondialog";
const std::string SYSTEM_UI_ABILITY_NAME = "AssessmentServiceDialogAbility";

const char* const TRACE_ERR_REASON_NO_ERROR = "NO_ERROR";
const char* const TRACE_ERR_REASON_INVALID_PARAMS = "INVALID_PARAMS";
const char* const TRACE_ERR_REASON_PERMISSION_DENIED = "PERMISSION_DENIED";
const char* const TRACE_ERR_REASON_DEVICE_NOT_SUPPORTED = "DEVICE_NOT_SUPPORTED";
const char* const TRACE_ERR_REASON_EXAM_ALREADY_STARTED = "EXAM_ALREADY_STARTED";
const char* const TRACE_ERR_REASON_USER_CONFIRM = "USER_CONFIRM";
const char* const TRACE_ERR_REASON_USER_CANCEL = "USER_CANCEL";
const char* const TRACE_ERR_REASON_CALL_KIOSK_FAIL = "CALL_KIOSK_FAIL";
const char* const TRACE_ERR_REASON_SAVE_SYSPARAM_FAIL = "SAVE_SYSPARAM_FAIL";
const char* const TRACE_ERR_REASON_NO_EXAM = "NO_EXAM";
const char* const TRACE_ERR_REASON_RESET_SYSPARAM_FAIL = "RESET_SYSPARAM_FAIL";
const char* const TRACE_ERR_REASON_NON_OWNER_END = "NON_OWNER_END";
const char* const TRACE_ERR_REASON_LOCK_SCREENOFF_FAILED = "LOCK_SCREENOFF_FAILED";
const char* const TRACE_ERR_REASON_KILL_ANCO_APP_FAILED = "KILL_ANCO_APP_FAILED";
const char* const TRACE_ERR_REASON_APP_DIED = "APP_DIED";

const char* const TRACE_EXIT_REASON_TIMEOUT = "TIMEOUT";
const char* const TRACE_EXIT_REASON_USER_INITIATED_EXIT = "USER_INITIATED_EXIT";
const char* const TRACE_EXIT_REASON_ENTER_HIBERNATE = "ENTER_HIBERNATE";
const char* const TRACE_EXIT_REASON_ENTER_POWER_SAVE_MODE = "ENTER_POWER_SAVE_MODE";
const char* const TRACE_EXIT_REASON_SHUTDOWN = "SHUTDOWN";
const char* const TRACE_EXIT_REASON_DEVICE_REBOOT = "DEVICE_REBOOT";
const char* const TRACE_EXIT_REASON_LID_CLOSED = "LID_CLOSED";

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

    if (!InitSubsystems()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "InitSubsystems fail");
        return false;
    }
    // Heavy dependency init (extension-loader dlopen, DeviceManager and
    // CallManager IPC) runs on the service thread so that SA OnStart is not
    // blocked on external services. Until it completes, the affected env
    // checks are skipped (fail-open, documented in EnvChecker).
    if (!this->envChecker_.Init()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "EnvChecker init incomplete, some env checks may be skipped");
        return false;
    }
    if (!this->processController_.Init()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "ProcessController init failed");
        return false;
    }

    this->running_ = true;
    this->thread_ = std::thread([this]() {
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
    int ret = system::SetParameter(PARAM_ASSESSMENT_IS_ACTIVE, isActive_ ? "true" : "false");
    if (ret != 0) {
        EnterExamParam params = BuildEnterExamParamLockUnsafe();
        AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_SAVE_SYSPARAM_FAIL);
    }
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
        int32_t retCode = envChecker_.RestrictAncoApp();
        constexpr int32_t RESTRICT_ANCO_APP_FAILED = 1;
        if (retCode == RESTRICT_ANCO_APP_FAILED) {
            EnterExamParam params = BuildEnterExamParamLockUnsafe();
            AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_KILL_ANCO_APP_FAILED);
        }
    }
    TAG_LOGD(AAFwkTag::DEFAULT, "State saved");
}

void AssessmentService::ClearState()
{
    int ret = system::SetParameter(PARAM_ASSESSMENT_IS_ACTIVE, "false");
    if (ret != 0) {
        ExitExamParam params = BuildExitExamParamLockUnsafe();
        AssessmentEventPublisher::PublishExitExamModeEvent(
            params, TRACE_ERR_REASON_RESET_SYSPARAM_FAIL, TRACE_ERR_REASON_RESET_SYSPARAM_FAIL);
    }
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
    ErrCode retRestrictScreenOff = RestrictScreenOff(false);
    if (retRestrictScreenOff != ERR_OK) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assessment RestrictScreenOff fail, %{public}d", retRestrictScreenOff);
        ExitExamParam params = BuildExitExamParamLockUnsafe();
        AssessmentEventPublisher::PublishExitExamModeEvent(
            params, TRACE_ERR_REASON_LOCK_SCREENOFF_FAILED, TRACE_ERR_REASON_LOCK_SCREENOFF_FAILED);
    }
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

bool AssessmentService::CheckBeginPreconditions(const sptr<IRemoteObject> &token, uint32_t duration,
    const std::vector<std::string> &allowedApps, const sptr<IRemoteObject> &callback, int32_t &errCode)
{
    EnterExamParam params;
    params.bundleName = allowedApps.empty() ? "" : allowedApps.back();
    params.duration = duration;
    params.allowedApps = allowedApps;
    params.examStartTime = static_cast<long long>(AssessmentServiceUtils::GetCurrentAssessmentTimeStamp());
    if (token == nullptr || callback == nullptr || allowedApps.empty()) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment invalid params");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_PARAMS);
        AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_INVALID_PARAMS);
        return false;
    }
    if (!AssessmentServiceUtils::CheckDeviceTypeSupported()) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment device not supported");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_CAPABILITY_NOT_SUPPORT);
        AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_DEVICE_NOT_SUPPORTED);
        return false;
    }
    if (!AssessmentServiceUtils::VerifyCallingPermission(PERMISSION_ASSESSMENT_CONFIGURATION)) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "no permission: ohos.permission.ASSESSMENT_CONFIGURATION");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_PERMISSION_DENIED);
        AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_PERMISSION_DENIED);
        return false;
    }
    return true;
}

bool AssessmentService::CheckEndPreconditionsLocked(const sptr<IRemoteObject> &token, int32_t &errCode)
{
    ExitExamParam params = BuildExitExamParamLockUnsafe();
    if (token == nullptr) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment invalid params");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_PARAMS);
        AssessmentEventPublisher::PublishExitExamModeEvent(
            params, TRACE_ERR_REASON_INVALID_PARAMS, TRACE_ERR_REASON_INVALID_PARAMS);
        return false;
    }
    
    if (!AssessmentServiceUtils::CheckDeviceTypeSupported()) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment device not supported");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_CAPABILITY_NOT_SUPPORT);
        AssessmentEventPublisher::PublishExitExamModeEvent(
            params, TRACE_ERR_REASON_DEVICE_NOT_SUPPORTED, TRACE_ERR_REASON_DEVICE_NOT_SUPPORTED);
        return false;
    }
    if (!AssessmentServiceUtils::VerifyCallingPermission(PERMISSION_ASSESSMENT_CONFIGURATION)) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "no permission: ohos.permission.ASSESSMENT_CONFIGURATION");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_PERMISSION_DENIED);
        AssessmentEventPublisher::PublishExitExamModeEvent(
            params, TRACE_ERR_REASON_PERMISSION_DENIED, TRACE_ERR_REASON_PERMISSION_DENIED);
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

    if (!CheckBeginPreconditions(token, duration, allowedApps, callback, errCode)) {
        return ERR_OK;
    }

    std::unique_lock<std::mutex> lock(this->mutexSa_);
    RemarkSaIdleLockedUnsafe();
    if (isActive_) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "Assessment already active");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_ASSESSMENT_ALREADY_ACTIVE);
        EnterExamParam params = BuildEnterExamParamLockUnsafe();
        AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_EXAM_ALREADY_STARTED);
        return ERR_OK;
    }
    if (!running_) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "Assessment reject accept request");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INTERNAL_ERROR);
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
    errCode = ERR_OK;

    bool uiComponentExists = AssessmentServiceUtils::IsSystemDialogAvailable(
        SYSTEM_UI_BUNDLE_NAME, SYSTEM_UI_ABILITY_NAME);
    if (uiComponentExists) {
        auto invokeRet = InvokeSystemDialog();
        TAG_LOGI(AAFwkTag::ASSESSMENT, "Begin phrase invoke system dialog, ret: %{public}d", invokeRet);
        if (invokeRet == ERR_OK) {
            return ERR_OK;
        }
    }

    ConfirmationBeginLockedUnsafe();
    condSa_.notify_all();

    TAG_LOGI(AAFwkTag::ASSESSMENT, "Begin over, direct to exam state");
    return ERR_OK;
}

ErrCode AssessmentService::End(const sptr<IRemoteObject> &token, int32_t &errCode)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "End called");
    std::unique_lock<std::mutex> lock(this->mutexSa_);
    if (!CheckEndPreconditionsLocked(token, errCode)) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "check fail, so end fail");
        return ERR_OK;
    }
    RemarkSaIdleLockedUnsafe();
    if (!isActive_) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "Assessment not active");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_ASSESSMENT_NOT_ACTIVE);
        ExitExamParam params = BuildExitExamParamLockUnsafe();
        AssessmentEventPublisher::PublishExitExamModeEvent(
            params, TRACE_ERR_REASON_NO_EXAM, TRACE_ERR_REASON_NO_EXAM);
        return ERR_OK;
    }
    int32_t callingUid = IPCSkeleton::GetCallingUid();
    if (callingUid_ != callingUid) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "Assessment be called for other app");
        errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_OPERATION);
        ExitExamParam params = BuildExitExamParamLockUnsafe();
        AssessmentEventPublisher::PublishExitExamModeEvent(
            params, TRACE_ERR_REASON_NON_OWNER_END, TRACE_ERR_REASON_NON_OWNER_END);
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
    ExitExamParam params = BuildExitExamParamLockUnsafe();
    AssessmentEventPublisher::PublishExitExamModeEvent(
        params, TRACE_EXIT_REASON_USER_INITIATED_EXIT, TRACE_ERR_REASON_NO_ERROR);
    CleanupCurrentSession();
    ClearState();
    errCode = ERR_OK;
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
    RemarkSaIdleLockedUnsafe();
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
    RemarkSaIdleLockedUnsafe();
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

        auto start = std::chrono::steady_clock::now();
        condSa_.wait_for(lock, std::chrono::milliseconds(timeout));

        this->CheckEndpointAndExecuteTaskLockedUnsafe();

        auto end = std::chrono::steady_clock::now();
        auto delta = static_cast<int32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
        delta = std::min(std::max(0, delta), timeout);
        this->CheckAndHandleSaIdleLockedUnsafe(delta);

        TAG_LOGI(AAFwkTag::DEFAULT, "Assessment execute once");
    }
    Destroy();
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
    }
}

void AssessmentService::CheckAndHandleSaIdleLockedUnsafe(int32_t delta)
{
    if (!isActive_ && examStatus_ == AssessmentExamStatus::IDLE) {
        accumulateIdleTime_ += delta;
        if (accumulateIdleTime_ > SA_IDLE_MAX_INTERVAL) {
            this->running_ = false;
        }
    } else {
        accumulateIdleTime_ = 0;
    }
}

void AssessmentService::RemarkSaIdleLockedUnsafe()
{
    accumulateIdleTime_ = 0;
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

void AssessmentService::PostCommonEventForSystemDialog(const std::string &ticket)
{
    OHOS::EventFwk::CommonEventPublishInfo publishInfo;
    OHOS::AAFwk::Want want;
    want.SetAction(ASSESSMENT_COMMON_EVENT_ACK);
    want.SetParam("ticket", ticket);
    OHOS::EventFwk::CommonEventData event(want);
    OHOS::EventFwk::CommonEventManager::PublishCommonEvent(event, publishInfo, nullptr);
}

int32_t AssessmentService::InvokeSystemDialog()
{
    OHOS::AAFwk::Want want;
    want.SetElementName(SCENEBOARD_BUNDLE_NAME, SCENEBOARD_ABILITY_NAME);
    std::string parameters =
        "{\"ability.want.params.uiExtensionType\":\"sysDialog/common\","
        "\"ticket\":\"" + ticket_ + "\",\"bundleName\":\"" + bundleName_ + "\"}";
    std::string ticket = ticket_;
    sptr<AssessmentAbilityConnection> connection = sptr<AssessmentAbilityConnection>(
        new (std::nothrow)AssessmentAbilityConnection(
            SYSTEM_UI_BUNDLE_NAME, SYSTEM_UI_ABILITY_NAME, parameters,
            [ticket, this]() {
                this->BeginDialogSystemError(ticket);
            }));
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
    if (operation != AssessmentConfirmationOperation::CANCEL
        && operation != AssessmentConfirmationOperation::CONFIRM) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assessment invalid operation: %{public}d, ignore", operation);
        return;
    }
    std::unique_lock<std::mutex> lock(this->mutexSa_);
    if (this->examStatus_ != AssessmentExamStatus::CONFIRMING) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment is not confirmation state, ignore");
        return;
    }
    if (ticket_ != ticket) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment replay ticket: %{public}s, ignore", ticket.c_str());
        return;
    }

    PostCommonEventForSystemDialog(ticket);
    if (operation == AssessmentConfirmationOperation::CANCEL) {
        EnterExamParam params = BuildEnterExamParamLockUnsafe();
        AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_USER_CANCEL);
        CancelBeginLockedUnsafe();
    } else if (operation == AssessmentConfirmationOperation::CONFIRM) {
        ConfirmationBeginLockedUnsafe();
        EnterExamParam params = BuildEnterExamParamLockUnsafe();
        AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_USER_CONFIRM);
        condSa_.notify_all();
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

    if (!envChecker_.CheckAll()) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "Environment check failed after user confirmation");
        CallbackManager::GetInstance().OnBegin(callerToken_,
            static_cast<int32_t>(AssessmentErrorCode::ENV_ANOMALY),
            AssessmentErrCodeToErrMsg(AssessmentErrorCode::ENV_ANOMALY));
        CleanupCurrentSession();
        return;
    }

    if (!processController_.Activate(currentConfig_.allowedApps)) {
        TAG_LOGE(AAFwkTag::ASSESSMENT, "process control activation failed, interrupt assessment");
        cleanUp();
        return;
    }

    if (EnterKioskModeLockedUnsafe() != ERR_OK) {
        cleanUp();
        return;
    }

    auto pt = std::chrono::system_clock::now() + std::chrono::milliseconds(currentConfig_.duration);
    endpointCheckPoint_
        = std::chrono::duration_cast<std::chrono::milliseconds>(pt.time_since_epoch()).count();
    isActive_ = true;
    examStatus_ = AssessmentExamStatus::ACTIVE;
    SaveState();
    CallbackManager::GetInstance().OnBegin(callerToken_,
        static_cast<int32_t>(AssessmentErrorCode::OK),
        AssessmentErrCodeToErrMsg(AssessmentErrorCode::OK));
    EnterExamParam params = BuildEnterExamParamLockUnsafe();
    AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_NO_ERROR);
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
    ExitExamParam params = BuildExitExamParamLockUnsafe();
    AssessmentEventPublisher::PublishExitExamModeEvent(
        params, TRACE_EXIT_REASON_TIMEOUT, TRACE_ERR_REASON_NO_ERROR);
    CleanupCurrentSession();
    ClearState();
}

ErrCode AssessmentService::EnterKioskModeLockedUnsafe()
{
    ErrCode retRestrictScreenOff = RestrictScreenOff(true);
    if (retRestrictScreenOff != ERR_OK) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assessment RestrictScreenOff fail, %{public}d", retRestrictScreenOff);
        EnterExamParam params = BuildEnterExamParamLockUnsafe();
        AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_LOCK_SCREENOFF_FAILED);
        return retRestrictScreenOff;
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "assessment retRestrictScreenOff succcessfully");

    EnterExamParam params = BuildEnterExamParamLockUnsafe();
    std::vector<std::string> whiteAppList = GetFinalAppList();
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> abilityManagerClient
        = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    ErrCode retSetAppList = abilityManagerClient->AddKioskApplicationList(whiteAppList);
    if (retSetAppList != ERR_OK) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assessment set application list fail, %{public}d", retSetAppList);
        AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_CALL_KIOSK_FAIL);
        return retSetAppList;
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "assessment set application list succcessfully");
    ErrCode retEnterKioskMode = abilityManagerClient->EnterKioskMode(callerToken_, 1);
    if (retEnterKioskMode != ERR_OK) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assessment EnterKioskMode fail, %{public}d", retEnterKioskMode);
        AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_CALL_KIOSK_FAIL);
        return retEnterKioskMode;
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "assessment EnterKioskMode succcessfully");
    return ERR_OK;
}

ErrCode AssessmentService::ExitKioskModeLockedUnsafe()
{
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> abilityManagerClient
        = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    ErrCode retExitKioskMode = abilityManagerClient->ExitKioskMode(callerToken_);
    if (retExitKioskMode != ERR_OK) {
        TAG_LOGW(AAFwkTag::ASSESSMENT, "assessment exitKioskMode failed, %{public}d", retExitKioskMode);
        ExitExamParam params = BuildExitExamParamLockUnsafe();
        AssessmentEventPublisher::PublishExitExamModeEvent(
            params, TRACE_ERR_REASON_CALL_KIOSK_FAIL, TRACE_ERR_REASON_CALL_KIOSK_FAIL);
        return retExitKioskMode;
    }
    TAG_LOGI(AAFwkTag::ASSESSMENT, "assessment exitKioskMode successfully");
    std::vector<std::string> whiteAppList = GetFinalAppList();
    ErrCode retDelAppList = abilityManagerClient->DeleteKioskApplicationList(whiteAppList);
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
    ExitExamParam params = BuildExitExamParamLockUnsafe();
    AssessmentEventPublisher::PublishExitExamModeEvent(
        params, TRACE_ERR_REASON_APP_DIED, TRACE_ERR_REASON_APP_DIED);
    CleanupCurrentSession();
    ClearState();
    TAG_LOGI(AAFwkTag::ASSESSMENT, "assement clear app: %{public}s for app died", bundleName.c_str());
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
}

void AssessmentService::DispatchEvent(const OHOS::EventFwk::CommonEventData& eventData)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment dispatch_event");
    const OHOS::AAFwk::Want& want = eventData.GetWant();
    std::string action = want.GetAction();
    if (action == ASSESSMENT_COMMON_EVENT_CONFIRMATION) {
        std::string ticket = want.GetStringParam("ticket");
        int operation = want.GetIntParam("operation", 2);
        HandleBegin(ticket, operation);
        TAG_LOGI(AAFwkTag::DEFAULT, "assessment receive ticket:%{public}s, operation:%{public}d for handle begin",
            ticket.c_str(), operation);
    } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_ENTER_HIBERNATE) {
        std::unique_lock<std::mutex> lock(this->mutexSa_);
        ExitExamParam params = BuildExitExamParamLockUnsafe();
        AssessmentEventPublisher::PublishExitExamModeEvent(
            params, TRACE_EXIT_REASON_ENTER_HIBERNATE, TRACE_ERR_REASON_NO_ERROR);
        TAG_LOGI(AAFwkTag::DEFAULT, "System entered HIBERNATE");
    } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_EXIT_HIBERNATE) {
        std::unique_lock<std::mutex> lock(this->mutexSa_);
        EnterExamParam params = BuildEnterExamParamLockUnsafe();
        AssessmentEventPublisher::PublishEnterExamModeEvent(params, TRACE_ERR_REASON_NO_ERROR);
        TAG_LOGI(AAFwkTag::DEFAULT, "System exited HIBERNATE");
    } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SHUTDOWN) {
        std::unique_lock<std::mutex> lock(this->mutexSa_);
        ExitExamParam params = BuildExitExamParamLockUnsafe();
        AssessmentEventPublisher::PublishExitExamModeEvent(
            params, TRACE_EXIT_REASON_SHUTDOWN, TRACE_ERR_REASON_NO_ERROR);
        TAG_LOGI(AAFwkTag::DEFAULT, "System shutdown");
    } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_POWER_SAVE_MODE_CHANGED) {
        uint32_t code = eventData.GetCode();
        TAG_LOGI(AAFwkTag::ASSESSMENT,
            "assessment POWER_SAVE_MODE_CHANGED, mode: %{public}u", code);
        if (code == static_cast<uint32_t>(OHOS::PowerMgr::PowerMode::EXTREME_POWER_SAVE_MODE)) {
            std::unique_lock<std::mutex> lock(this->mutexSa_);
            ExitExamParam params = BuildExitExamParamLockUnsafe();
            AssessmentEventPublisher::PublishExitExamModeEvent(
                params, TRACE_EXIT_REASON_ENTER_POWER_SAVE_MODE, TRACE_ERR_REASON_NO_ERROR);
            this->EnvAnomalyLockedUnsafe();
        }
    } else if (action == OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_BOOT_COMPLETED) {
        std::unique_lock<std::mutex> lock(this->mutexSa_);
        ExitExamParam params = BuildExitExamParamLockUnsafe();
        AssessmentEventPublisher::PublishExitExamModeEvent(
            params, TRACE_EXIT_REASON_DEVICE_REBOOT, TRACE_ERR_REASON_NO_ERROR);
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
    std::unique_lock<std::mutex> lock(service->mutexSa_);
    TAG_LOGI(AAFwkTag::DEFAULT,
             "AncoStateChangeCallback called, isWaittingAncoActive_: %{public}s",
             service->isWaittingAncoActive_ ? "true" : "false");
    bool isAncoStateKey = (strcmp(key, PARAM_ANCO_STATE) == 0);
    bool isAncoActive = (strcmp(value, "0") == 0);
    if (service->isWaittingAncoActive_ && isAncoStateKey && isAncoActive) {
        system::SetParameter(PARAM_ASSESSMENT_IS_ACTIVE, service->isActive_ ? "true" : "false");
        TAG_LOGI(AAFwkTag::DEFAULT,
                 "AncoStateChangeCallback called, sync anco state success, assessment status: %{public}s",
                 service->isActive_ ? "true" : "false");
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
        ExitExamParam params = BuildExitExamParamLockUnsafe();
        AssessmentEventPublisher::PublishExitExamModeEvent(
            params, TRACE_EXIT_REASON_LID_CLOSED, TRACE_ERR_REASON_NO_ERROR);
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

bool AssessmentService::GetEnvCheckResult()
{
    std::lock_guard<std::mutex> lock(this->mutexSa_);
    TAG_LOGI(AAFwkTag::ASSESSMENT, "GetEnvCheckResult called");
    bool envCheckResult = envChecker_.CheckAll();
    return envCheckResult;
}

void AssessmentService::BeginDialogSystemError(const std::string &ticket)
{
    std::unique_lock<std::mutex> lock(this->mutexSa_);
    if (this->ticket_ != ticket) {
        return;
    }
    if (isActive_) {
        return;
    }
    if (callerToken_ != nullptr) {
        CallbackManager::GetInstance().OnBegin(
            callerToken_, static_cast<int32_t>(AssessmentErrorCode::SYSTEM_ERROR),
            AssessmentErrCodeToErrMsg(AssessmentErrorCode::SYSTEM_ERROR));
    }
    CleanupCurrentSession();
    ClearState();
    TAG_LOGI(AAFwkTag::ASSESSMENT, "BeginDialogSystemError called");
}

ErrCode AssessmentService::RestrictScreenOff(bool enable)
{
    auto &powerClient = PowerMgr::PowerMgrClient::GetInstance();
    PowerMgr::PowerErrors powerError = powerClient.LockScreenAfterTimingOut(!enable, !enable);
    if (powerError != PowerMgr::PowerErrors::ERR_OK) {
        return static_cast<int32_t>(AssessmentErrorCode::SYSTEM_ERROR);
    }
    powerError = powerClient.SetInterfaceCallFilteringStrategy(enable ?
        PowerMgr::InterfaceCallFilteringStrategy::SUSPEND_DEVICE_FILTERING :
        PowerMgr::InterfaceCallFilteringStrategy::SUSPEND_DEVICE_NOT_FILTERING);
    if (powerError != PowerMgr::PowerErrors::ERR_OK) {
        return static_cast<int32_t>(AssessmentErrorCode::SYSTEM_ERROR);
    }
    powerError = powerClient.SetLidFilteringStrategy(enable ?
        PowerMgr::LidFilteringStrategy::LID_CLOSE_FILTERING :
        PowerMgr::LidFilteringStrategy::LID_CLOSE_NOT_FILTERING);
    if (powerError != PowerMgr::PowerErrors::ERR_OK) {
        return static_cast<int32_t>(AssessmentErrorCode::SYSTEM_ERROR);
    }
    powerError = powerClient.SetPowerKeyFilteringStrategy(enable ?
        PowerMgr::PowerKeyFilteringStrategy::POWER_KEY_UP_SHORT_PRESS_FILTERING :
        PowerMgr::PowerKeyFilteringStrategy::POWER_KEY_UP_SHORT_PRESS_NOT_FILTERING);
    if (powerError != PowerMgr::PowerErrors::ERR_OK) {
        return static_cast<int32_t>(AssessmentErrorCode::SYSTEM_ERROR);
    }
    return ERR_OK;
}

std::vector<std::string> AssessmentService::GetFinalAppList()
{
    std::vector<std::string> result = currentConfig_.allowedApps;
    const std::string PARAM_IME_KEY = "persist.sys.default_ime";
    std::string inputName = system::GetParameter(PARAM_IME_KEY, "");
    if (!inputName.empty()) {
        size_t pos = inputName.find("/");
        if (pos != std::string::npos) {
            inputName = inputName.substr(0, pos);
        }
        result.push_back(inputName);
        TAG_LOGE(AAFwkTag::ASSESSMENT, "assessment fetch ime default: %{public}s", inputName.c_str());
    }
    return result;
}

EnterExamParam AssessmentService::BuildEnterExamParamLockUnsafe()
{
    EnterExamParam params;
    params.bundleName = bundleName_;
    params.examId = static_cast<long long>(currentConfig_.examId);
    params.examStartTime = static_cast<long long>(currentConfig_.examStartTime);
    params.duration = static_cast<long long>(currentConfig_.duration);
    params.allowedApps = currentConfig_.allowedApps;
    params.envCheckResult = false;
    return params;
}

ExitExamParam AssessmentService::BuildExitExamParamLockUnsafe()
{
    ExitExamParam params;
    params.bundleName = bundleName_;
    params.examId = static_cast<long long>(currentConfig_.examId);
    params.examStartTime = static_cast<long long>(currentConfig_.examStartTime);
    return params;
}
}  // namespace AAFwk
}  // namespace OHOS
