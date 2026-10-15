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
 
#include "assessment_ar_model_manager.h"
#include <filesystem>
#include <string>
#include "hilog_tag_wrapper.h"
#include "bundle_mgr_interface.h"
#include "bundle_mgr_proxy.h"
#include "bundle_info.h"

using namespace std;
using namespace OHOS::AAFwk;
namespace fs = std::filesystem;

AssessmenModelDownload::~AssessmenModelDownload()
{
    TAG_LOGI(AAFwkTag::DEFAULT, "AssessmenModelDownload Singleton Instance Destroyed.");
    std::lock_guard<std::mutex> lock(mutex_);
    if (soHandle_ != nullptr) {
        dlclose(soHandle_);
        soHandle_ = nullptr;
    }
}

AssessmenModelDownload &AssessmenModelDownload::GetInstance()
{
    static AssessmenModelDownload modelDownload;
    return modelDownload;
}

bool AssessmenModelDownload::InitLibrary()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (soHandle_ != nullptr) {
        return true;
    }
    soHandle_ = dlopen("libarengine_ai_model_adapt.z.so", RTLD_NOW);
    if (soHandle_ == nullptr) {
        TAG_LOGI(AAFwkTag::DEFAULT, "dlopen failed: %{public}s", dlerror());
        return false;
    }
    setPidFn_ = reinterpret_cast<SetPidFunc>(dlsym(soHandle_, "HiARSetPid"));
    setTokenidFn_ = reinterpret_cast<SetTokenidFunc>(dlsym(soHandle_, "HiARSetTokenid"));
    checkModelStatusFn_ = reinterpret_cast<CheckModelStatusFunc>(dlsym(soHandle_, "HiARCheckModelStatus"));
    if (!setPidFn_ || !setTokenidFn_ || !checkModelStatusFn_) {
        TAG_LOGI(AAFwkTag::DEFAULT, "dlsym failed: %{public}s", dlerror());
        dlclose(soHandle_);
        soHandle_ = nullptr;
        return false;
    }
    return true;
}

void AssessmenModelDownload::SetPid(int32_t pid)
{
    if (InitLibrary() && setPidFn_) {
        setPidFn_(pid);
    }
}

void AssessmenModelDownload::SetTokenid(uint32_t tokenId)
{
    if (InitLibrary() && setTokenidFn_) {
        setTokenidFn_(tokenId);
    }
}

int AssessmenModelDownload::CheckModelStatus()
{
    if (InitLibrary() && checkModelStatusFn_) {
        return checkModelStatusFn_(nullptr);
    }
    return static_cast<int>(AssessmentARModelStatus::UNKNOWN);
}

int AssessmenModelDownload::CheckModelStatus(void* callbackToken)
{
    if (InitLibrary() && checkModelStatusFn_) {
        return checkModelStatusFn_(callbackToken);
    }
    return static_cast<int>(AssessmentARModelStatus::UNKNOWN);
}