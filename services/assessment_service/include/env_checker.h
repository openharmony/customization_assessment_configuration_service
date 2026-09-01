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

#ifndef OHOS_AAFWK_ASSESSMENT_ENV_CHECKER_H
#define OHOS_AAFWK_ASSESSMENT_ENV_CHECKER_H

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "call_manager_client.h"
#include "device_manager_callback.h"
#include "extension_loader.h"

namespace OHOS {
namespace AAFwk {

class AssessmentDmInitCallback : public DistributedHardware::DmInitCallback {
public:
    void OnRemoteDied() override;    
};

/**
 * @class EnvChecker
 * @brief Environment security checker for assessment scenarios.
 *
 * EnvChecker verifies whether the current device environment is safe before
 * starting an assessment. It performs a series of checks including screen
 * recording/casting detection, multi-screen detection, in-call detection,
 * PC virtual-machine detection, and foreground application whitelist validation.
 *
 * The check logic is organized as follows:
 * 1. IsScreenRecording  - Detects screen capture via DisplayManager.
 * 2. IsScreenCasting    - Detects remote devices via DeviceManager (soft bus).
 * 3. IsMultiScreen      - Detects virtual screens via ScreenManager.
 * 4. IsInCall           - Detects active phone call via CallManagerClient.
 * 5. IsPcDevice         - Checks if the device type is "2in1" (PC form factor).
 *    └─ IsVirtualMachine - Delegates to ExtensionLoader for closed-source VM check.
 * 6. HasOtherAppRunning - Ensures no non-whitelisted foreground apps are running.
 *
 * All checks must pass (return true from CheckAll) for the assessment to proceed.
 * Any single failure causes CheckAll to return false immediately.
 *
 * @note The preset whitelist always includes "com.ohos.sceneboard". Callers can
 *       supply additional allowed bundle names via the allowedApps parameter.
 */
class EnvChecker {
public:
    EnvChecker();
    ~EnvChecker() = default;

    /**
     * @brief Run all environment checks sequentially.
     * @param allowedApps Additional bundle names allowed to run in the foreground.
     * @return true if all checks pass, false if any check fails.
     */
    bool CheckAll(const std::vector<std::string> &allowedApps);

private:
    bool InitDeviceManager();
    bool InitCallManager();
    bool IsScreenRecording();
    bool IsScreenCasting();
    bool IsMultiScreen();
    bool IsInCall();
    bool IsPcDevice();
    bool IsVirtualMachine();
    bool HasOtherAppRunning(const std::vector<std::string> &allowedApps);

    bool deviceManagerInited_ = false;
    bool callManagerInited_ = false;

    std::shared_ptr<AssessmentDmInitCallback> dmInitCallback_;
    std::unique_ptr<ExtensionLoader> loader_;

    const std::set<std::string> presetAllowedApps_ = {
        "com.ohos.sceneboard",
    };
};

} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_AAFWK_ASSESSMENT_ENV_CHECKER_H