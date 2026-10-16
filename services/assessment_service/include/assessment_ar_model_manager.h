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
#ifndef OHOS_ASSESSMENT_AR_MODEL_MANAGER_H
#define OHOS_ASSESSMENT_AR_MODEL_MANAGER_H

#include <cstdint>
#include <mutex>
#include <memory>
#include <dlfcn.h>
namespace OHOS {
namespace AAFwk {

enum class AssessmentARModelStatus {
    UNKNOWN = 0,
    READY = 1,
    UNDOWNLOADED = 2,
    DOWNLOADING = 3,
    DOWNLOAD_FAILED = 4,
    DOWNLOAD_PAUSED = 5,
    NEED_UPDATE = 6,
    COPYING = 7,
    COPY_FAILED = 8,
    INSTALLING = 9,
    INSTALL_FAILED = 10,
    WAITING_AUTH = 11
};
class AssessmenModelDownload {
public:
    ~AssessmenModelDownload();

    static AssessmenModelDownload &GetInstance();
    //防止拷贝
    AssessmenModelDownload(const AssessmenModelDownload&) = delete;
    void operator=(const AssessmenModelDownload&) = delete;

    void SetPid(int32_t pid);
    void SetTokenid(uint32_t tokenId);
    int CheckModelStatus();
    int CheckModelStatus(void* callbackToken);

private:
    AssessmenModelDownload() = default;
    bool InitLibrary();
    typedef void (*SetPidFunc)(int32_t);
    typedef void (*SetTokenidFunc)(uint32_t);
    typedef int (*CheckModelStatusFunc)(void*);
    void* soHandle_ = nullptr;
    std::mutex mutex_;
    SetPidFunc setPidFn_ = nullptr;
    SetTokenidFunc setTokenidFn_ = nullptr;
    CheckModelStatusFunc checkModelStatusFn_ = nullptr;
};
}
}
#endif // HIAR_MODEL_DOWNLOAD
