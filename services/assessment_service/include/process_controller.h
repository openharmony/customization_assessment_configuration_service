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
     * Stops the distributed softbus, registers the telephony observer that
     * auto-rejects incoming calls, registers the screen hotplug listener, then asks
     * the closed-source extension to apply its own restrictions.
     *
     * Every sub-item is fatal: an assessment must not run without a restriction it
     * asked for. The first failure stops the sequence, the stages already completed
     * are rolled back in reverse order, and the assessment does not start.
     * @param allowedApps Bundle names that must stay usable during the assessment.
     * @param failReason Filled with the reason of the sub-item that aborted the
     *                   activation, using the vocabulary declared in
     *                   assessment_fail_reason.h. The closed-source extension fails
     *                   fast internally, so it reports at most one reason and a
     *                   single string carries it. Left untouched on success.
     * @return true if every sub-item was applied; false otherwise, in which case
     *         nothing is left applied.
     */
    bool Activate(const std::vector<std::string> &allowedApps, std::string &failReason);

    /**
     * @brief Revert every restriction applied by Activate().
     * @param failReason Filled with the reason of the first sub-item that could not
     *                   be reverted (softbus restart or closed-source DeactivateAll).
     *                   Left untouched when everything was restored.
     */
    void Deactivate(std::string &failReason);

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
    bool RegisterCallObserver();
    void UnRegisterCallObserver();
    bool RegisterScreenListener();
    void UnregisterScreenListener();

    /**
     * @brief Undo the stages Activate() already completed, in reverse order.
     *
     * Must be called with mutex_ held, from Activate() only. Each stage that
     * Activate() completes appends its own undo action to the list, so the list is
     * exactly what has to be reverted and this helper needs no knowledge of the
     * stage order or of how many stages there are. The closed-source extension
     * appends its undo *before* ActivateAll() is called, because that call fails
     * fast internally and may already have applied some of its sub-items.
     * A failure to roll back is logged but not reported: the reason handed to the
     * caller must stay the one that aborted the activation.
     * @param undoActions Undo actions of the completed stages, run back to front
     *                    and emptied.
     */
    void RollbackActivateLockedUnsafe(std::vector<std::function<void()>> &undoActions);

    /**
     * @brief Ask the closed-source extension to apply its own restrictions.
     *
     * Must be called with mutex_ held, from Activate() only. Appends the undo action
     * *before* calling ActivateAll(), because that call fails fast internally and may
     * already have applied some sub-items when it reports a failure. Does no rollback
     * itself, so that Activate() keeps a single place that decides to roll back.
     * @param allowedApps Bundle names that must stay usable during the assessment.
     * @param failReason Filled with the extension's reason, or with the generic
     *                   virtual-machine reason when the extension fails but leaves it
     *                   empty. Left untouched on success and when there is no
     *                   extension at all.
     * @param undoActions List the extension undo is appended to.
     * @return true if applied, or if this device ships no closed-source extension.
     */
    bool ActivateExtensionLockedUnsafe(const std::vector<std::string> &allowedApps,
        std::string &failReason, std::vector<std::function<void()>> &undoActions);

    /**
     * @brief Restart the distributed softbus, logging a failure instead of
     *        reporting it. Used as the undo action of the stop-softbus stage.
     */
    void StartSoftBusForRollback();

    /**
     * @brief Ask the closed-source extension to revert what ActivateAll() applied.
     *
     * Used as the undo action of the extension stage. The extension's own reason is
     * dropped on purpose, see RollbackActivateLockedUnsafe().
     */
    void RevertExtensionForRollback();

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
