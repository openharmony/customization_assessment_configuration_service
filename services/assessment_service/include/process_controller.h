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
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "extension_loader.h"
#include "screen_manager.h"
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
 * @class AssessmentScreenListener
 * @brief Screen listener that forwards screen hotplug events to ProcessController.
 *
 * Only OnConnect is meaningful: plugging in an external screen while an
 * assessment is running is an environment anomaly. OnDisconnect and OnChange
 * are pure virtual in the base class, so they are implemented as log-only.
 */
class AssessmentScreenListener : public Rosen::ScreenManager::IScreenListener {
public:
    explicit AssessmentScreenListener(ProcessController *controller) : controller_(controller) {}
    ~AssessmentScreenListener() override = default;

    void OnConnect(Rosen::ScreenId screenId) override;
    void OnDisconnect(Rosen::ScreenId screenId) override;
    void OnChange(Rosen::ScreenId screenId) override;

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
 * 3. Disables virtual machines via the closed-source extension.
 * 4. Stops the distributed softbus service.
 * 5. Registers a screen listener; an external screen plugged in mid-assessment
 *    is reported through the environment-anomaly callback so that the service
 *    can interrupt the assessment.
 */
class ProcessController {
public:
    ProcessController() = default;
    ~ProcessController() = default;

    /**
     * @brief Initialize the CallManager dependency and the extension loader
     *        used for VM disable/enable. Intended to be called once at
     *        service startup rather than on each Activate().
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

    /**
     * @brief Set the callback invoked when an environment anomaly is detected
     *        while the assessment is running (currently: external screen plugged in).
     *
     * Must be called once at service startup, before any Activate(), and must not
     * be reassigned later: NotifyScreenConnected() reads it without holding mutex_
     * so that the RenderService callback thread never acquires mutex_ before the
     * service lock (the normal path takes the service lock first, then mutex_).
     */
    void SetEnvAnomalyCallback(std::function<void()> callback);

    /**
     * @brief Entry point for AssessmentScreenListener::OnConnect.
     *
     * Runs on the RenderService callback thread. Deliberately takes no lock:
     * the callback it invokes ends up calling back into Deactivate(), which
     * acquires mutex_ -- holding mutex_ here would self-deadlock.
     */
    void NotifyScreenConnected(Rosen::ScreenId screenId);

private:
    bool InitCallManager();
    void RegisterCallObserver();
    void UnRegisterCallObserver();
    void RegisterScreenListener();
    void UnregisterScreenListener();

    // Serializes Activate()/Deactivate(): they may run on different threads
    // (activation from the common-event thread without the service lock,
    // deactivation from IPC/DoLoop threads under the service lock).
    std::mutex mutex_;
    std::atomic<bool> activated_ = false;
    bool callManagerInited_ = false;
    bool callObserverRegistered_ = false;
    bool screenListenerRegistered_ = false;
    sptr<AssessmentTelephonyObserver> telephonyObserver_;
    sptr<AssessmentScreenListener> screenListener_;
    std::unique_ptr<ExtensionLoader> extLoader_;
    // Written once by SetEnvAnomalyCallback() before any Activate(), then only
    // read; see that method for why it is accessed without mutex_.
    std::function<void()> envAnomalyCallback_;
};

} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_AAFWK_ASSESSMENT_PROCESS_CONTROLLER_H
