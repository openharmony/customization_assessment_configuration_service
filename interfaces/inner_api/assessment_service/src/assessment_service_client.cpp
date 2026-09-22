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

#include "assessment_service_client.h"

#include "assessment_service_load_callback.h"
#include "hilog_tag_wrapper.h"
#include "hitrace_meter.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "assessment_api_error_code.h"

namespace OHOS {
namespace AAFwk {
namespace {
constexpr int32_t LOAD_SA_TIMEOUT_MS = 4000;
}

std::shared_ptr<AssessmentServiceClient> AssessmentServiceClient::GetInstance()
{
    TAG_LOGI(AAFwkTag::DEFAULT, "GetInstance called");
    static std::shared_ptr<AssessmentServiceClient> instance = std::make_shared<AssessmentServiceClient>();
    return instance;
}

ErrCode AssessmentServiceClient::Begin(const sptr<IRemoteObject> &token, uint32_t duration,
                                       const std::vector<std::string> &allowedApps,
                                       const sptr<IRemoteObject> &callback)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    TAG_LOGI(AAFwkTag::DEFAULT, "Begin called, duration: %{public}d", duration);

    auto assessmentService = GetAssessmentProxyWithCheck();
    if (assessmentService == nullptr) {
        return static_cast<ErrCode>(AssessmentApiErrCode::ERR_INTERNAL_ERROR);
    }

    int32_t errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INTERNAL_ERROR);
    assessmentService->Begin(token, duration, allowedApps, callback, errCode);
    return static_cast<ErrCode>(errCode);
}

ErrCode AssessmentServiceClient::End(const sptr<IRemoteObject> &token)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    TAG_LOGI(AAFwkTag::DEFAULT, "End called");

    auto assessmentService = GetAssessmentProxyWithCheck();
    if (assessmentService == nullptr) {
        return static_cast<ErrCode>(AssessmentApiErrCode::ERR_INTERNAL_ERROR);
    }

    int32_t errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INTERNAL_ERROR);
    assessmentService->End(token, errCode);
    return static_cast<ErrCode>(errCode);
}

ErrCode AssessmentServiceClient::IsActive(bool &isActive)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    auto assessmentService = GetAssessmentProxyWithCheck();
    if (assessmentService == nullptr) {
        isActive = false;
        return static_cast<ErrCode>(AssessmentApiErrCode::ERR_INTERNAL_ERROR);
    }
    int32_t errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INTERNAL_ERROR);
    assessmentService->IsActive(isActive, errCode);
    return static_cast<ErrCode>(errCode);
}

ErrCode AssessmentServiceClient::GetConfiguration(uint32_t &duration, std::vector<std::string> &allowedApps)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    auto assessmentService = GetAssessmentProxyWithCheck();
    if (assessmentService == nullptr) {
        duration = 0;
        allowedApps.clear();
        return static_cast<ErrCode>(AssessmentApiErrCode::ERR_INTERNAL_ERROR);
    }
    int32_t errCode = static_cast<int32_t>(AssessmentApiErrCode::ERR_INTERNAL_ERROR);
    assessmentService->GetConfiguration(duration, allowedApps, errCode);
    return static_cast<ErrCode>(errCode);
}

sptr<IAssessmentService> AssessmentServiceClient::GetAssessmentProxyWithCheck()
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    auto assessmentService = GetAssessmentProxy();
    if (assessmentService == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "GetAssessmentProxy failed");
        return nullptr;
    }
    return assessmentService;
}

sptr<IAssessmentService> AssessmentServiceClient::GetAssessmentProxy()
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    auto assessmentService = GetAssessmentService();
    if (assessmentService != nullptr) {
        TAG_LOGI(AAFwkTag::DEFAULT, "Assessment service already loaded, reuse proxy");
        return assessmentService;
    }

    if (!LoadAssessmentService()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "LoadAssessmentService failed");
        return nullptr;
    }

    assessmentService = GetAssessmentService();
    if (assessmentService == nullptr || assessmentService->AsObject() == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "GetAssessmentService failed");
        return nullptr;
    }

    auto self = weak_from_this();
    auto onClearProxyCallback = [self](const wptr<IRemoteObject> &remote) {
        auto impl = self.lock();
        if (impl) {
            impl->ClearProxy();
        }
    };

    sptr<AssessmentDeathRecipient> recipient = new (std::nothrow) AssessmentDeathRecipient(self, onClearProxyCallback);
    if (recipient != nullptr) {
        assessmentService->AsObject()->AddDeathRecipient(recipient);
    }

    return assessmentService;
}

void AssessmentServiceClient::ClearProxy()
{
    TAG_LOGI(AAFwkTag::DEFAULT, "ClearProxy called");
    std::lock_guard<std::mutex> lock(mutex_);
    assessmentService_ = nullptr;
}

void AssessmentServiceClient::AssessmentDeathRecipient::OnRemoteDied([[maybe_unused]] const wptr<IRemoteObject> &remote)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment service died");
    if (proxy_ != nullptr) {
        proxy_(remote);
    }

    auto impl = client_.lock();
    if (impl) {
        impl->ClearProxy();
        {
            std::unique_lock<std::mutex> lock(impl->loadSaMutex_);
            impl->loadSaFinished_ = false;
            impl->loadSaCondation_.notify_one();
        }
    }

    TAG_LOGI(AAFwkTag::DEFAULT, "State reset after SA died");
}

bool AssessmentServiceClient::LoadAssessmentService()
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    {
        std::unique_lock<std::mutex> lock(loadSaMutex_);
        loadSaFinished_ = false;
    }

    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "GetSystemAbilityManager failed");
        return false;
    }

    sptr<ISystemAbilityLoadCallback> loadCallback = new (std::nothrow) AssessmentServiceLoadCallback();
    if (loadCallback == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Create AssessmentServiceLoadCallback failed");
        return false;
    }

    auto ret = systemAbilityMgr->LoadSystemAbility(ASSESSMENT_SERVICE_ID, loadCallback);
    if (ret != ERR_OK) {
        TAG_LOGE(AAFwkTag::DEFAULT, "LoadSystemAbility failed, ret: %{public}d", ret);
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(loadSaMutex_);
        auto waitStatus = loadSaCondation_.wait_for(lock, std::chrono::milliseconds(LOAD_SA_TIMEOUT_MS),
            [this]() {
                return loadSaFinished_;
            });
        if (!waitStatus) {
            TAG_LOGE(AAFwkTag::DEFAULT, "Wait for load SA timeout");
            return false;
        }
    }

    return true;
}

void AssessmentServiceClient::SetAssessmentService(const sptr<IRemoteObject> &remoteObject)
{
    std::lock_guard<std::mutex> lock(mutex_);
    assessmentService_ = iface_cast<IAssessmentService>(remoteObject);
}

sptr<IAssessmentService> AssessmentServiceClient::GetAssessmentService()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return assessmentService_;
}

void AssessmentServiceClient::OnLoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject)
{
    SetAssessmentService(remoteObject);
    std::unique_lock<std::mutex> lock(loadSaMutex_);
    loadSaFinished_ = true;
    loadSaCondation_.notify_one();
}

void AssessmentServiceClient::OnLoadSystemAbilityFail()
{
    SetAssessmentService(nullptr);
    std::unique_lock<std::mutex> lock(loadSaMutex_);
    loadSaFinished_ = true;
    loadSaCondation_.notify_one();
}
} // namespace AAFwk
} // namespace OHOS