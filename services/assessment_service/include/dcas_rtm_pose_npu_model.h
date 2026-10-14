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

#ifndef OHOS_ASSESSMENT_DCAS_RTM_POSE_NPU_MODEL_H
#define OHOS_ASSESSMENT_DCAS_RTM_POSE_NPU_MODEL_H

#include <singleton.h>
#include <string>
#include <vector>
#include <dlfcn.h>
#include "event_runner.h"

namespace OHOS {
namespace AAFwk {

class DcasRtmPoseNpuModel final : public DelayedSingleton<DcasRtmPoseNpuModel> {
    DECLARE_DELAYED_SINGLETON(DcasRtmPoseNpuModel);

public:
    ErrCode LoadModel();
    ErrCode UnloadModel();
    ErrCode Infer(int inputFd, int32_t inputSize, int& outputFd, int32_t& outputSize);

private:
    bool InitLibrary();
    
    void* soHandle_ = nullptr;
    std::mutex mutex_;

    typedef int32_t (*HiARLoadModelFunc)();
    typedef int32_t (*HiARUloadModelFunc)();
    typedef int32_t (*HiARInferFunc)(int, int32_t, int*, int*);

    HiARLoadModelFunc hiARLoadModelFn_ = nullptr;
    HiARUloadModelFunc hiARUloadModelFn_ = nullptr;
    HiARInferFunc hiARInferFn_ = nullptr;

    DISALLOW_COPY_AND_MOVE(DcasRtmPoseNpuModel);
};
}
}
#endif // OHOS_AAFwk_ASSESSMENT_DCAS_RTM_POSE_NPU_MODEL_H