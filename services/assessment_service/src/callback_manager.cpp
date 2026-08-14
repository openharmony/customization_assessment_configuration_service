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

#include "callback_manager.h"
#include "hilog_tag_wrapper.h"
#include "ipc_types.h"

namespace OHOS {
namespace AAFwk {

CallbackManager& CallbackManager::GetInstance()
{
    static CallbackManager instance;
    return instance;
}

int32_t CallbackManager::RegisterCallback(const sptr<IRemoteObject>& token, const sptr<IRemoteObject>& callback)
{
    if (token == nullptr || callback == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "invalid parameter");
        return -1;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_[token] = callback;
    TAG_LOGI(AAFwkTag::DEFAULT, "callback registered, size: %{public}zu", callbacks_.size());
    return 0;
}

int32_t CallbackManager::UnregisterCallback(const sptr<IRemoteObject>& token)
{
    if (token == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "token is null");
        return -1;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = callbacks_.find(token);
    if (it != callbacks_.end()) {
        callbacks_.erase(it);
        TAG_LOGI(AAFwkTag::DEFAULT, "callback unregistered, size: %{public}zu", callbacks_.size());
    }
    return 0;
}

sptr<IRemoteObject> CallbackManager::GetCallback(const sptr<IRemoteObject>& token)
{
    if (token == nullptr) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = callbacks_.find(token);
    if (it != callbacks_.end()) {
        return it->second;
    }
    return nullptr;
}

void CallbackManager::ClearAllCallbacks()
{
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.clear();
    TAG_LOGI(AAFwkTag::DEFAULT, "all callbacks cleared");
}

void CallbackManager::OnBegin(const sptr<IRemoteObject>& token, int32_t code, const std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = callbacks_.find(token);
    if (it == callbacks_.end()) {
        TAG_LOGW(AAFwkTag::DEFAULT, "callback not found for token");
        return;
    }

    MessageParcel data;
    data.WriteInt32(code);
    data.WriteString(message);

    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    it->second->SendRequest(static_cast<uint32_t>(AssessmentCallbackCode::ON_BEGIN), data, reply, option);
}

void CallbackManager::OnInterrupted(const sptr<IRemoteObject>& token, int32_t reason, const std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = callbacks_.find(token);
    if (it == callbacks_.end()) {
        TAG_LOGW(AAFwkTag::DEFAULT, "callback not found for token");
        return;
    }

    MessageParcel data;
    data.WriteInt32(reason);
    data.WriteString(message);

    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    it->second->SendRequest(static_cast<uint32_t>(AssessmentCallbackCode::ON_INTERRUPTED), data, reply, option);
}

void CallbackManager::OnEnd(const sptr<IRemoteObject>& token)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = callbacks_.find(token);
    if (it == callbacks_.end()) {
        TAG_LOGW(AAFwkTag::DEFAULT, "callback not found for token");
        return;
    }

    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    it->second->SendRequest(static_cast<uint32_t>(AssessmentCallbackCode::ON_END), data, reply, option);
}
} // namespace AAFwk
} // namespace OHOS