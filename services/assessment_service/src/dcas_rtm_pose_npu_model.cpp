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
#include "dcas_rtm_pose_npu_model.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <algorithm>
#include <securec.h>
#include <sys/mman.h>
#include "hilog_tag_wrapper.h"

namespace OHOS {
namespace AAFwk {

DcasRtmPoseNpuModel::DcasRtmPoseNpuModel : soHandle_(nullptr),
    hiARLoadModelFn_(nullptr), hiARUnloadModelFn_(nullptr), hiARInterFn_(nullptr)
{
    TAG_LOGI(AAFwk::DEFAULT, "DcasRtmPoseNpuModel.");
}

DcasRtmPoseNpuModel::~DcasRtmPoseNpuModel()
{
    TAG_LOGI(AAFwk::DEFAULT, "~DcasRtmPoseNpuModel.");
    std::lock_guard<std::mutex> lock(mutex_);
    if (soHandle_ != nullptr) {
        dlclose(soHandle_);
        soHandle_ = nullptr;
    }
}

bool DcasRtmPoseNpuModel::InitLibrary()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (soHandle_ != nullptr) {
        return true;
    }

    soHandle_ = dlopen("libarengine_ai_model_adapt.z.so", RTLD_NOW);
    if (soHandle_ == nullptr) {
        TAG_LOGI(AAFwk::DEFAULT, "DcasRtmPoseNpuModel dlopen failed: %{public}s", dlerror());
        return false;
    }

    hiARLoadModelFn_ = reinterpret_cast<HiARLoadModelFunc>(dlsym(soHandle_, "HiARLoadModel"));
    hiARUnloadModelFn_ = reinterpret_cast<HiARUnloadModelFunc>(dlsym(soHandle_, "HiARUnloadModel"));
    hiARInferFn_ = reinterpret_cast<HiARInferFunc>(dlsym(soHandle_, "HiARInfer"));

    if (hiARLoadModelFn_ == nullptr || hiARUnloadModelFn_ == nullptr || hiARInferFn_ == nullptr) {
        TAG_LOGI(AAFwk::DEFAULT, "DcasRtmPoseNpuModel dlsym failed: %{public}s", dlerror());
        dlclose(soHandle_);
        soHandle_ = nullptr;
        return false;
    }

    return true;
}

ErrCode DcasRtmPoseNpuModel::LoadModel()
{
    if (!InitLibrary()) {
        return ERR_INVALID_VALUE;
    }

    return static_cast<ErrCode>(hiARLoadModelFn_());
}

ErrCode DcasRtmPoseNpuModel::UnloadModel()
{
    if (!InitLibrary()) {
        return ERR_INVALID_VALUE;
    }

    return static_cast<ErrCode>(hiARUnloadModelFn_());
}

ErrCode DcasRtmPoseNpuModel::Infer(int intputFd, int32_t inputSize, int& outputFd, int32_t& outputSize)
{
    if (!InitLibrary()) {
        return ERR_INVALID_VALUE;
    }

    int32_t ret = hiARInferFn_(intputFd, inputSize, &outputFd, &outputSize);
    return static_cast<ErrCode>(ret);
}
}
}