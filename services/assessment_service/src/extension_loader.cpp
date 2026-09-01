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

#include "extension_loader.h"

#include <dlfcn.h>
#include <thread>

#include "hilog_tag_wrapper.h"

namespace OHOS {
namespace AAFwk {

ExtensionLoader::ExtensionLoader(const std::string &soPath) : soPath_(soPath) {}

ExtensionLoader::~ExtensionLoader() 
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ != nullptr) {
        dlclose(handle_);
        handle_ = nullptr;
        TAG_LOGI(AAFwkTag::DEFAULT, "dlclose %{public}s", soPath_.c_str());
    }
}

void ExtensionLoader::InitExtensionLoader()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ != nullptr) {
        return;
    }

    int32_t attempt = 0;
    while (attempt <= retryPolicy_.maxRetries) {
        if (LoadInternal()) {
            break;
        }
        if (attempt < retryPolicy_.maxRetries) {
            TAG_LOGI(AAFwkTag::DEFAULT, "Load %{public}s retry %{public}d/%{public}d after %{public}d ms",
                soPath_.c_str(), attempt + 1, retryPolicy_.maxRetries, retryPolicy_.retryIntervalMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(retryPolicy_.retryIntervalMs));
        }
        attempt++;
    }
    
    if (handle_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Load %{public}s failed after %{public}d attempts",
            soPath_.c_str(), attempt);
        return;
    }

    checkAllFunc_ = (CHECK_ALL_FUNC)dlsym(handle_, "CheckAll");
    if (checkAllFunc_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "dlsym CheckAll failed: %{public}s", dlerror());
    }

    TAG_LOGI(AAFwkTag::DEFAULT, "extension loader init success");
}

bool ExtensionLoader::LoadInternal()
{
    handle_ = dlopen(soPath_.c_str(), RTLD_NOW);
    if (handle_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "dlopen %{public}s failed: %{public}s",
            soPath_.c_str(), dlerror());
        return false;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "dlopen %{public}s success", soPath_.c_str());
    return true;
}

bool ExtensionLoader::InvokeCheckAll(const std::vector<std::string> &allowedApps)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (checkAllFunc_ == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "CheckAll func is null, degrade");
        if (degradedCallback_) {
            degradedCallback_(soPath_, "CheckAll");
        }
        return true; // defalut pass
    }
    return checkAllFunc_(allowedApps);
}

bool ExtensionLoader::IsDegrade() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return checkAllFunc_ == nullptr;
}

void ExtensionLoader::SetDegradedCallback(DegradedCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    degradedCallback_ = std::move(callback);
}

void ExtensionLoader::SetRetryPolicy(const RetryPolicy &policy)
{
    std::lock_guard<std::mutex> lock(mutex_);
    retryPolicy_ = policy;
}

} // namespace AAFwk
} // namespace OHOS