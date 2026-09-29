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
        TAG_LOGI(AAFwkTag::DEFAULT, "dlclose %{private}s", soPath_.c_str());
    }
}

bool ExtensionLoader::InitExtensionLoader()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ != nullptr) {
        return checkAllFunc_ != nullptr;
    }

    if (!LoadInternal()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Load %{private}s failed, entering degraded mode", soPath_.c_str());
        return false;
    }

    checkAllFunc_ = reinterpret_cast<CHECK_ALL_FUNC>(dlsym(handle_, "CheckAll"));
    if (checkAllFunc_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "dlsym CheckAll failed: %{private}s", dlerror());
        return false;
    }

    isAwakeAncoFunc_ = reinterpret_cast<IS_AWAKE_ANCO_FUNC>(dlsym(handle_, "IsAwakeAnco"));
    if (isAwakeAncoFunc_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "dlsym IsAwakeAnco failed: %{private}s", dlerror());
        return false;
    }

    restrictAncoAppFunc = reinterpret_cast<RESTRICT_ANCO_APP_FUNC>(dlsym(handle_, "RestrictAncoApp"));
    if (restrictAncoAppFunc == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "dlsym RestrictAncoApp failed: %{private}s", dlerror());
        return false;
    }

    TAG_LOGI(AAFwkTag::DEFAULT, "extension loader init success");
    return true;
}

bool ExtensionLoader::LoadInternal()
{
    handle_ = dlopen(soPath_.c_str(), RTLD_NOW);
    if (handle_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "dlopen %{private}s failed: %{private}s",
            soPath_.c_str(), dlerror());
        return false;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "dlopen %{private}s success", soPath_.c_str());
    return true;
}

bool ExtensionLoader::InvokeCheckAll(const std::vector<std::string> &allowedApps)
{
    CHECK_ALL_FUNC checkFunc = nullptr;
    DegradedCallback degradedCb = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        checkFunc = checkAllFunc_;
        degradedCb = degradedCallback_;
    }
    if (checkFunc == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "CheckAll func is null, degrade");
        if (degradedCb) {
            degradedCb(soPath_, "CheckAll");
        }
        return true; // default pass
    }
    return checkFunc(allowedApps);
}

bool ExtensionLoader::InvokeIsAwakeAnco(std::string ancoState)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (isAwakeAncoFunc_ == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "IsAwakeAnco func is null, degrade");
        if (degradedCallback_) {
            degradedCallback_(soPath_, "IsAwakeAnco");
        }
        return false;
    }
    return isAwakeAncoFunc_(ancoState);
}

void ExtensionLoader::InvokeRestrictAncoApp()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (restrictAncoAppFunc == nullptr) {
        TAG_LOGW(AAFwkTag::DEFAULT, "RestrictAncoApp func is null, degrade");
        if (degradedCallback_) {
            degradedCallback_(soPath_, "RestrictAncoApp");
        }
    } else {
        restrictAncoAppFunc();
    }
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

} // namespace AAFwk
} // namespace OHOS
