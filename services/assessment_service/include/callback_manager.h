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

#ifndef OHOS_AAFWK_ASSESSMENT_CALLBACK_MANAGER_H
#define OHOS_AAFWK_ASSESSMENT_CALLBACK_MANAGER_H

#include <map>
#include <mutex>

#include "assessment_callback_code.h"
#include "iremote_object.h"

namespace OHOS {
namespace AAFwk {

class CallbackManager {
public:
    static CallbackManager& GetInstance();

    int32_t RegisterCallback(const sptr<IRemoteObject>& token, const sptr<IRemoteObject>& callback);
    int32_t UnregisterCallback(const sptr<IRemoteObject>& token);
    sptr<IRemoteObject> GetCallback(const sptr<IRemoteObject>& token);
    void ClearAllCallbacks();

    void OnBegin(const sptr<IRemoteObject>& token, int32_t code, const std::string& message);
    void OnInterrupted(const sptr<IRemoteObject>& token, int32_t reason, const std::string& message);
    void OnEnd(const sptr<IRemoteObject>& token);

private:
    CallbackManager() = default;
    ~CallbackManager() = default;
    CallbackManager(const CallbackManager&) = delete;
    CallbackManager& operator=(const CallbackManager&) = delete;

    std::mutex mutex_;
    std::map<sptr<IRemoteObject>, sptr<IRemoteObject>> callbacks_;
};
} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_AAFWK_ASSESSMENT_CALLBACK_MANAGER_H