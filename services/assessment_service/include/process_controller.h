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

#ifndef OHOS_AAFWK_ASSESSMENT_PROCESS_CONTROLLER_H
#define OHOS_AAFWK_ASSESSMENT_PROCESS_CONTROLLER_H

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "telephony_observer.h"
#include "telephony_observer_client.h"

namespace OHOS {
namespace AAFwk {

class ProcessController;

/**
 * @class AssessmentTelephonyObserver
 * @brief Telephony observer that forwards call-state changes to ProcessController.
 */
class AssessmentTelephonyObserver : public Telephony::TelephonyObserver {
public:
    explicit AssessmentTelephonyObserver(ProcessController *controller) : controller_(controller) {}
    ~AssessmentTelephonyObserver() override = default;

    void OnCallStateUpdated(int32_t slotId, int32_t callState, const std::u16string &phoneNumber) override;

private:
    ProcessController *controller_;
};

/**
 * @class ProcessController
 * @brief Process controller that locks down the device during an assessment.
 *
 * While activated, ProcessController:
 * 1. Registers a telephony observer to detect incoming calls.
 * 2. Forwards call-state events to the assessment service for interruption.
 */
class ProcessController {
public:
    ProcessController() = default;
    ~ProcessController() = default;

    /**
     * @brief Initialize the CallManager dependency. Intended to be called once
     *        at service startup rather than on each Activate().
     * @return true if CallManager initialized successfully.
     */
    bool Init();

    /**
     * @brief Lock down the device for an assessment.
     *
     * Registers the telephony observer that auto-rejects incoming calls.
     * @return true if process control was fully activated.
     */
    bool Activate(const std::vector<std::string> &allowedApps);
    void Deactivate();

    bool IsActivated() const;

private:
    bool InitCallManager();
    void RegisterCallObserver();
    void UnRegisterCallObserver();

    // Serializes Activate()/Deactivate(): they may run on different threads
    // (activation from the common-event thread without the service lock,
    // deactivation from IPC/DoLoop threads under the service lock).
    std::mutex mutex_;
    std::atomic<bool> activated_ = false;
    bool callManagerInited_ = false;
    bool callObserverRegistered_ = false;
    sptr<AssessmentTelephonyObserver> telephonyObserver_;
};

} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_AAFWK_ASSESSMENT_PROCESS_CONTROLLER_H
