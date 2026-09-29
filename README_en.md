# Assessment Configuration Service

## Introduction

The Assessment Configuration Service (hereafter this service) targets third-party assessment applications (hereafter the caller) in education, evaluation and exam scenarios, and provides the capability to enter assessment mode. After the caller requests to enter assessment mode, this service completes the pre-start environment security checks and enters Kiosk mode (only applications in the allowlist can run), and automatically releases the control and notifies the caller when the assessment duration expires, the caller exits, or the assessment is ended actively. The controls applied during an assessment are implemented by this service together with cooperating components; the assessment state is persisted in system parameters, and after an abnormal service restart this service exits assessment mode.

Table 1 Component information
| Item | Value |
|------|-------|
| Component | `assessment_configuration_service` (subsystem `customization`) |
| SysCap | `SystemCapability.Customization.AssessmentConfiguration` |
| SystemAbility | SystemAbility ID (SAID) `8660`, library `libassessmentsvcs.z.so`, on-demand start (`ondemand`), auto-restart supported |
| Process | `assessment_service` (hosted by `sa_main`, uid/gid: `system`, APL: `system_basic`) |
| Languages | C++ (service, Inner Kit, NAPI), ArkTS (application-side calls) |

## Architecture

**Figure 1** Assessment Configuration Service architecture

![](figure/assessment_configuration_service.png)

Notes on the architecture figure:

- A request from the caller goes through the Automatic Scene Configuration Kit; the **Assessment Configuration Service proxy** loads and accesses the Assessment Configuration Service SA in the system service layer on demand through SAManager, realizing the inter-process communication (IPC) from the application layer to the system service layer.
- The Assessment Configuration Service (assessment\_configuration\_service) in the system service layer contains six modules: lifecycle management, event management, environment detection, device control, recovery management and security management.
- The capabilities required by assessment mode, such as Kiosk control, device control, and notification and interconnection control, are supported by APIs provided by cooperating system services.


### Module Functions

The architecture is divided, from top to bottom, into the application layer, the Automatic Scene Configuration Kit, the framework layer and the system service layer (the system service layer contains the Assessment Configuration Service and the cooperating system services).

* **Application layer**
  * **Third-party assessment applications**: request to enter and exit assessment mode, query the assessment state, and receive assessment state callbacks through the Automatic Scene Configuration Kit; the third-party assessment application must hold `ohos.permission.ASSESSMENT_CONFIGURATION` (system grant).
  * **SceneBoard**: notified by the Ability Runtime framework through IPC to enter or exit Kiosk mode and receive the allowlist, based on which it hides interface elements such as the notification center, control center, live capsule, multitask center, start menu and three-key navigation; this service does not call its APIs directly.

* **Automatic Scene Configuration Kit**: the assessment mode configuration APIs for applications (begin, end, isActive, getConfiguration and the assessment state callbacks).

* **Framework layer**
  * **Assessment Configuration Service proxy**: receives the call requests from the Kit, performs parameter validation, caller bundle name insertion and callback bridging, and starts the service on demand, forwards IPC calls and dispatches callbacks.

* **System service layer (assessment_service)**
  * **Assessment Configuration Service**
    * **Lifecycle management**: manages the initialization flow of the modules, the assessment duration countdown and the assessment state persistence (system parameters), and observes the process state of the caller.
    * **Event management**: common event management. Subscribes to system events such as suspend, shutdown and power-saving mode change, and to the LID (cover switch) events of the input subsystem, and exits assessment mode when the system state changes.
    * **Environment detection**: environment security checks before and during an assessment, which identify and block scenarios that do not meet a controlled assessment environment, mainly including:
      * Screen recording and screenshot: check whether screen recording or a screenshot is in progress;
      * Multi-screen and screen casting: check the external display and casting state to prevent the assessment content from leaking;
      * Call state: check whether a call is in progress; assessment mode is not entered during a call;
      * MDM device: check whether the device is managed by mobile device management (MDM) to avoid conflicts with assessment mode;
      * If a check fails, assessment mode is not entered.
    * **Device control**: implements the device control capabilities, for example rejecting incoming calls; for the capabilities see [Core Capabilities](#core-capabilities) and for the implementation see [Key Interaction Flows](#key-interaction-flows).
    * **Recovery management**: restores system functions and releases the assessment restrictions in abnormal scenarios such as a service process restart and an application crash.
    * **Security management**: security controls such as log management and caller permission verification (`ohos.permission.ASSESSMENT_CONFIGURATION`).
  * **Cooperating system services**
    * **Ability Runtime framework**: provides the APIs for entering and exiting Kiosk mode for the Assessment Configuration Service to call, implementing the single-app mode, preventing applications from being launched, and terminating applications that are not allowlisted. When Kiosk mode is entered or exited, it publishes the Kiosk mode common events, whose parameters indicate whether the entry originates from assessment mode, for cooperating components to listen to.
    * **Enterprise device management**: provides the MDM device query API for the Assessment Configuration Service to call, used to identify managed devices and avoid conflicts with assessment mode.
    * **Power management**: provides the screen-lock control API for the Assessment Configuration Service to call, so that automatic screen locking and user-initiated screen locking are prevented during an assessment.
    * **Telephony call manager**: provides the call state query and incoming-call rejection APIs. After subscribing to incoming-call events, the Assessment Configuration Service calls the rejection API, and the telephony call manager rejects the call and generates a missed call record.
    * **Notification system service**: listens to the Kiosk mode common events to restrict notifications during an assessment.
    * **Softbus**: provides the softbus query and switch APIs for the Assessment Configuration Service to call, used both as a pre-assessment check item and to disable cross-device interconnection during an assessment.
    * **Input method framework**: listens to the Kiosk mode common events to prevent input method switching during an assessment.
    * **Lock screen management service**: listens to the Kiosk mode common events so that the lock screen supports only restricted functions during an assessment.

### Core Capabilities

**Kiosk control**
- After the checks pass, Kiosk mode is entered; the end, timeout and exception paths all release the control.

**Device function restrictions during an assessment**

After assessment mode is entered, device functions are heavily restricted. This service receives the allowlist delivered by the caller, triggers entering and exiting Kiosk mode, releases the control on expiration or exceptions, and implements part of the restrictions. The remaining restrictions are implemented by Kiosk mode, or by this service calling the APIs of cooperating system services, or by cooperating components after they listen to the Kiosk mode common events (which carry the assessment mode flag).
- During an assessment, only the third-party assessment application is allowed to run; the application cannot be exited or switched to other applications;
- The third-party assessment application can allow other applications to run during an assessment (for example a calculator) by setting the `allowedApps` parameter; applications not in `allowedApps` are forcibly killed by the system after entering assessment mode;
- Remote control capabilities are prohibited;
- Taking screenshots and recording the screen through system capabilities are prohibited; the caller may take screenshots and record the screen themselves;
- Picture-in-picture, floating windows and the floating control ball are disabled (implemented by window management after it synchronizes the Kiosk state and the allowlist);
- The AI assistant is disabled, including voice wakeup and shortcut-key wakeup;
- Text selection lookup is disabled.
- The notification center, control center, live capsule, multitask center, start menu and three-key navigation are hidden (SceneBoard);
- Notification reminders are restricted during an assessment, including no ringtone, vibration, banner or live cards (including NFC), covering Push and system service notifications;
- If an audio/video call application is not included in the allowedApps parameter, calls cannot be connected during an assessment; if an audio/video call application is included in the allowedApps parameter, only reminders (such as ringing) are available during an assessment, but the call cannot be answered (live card notification reminders are restricted);
- Forwarding notifications to third-party devices is restricted (notification system service);
- The distributed softbus is disabled, and cross-device message forwarding is blocked during an assessment (softbus);
- Automatic incoming-call rejection: this service subscribes to incoming-call events and then rejects the call, and a missed call record is generated;
- Input method switching is prohibited (input method framework);
- Automatic screen locking and user-initiated screen locking are prevented (power management);
- The lock screen functions are restricted (lock screen management service);

**Assessment duration and automatic release**
- The assessment duration is in milliseconds; 0 means the default 8 hours, and the upper limit is 8 hours.
- The service polls with a maximum time slice of 5 seconds and converges to the remaining time when less than 5 seconds are left. The expiration decision is based on the expiration checkpoint recorded when the assessment starts, so polling jitter does not accumulate; the actual release time deviates from the configured duration only by milliseconds (thread wakeup and scheduling latency). After the duration expires, the control is released automatically.
- The caller process is observed; the assessment ends as soon as the caller exits.

**System event interworking**
- Subscribes to system events such as suspend, shutdown and power-saving mode change, and to the LID (cover switch) events of the input subsystem, to adapt to the 2in1 form factor: on suspend, shutdown, power-saving mode change and lid close, the service exits assessment mode (releasing Kiosk mode and all restrictions, clearing the assessment context and parameters, and calling back).

**State persistence**
- The assessment state is written to system parameters for persistence.

**Abnormal restart policy**
- When the service process is started again after an abnormal exit, the service exits assessment mode directly: Kiosk mode and all controls are released and system functions are restored.

**Callback notification**
- onBegin, onInterrupted and onEnd are called back to the caller.

### Key Interaction Flows

The three core flows are described below.

#### Entering Assessment Mode

1. **The caller requests to enter**: the caller calls `begin(context, config, callback)`; the NAPI performs parameter validation and caller bundle name insertion, bridges the JS callback into a C++ callback object, and initiates the IPC call through `AssessmentServiceClient` after loading this service on demand.
2. **This service performs the admission checks**: the security management module verifies the device form factor and the caller permission (`ohos.permission.ASSESSMENT_CONFIGURATION`) through `AssessmentServiceUtils`; the environment detection module checks screen recording and screenshot (`Rosen::DisplayManager::IsCaptured`), multi-screen and screen casting (`Rosen::ScreenManager`, `DistributedHardware::DeviceManager::GetAvailableDeviceList`), call state (`Telephony::CallManagerClient`) and MDM device through `EnvChecker::CheckAll()`; the lifecycle management module generates the assessment context (token, callback and expiration checkpoint) and writes the assessment state parameters. If any check fails, assessment mode is not entered and this service calls back `onBegin` with the failure result (an environment check failure is `ENV_ANOMALY`, code=4).
3. **This service registers the process observation**: the lifecycle management module registers the process state observer of the caller through `RegisterApplicationStateObserver`, and the exit of the caller process triggers exiting the assessment.
4. **This service subscribes to events**: the event management module subscribes to system events such as suspend (`usual.event.ENTER_HIBERNATE`), shutdown (`usual.event.SHUTDOWN`) and power-saving mode change (`usual.event.POWER_SAVE_MODE_CHANGED`) through `CommonEventManager::SubscribeCommonEvent`, subscribes to the LID (cover switch) event (`SwitchEvent::SWITCH_LID`) through `MMI::InputManager::SubscribeSwitchEvent`, and registers the call state observation through `Telephony::TelephonyObserverClient::AddStateObserver`.
5. **This service delivers the allowlist**: the device control module calls `AbilityManagerClient::AddKioskApplicationList` of the Ability Runtime framework to deliver the allowlist, and the Ability Runtime framework synchronizes it to SceneBoard through IPC.
6. **This service enters Kiosk mode and applies the restrictions**: the device control module calls `AbilityManagerClient::EnterKioskMode` of the Ability Runtime framework to enter [Kiosk mode](https://gitcode.com/openharmony/docs/blob/master/en/application-dev/reference/apis-ability-kit/js-apis-app-ability-kioskManager.md) (applications that are not allowlisted are terminated, users are prohibited from switching out of the application, remote control is prohibited, system screenshots and screen recording are prohibited, picture-in-picture, the floating ball and the floating control ball are disabled, the AI assistant and text selection lookup are disabled, and the notification center, control center, live capsule, multitask center, start menu and three-key navigation are hidden); the Ability Runtime framework notifies SceneBoard through IPC to enter Kiosk mode and delivers the allowlist, and publishes the Kiosk mode common event `usual.event.KIOSK_MODE_ON` (whose parameters indicate that this entry originates from assessment mode), so that the notification system service, input method framework and lock screen management service listen and implement their own restrictions:
    - SceneBoard: the Ability Runtime framework calls `SceneSessionManagerLite::UpdateKioskAppList` and `EnterKioskMode` through IPC to deliver the allowlist and the Kiosk state, hiding interface elements such as the notification center, control center, live capsule, multitask center, start menu and three-key navigation;
    - Notification system service: notifications are restricted, including no ringtone, vibration or banner;
    - Input method framework: input method switching is prohibited;
    - Lock screen management service: lock screen notifications and service cards are not displayed; pulling down the control center on the lock screen is disabled; switching to the private space or another user is prohibited; entering lock screen editing by a two-finger pinch or by pressing and holding the clock is prohibited;

    Restrictions implemented by this service:
    - Automatic incoming-call rejection: the device control module (`ProcessController`) calls `CallManagerProxy::RejectCall` of the telephony call manager after `AssessmentTelephonyObserver::OnCallStateUpdated` detects an incoming call;
    - Automatic locking after screen-off is prevented by calling `PowerMgrClient::LockScreenAfterTimingOut(false, false)` of power management;
    - The distributed softbus is disabled by calling `ServiceControl("softbus_server", ServiceAction::STOP)`.
7. **This service persists the state**: the lifecycle management module writes the assessment state to `persist.assessment.is_active`, `persist.assessment.duration` and `persist.assessment.allowed_apps` through `SaveState`, and sets the assessment state to active.
8. **This service calls back and starts the countdown**: this service calls back `onBegin` (code=0) to inform the caller that entering succeeded; the lifecycle management module starts the countdown thread, which checks the expiration with a maximum time slice of 5 seconds.

#### Exiting Assessment Mode

1. **The caller requests to end**: the caller calls `end(context)`, which must use the same context (token) as `begin`; the NAPI initiates the IPC call through `AssessmentServiceClient`, and the lifecycle management module verifies the assessment active state and the token (36700003 if not active, 36700004 if the token does not match).
2. **This service exits Kiosk mode**: the device control module calls `AbilityManagerClient::ExitKioskMode` of the Ability Runtime framework to release the Kiosk control; the Ability Runtime framework notifies SceneBoard through IPC to exit Kiosk mode, and publishes the Kiosk mode common event `usual.event.KIOSK_MODE_OFF` (whose parameters indicate that this exit originates from assessment mode), so that the notification system service, input method framework and lock screen management service listen and restore their capabilities.
3. **This service restores the restrictions**: after `AbilityManagerClient::ExitKioskMode` is called, the restrictions implemented by Kiosk mode are restored with the exit; the device control module (`ProcessController::Deactivate`) stops incoming-call rejection; `PowerMgrClient::LockScreenAfterTimingOut(true, false)` of power management is called to restore locking after auto-screen-off; `ServiceControl("softbus_server", ServiceAction::START)` is called to restore the distributed softbus; cooperating components restore notifications, input method and lock screen restrictions by themselves after listening to the Kiosk mode common events.
4. **This service unregisters the observation**: the lifecycle management module unregisters the process state observer of the caller and stops the countdown thread; the event management module cancels the system event subscription through `CommonEventManager::UnSubscribeCommonEvent`, cancels the LID (cover switch) event subscription, and removes the call state observation through `Telephony::TelephonyObserverClient::RemoveStateObserver`.
5. **This service clears the state**: the lifecycle management module clears the `persist.assessment.*` parameters and the assessment context through `ClearState` to avoid residual restrictions.
6. **This service returns the result**: this service calls back `onEnd` to inform the caller that the assessment ended.

#### Interrupted Assessment

An interruption is initiated by this service and is handled the same way as exiting assessment mode (releasing the Kiosk control, restoring the restrictions and clearing the state); the differences are the trigger and the callback:

1. **Duration expiration**: the countdown thread of the lifecycle management module compares the current time with the expiration checkpoint recorded when the assessment started, using a maximum time slice of 5 seconds (the default 8 hours is used when `duration` is 0), and converges to the remaining time when less than 5 seconds are left; after the duration expires, the control is released and this service calls back `onInterrupted` (`TIMEOUT`, code=2) to inform the caller that the assessment was interrupted.
2. **Environment anomaly**: if the environment detection module detects an environment anomaly during an assessment, the device control module releases the control, and this service calls back `onInterrupted` (`ENV_ANOMALY`, code=4) to inform the caller.
3. **System events**: when the event management module receives suspend (`usual.event.ENTER_HIBERNATE`), shutdown (`usual.event.SHUTDOWN`), power-saving mode change (`usual.event.POWER_SAVE_MODE_CHANGED`) or lid-close (`SwitchEvent::SWITCH_LID`) events, the service exits assessment mode.
4. **Caller exit**: after the process state observer of the lifecycle management module detects that the caller process is gone, it releases the control and clears the assessment context and the `persist.assessment.*` parameters.
5. **Service restart**: when the service process is started again after an abnormal exit, the recovery management module exits assessment mode directly, does not restore the assessment context and does not continue the countdown; the caller must call `begin` again.

### Concepts and Terms

Table 2 Concepts and terms
| Concept / Term | Meaning |
|----------------|---------|
| Assessment context | The runtime data of one assessment held by the service: the caller token, the callback, `duration`, `allowedApps` and the expiration checkpoint |
| Allowed application | The list of bundle names delivered by the caller at `begin`, allowed to run during Kiosk mode, usually the third-party assessment application and its companion applications |
| Assessment duration | `duration` (in milliseconds); `0` means the default 8 hours, and the upper limit is 8 hours |
| Assessment configuration | The `duration` and `allowedApps` of the current assessment, which can be queried by `getConfiguration` |
| Assessment mode | The controlled assessment environment state entered by the caller through `begin` and exited by `end` or on an exception |
| Caller | The third-party assessment application that requests to enter or exit assessment mode through the Automatic Scene Configuration Kit (hereafter) |
| Kiosk mode | The single-app lock mode provided by the Ability Runtime framework; only specified applications can run; assessment mode stacks controls on top of it |
| Cooperating system service | The system components that implement the assessment-time restrictions together with the Assessment Configuration Service, such as the Ability Runtime framework, the notification system service, power management, and the telephony call manager |

## Directory

The source code directory structure of this service is as follows:

```text
/base/customization/assessment_configuration_service
├── figure
│   └── assessment_configuration_service.png     # Architecture figure
├── frameworks
│   └── js/napi
│       ├── BUILD.gn                             # NAPI library build (libassessment_napi.z.so)
│       └── assessment
│           ├── assessment_napi.cpp/.h           # JS API implementation and parameter validation
│           └── js_assessment_callback.cpp/.h    # JS callback bridging
├── interfaces
│   └── inner_api/assessment_service
│       ├── BUILD.gn                             # Inner Kit library build (libassessment_manager.z.so)
│       ├── IAssessmentService.idl               # Service IPC interface definition
│       ├── IAssessmentCallback.idl              # Callback IPC interface definition
│       ├── include                              # Client, error code and log tag header files
│       └── src                                  # Client implementation, error codes and SA load callback
├── patches
│   └── patches.json                             # Companion PRs of other repositories
├── services
│   └── assessment_service
│       ├── BUILD.gn                             # Service library build (libassessmentsvcs.z.so)
│       ├── etc
│       │   ├── assessment_service.cfg           # init service configuration (hosted by sa_main, on-demand start, service-side permissions)
│       │   └── param
│       │       ├── assessment.para              # System parameter default values
│       │       └── assessment.para.dac          # System parameter DAC
│       ├── include                              # Service header files (lifecycle management, event management, environment detection, device control, recovery management, security management)
│       ├── sa_profile
│       │   └── assessment_service.json          # SystemAbility configuration (SAID 8660)
│       └── src                                  # Service implementation (lifecycle management, event management, environment detection, device control, recovery management, security management)
├── test
│   └── unittest                                 # Unit tests (including mocks)
├── assessment.gni                               # Component paths and build switch definitions
├── BUILD.gn                                     # Component build entry
├── bundle.json                                  # Component description file
├── LICENSE                                      # License (Apache 2.0)
├── README.md                                    # Chinese README
└── README_en.md                                 # English README
```

## Build

This component is a native component in the OpenHarmony source tree and is built with GN and ninja.

### Requirements

- A synchronized full OpenHarmony source tree (standard system, for example the `rk3568` product)
- The component is added to the product configuration (`customization` subsystem)

### Build Commands

Run the commands in the source root directory and replace `<product name>` with the actual product name:

```bash
# Build the whole component
./build.sh --product-name <product name> --ccache --build-target assessment_configuration_service

# Build the service only
./build.sh --product-name <product name> --ccache --build-target base/customization/assessment_configuration_service/services/assessment_service:assessmentsvcs

# Build the unit tests
./build.sh --product-name <product name> --ccache --build-target base/customization/assessment_configuration_service/test/unittest:unittest
```

### Build Outputs

Table 3 Build outputs
| Output | Type | Description |
|--------|------|-------------|
| `libassessmentsvcs.z.so` | SA library | The Assessment Configuration Service itself |
| `libassessment_manager.z.so` | Inner Kit | System-side client library |
| `libassessment_napi.z.so` | NAPI | Application-side JS API library, installed to `module/customization` |
| `assessment_service.json` | SA profile | Deployed to `/system/profile` |
| `assessment_service.cfg` | init configuration | Deployed to `/system/etc/init` |
| `assessment.para` and `assessment.para.dac` | System parameters | Parameter default values and DAC configuration |

## Assessment Configuration Service Development

This service is developed in **C++** based on the SystemAbility framework and IDL IPC; the application-side JS APIs are exported through the NAPI. For the responsibilities of each directory, see the [Directory](#directory) section.


### Development Based on Existing Modules

Applicable scenarios: customizing existing capabilities, for example adjusting the assessment context management logic, extending the allowlist validation, modifying the countdown and release logic, or adding event subscriptions.

Identify the modification points by business boundary:

- Application-side API behavior: `frameworks/js/napi`;
- API signature: synchronize the IDL, the client and the NAPI;
- Business flow: `services/assessment_service`;
- Service permissions, parameter default values and start mode: `etc` and `sa_profile`.

See Table 4 for the item-by-item mapping.

Table 4 Modification locations
| Modification | Location | Notes |
|--------------|----------|-------|
| JS API behavior and parameter validation | `frameworks/js/napi/assessment/assessment_napi.cpp` | Exported functions are registered in `descriptors[]` with `DECLARE_NAPI_FUNCTION` |
| JS callback bridging | `frameworks/js/napi/assessment/js_assessment_callback.cpp` | Converts C++ callbacks into JS callbacks |
| IPC interface signature | `interfaces/inner_api/assessment_service/IAssessmentService.idl` and `IAssessmentCallback.idl` | The Proxy and Stub are generated by `idl_gen_interface` at build time; **do not handwrite them** |
| System-side C++ client | `interfaces/inner_api/assessment_service/include/assessment_service_client.h` and `src/assessment_service_client.cpp` | Loads the SA on demand and forwards the calls |
| Error codes / callback codes | `interfaces/inner_api/assessment_service/include/assessment_api_error_code.h`, `assessment_callback_code.h` and `src/assessment_api_error_code.cpp` | The enums and descriptions must be maintained in pairs |
| Assessment context management and business logic | `services/assessment_service/src/assessment_service.cpp` (header files are in `include/`) | Note the lock scope of `mutexSa_` and follow the `LockedUnsafe` naming convention |
| Environment detection | `services/assessment_service/src/env_checker.cpp` (header file is in `include/`) | Check item implementation |
| Device control | `services/assessment_service/src/process_controller.cpp` (header file is in `include/`) | Implements controls such as incoming-call rejection and disabling the softbus |
| Common event subscription and handling | The subscription list and the `DispatchEvent` handling branches are in `services/assessment_service/src/assessment_service.cpp`; the subscribe/unsubscribe actions are in `services/assessment_service/src/assessment_event_manager.cpp` | The subscription list and the event handling branches must be modified in pairs |
| Callback dispatch | `services/assessment_service/src/callback_manager.cpp` | Dispatches by caller token |
| Application process state observation | `services/assessment_service/src/assessment_service_app_state_cb.cpp` | Handles the exit of the caller |
| Utility functions such as permission verification and device type | `services/assessment_service/src/assessment_utils.cpp` | — |
| init configuration, system parameters and SA profile | `services/assessment_service/etc/` and `services/assessment_service/sa_profile/` | Modify the service permissions, parameter default values and start mode |
| Unit tests | `test/unittest/src/` | New test files must be registered in `sources` of the `BUILD.gn` in the same directory; see `test/unittest/mock/` for mocks |

Common modification scenarios are listed below:

**Scenario 1: Add a restriction applied during an assessment**

- Restrictions implemented by this service: add the control call in the entry flow of `services/assessment_service/src/assessment_service.cpp` and restore it in the exit flow in pairs; if it depends on system events, also add the subscription and the handling branch in `SubscribeCommonEvent()` and `assessment_event_manager.cpp` in pairs
- Restrictions implemented by Kiosk mode: the Kiosk mode capabilities must not be modified in this repository (see [Constraints](#constraints)); only the allowlist delivered by the caller can be adjusted, and the rest requires companion additions on the Ability Runtime framework side
- Restrictions implemented by cooperating system services: cooperating components take effect by themselves after listening to the Kiosk mode common events (which carry the assessment mode flag); they are not implemented in this repository, so adding them requires companion work on the corresponding component side
- If a restriction must be configurable per application or per scenario: extend the `config` fields and synchronize the IDL, the client and the NAPI instead of hardcoding it in the service

For example, to disable a system capability during an assessment: this item is implemented by this service, so add the control call in the entry flow of `services/assessment_service` and restore it in the exit flow in pairs. If it must take effect only for some applications or scenarios, add a switch field to `config`, pass it through the IPC chain to the service, and let the service decide by the field when applying the restriction.

**Scenario 2: Relax a restriction applied during an assessment**

- Restrictions implemented by this service (disabling the softbus and automatic incoming-call rejection): the modifications are concentrated in the entry and exit flows of `services/assessment_service/src/assessment_service.cpp`, as well as the event subscription and handling branches
- Restrictions implemented by Kiosk mode (the single-app mode, terminating applications that are not allowlisted and preventing them from being launched, prohibiting system screenshots and screen recording, picture-in-picture and floating windows, the AI assistant and text selection lookup): the Kiosk mode capabilities must not be modified in this repository (see [Constraints](#constraints)); only the allowlist delivered by the caller can be adjusted, and the rest requires companion modifications on the Ability Runtime framework side
- Restrictions implemented by cooperating system services (restricted notifications, prohibiting input method switching, the restricted lock screen and hiding interface elements such as the notification center): cooperating components take effect by themselves after listening to the Kiosk mode common events (which carry the assessment mode flag); they are not implemented in this repository, so relaxing them requires companion modifications on the corresponding component side
- If a restriction must be configurable per application or per scenario: extend the `config` fields and synchronize the IDL, the client and the NAPI instead of hardcoding it in the service

For example, to relax automatic incoming-call rejection during an assessment: this item is implemented directly by this service, so it can be relaxed at the corresponding control call in `services/assessment_service`. If it must be relaxed only for allowlisted applications, add a switch field to `config`, pass it through the IPC chain to the service, and let the service decide by the field when applying the restriction.

### New Feature Development

The following example uses **adding an application-side query API** (illustration: query the remaining assessment duration) to describe the complete steps and their dependencies.

#### Target Feature (Example)

Callers are expected to query the remaining duration of the current assessment to display a countdown in the UI. Therefore three capabilities are required at the same time: the **service-side computation**, the **IPC interface** and the **JS API**. The three steps correspond to these three capability chains, and the order is usually **service → then IPC → then JS**.

**Step 1: Extend the service capability (define how the remaining duration is computed in the service layer)**

Table 5 Problems the remaining-duration API must solve

| Problem | Description |
|---------|-------------|
| What to return when no assessment is active | Return `ERR_ASSESSMENT_NOT_ACTIVE` to avoid returning a meaningless value to the caller |
| Where the remaining duration comes from | Subtract the current time from `endpointCheckPoint_`; it must be read within the `mutexSa_` lock to avoid racing with the countdown thread |

**Step 2: Extend the IPC interface (connect the service implementation with the application-side call)**

- Declare `GetRemainingDuration` in `interfaces/inner_api/assessment_service/IAssessmentService.idl`; the Proxy and Stub are generated by `idl_gen_interface` at build time;
- Add the forwarding method with the same name in `assessment_service_client.h` and `assessment_service_client.cpp`, following the load and call pattern of `GetConfiguration`.

**Step 3: Export the JS API (let applications see and call the capability of Step 1)**

- Implement `AssessmentNapiGetRemainingDuration` in `assessment_napi.cpp` and register it in `descriptors[]`;
- Add test cases in `test/unittest/src/assessment_service_test.cpp`; new test files must be registered in `BUILD.gn`.

**Summary of the three steps**: Step 1 determines whether the value is correct; Step 2 determines whether the call can cross processes; Step 3 determines whether applications can use it. Missing any step leads to problems such as "the logic exists but cannot be called" or "the API exists but has no implementation".

### Verification

```bash
# Build the service
./build.sh --product-name <product name> --ccache --build-target base/customization/assessment_configuration_service/services/assessment_service:assessmentsvcs

# Build the unit tests
./build.sh --product-name <product name> --ccache --build-target base/customization/assessment_configuration_service/test/unittest:unittest
```

## Constraints

**Languages**: C++ and ArkTS

**Device types**: only 2in1, phone and tablet are supported (decided by `const.product.devicetype`; other device types return 801)

**Concurrency**: only one assessment at a time; a repeated begin returns 36700002, and end must use the same token as begin

**Allowlist**: the application list persisted in a system parameter contains at most 20 entries and a total length of no more than 2048 characters.

**Modification boundary**

- Must not be modified: the Kiosk mode capabilities (the single-app mode, terminating applications that are not allowlisted and preventing them from being launched, prohibiting system screenshots and screen recording, picture-in-picture, the floating ball and the floating control ball, the AI assistant and text selection lookup, and hiding the notification center, control center, live capsule, multitask center, start menu and three-key navigation) are provided by the Ability Runtime framework; the restrictions implemented by cooperating components (restricted notifications, prohibiting input method switching, the restricted lock screen and hiding interface elements such as the notification center) are implemented by the corresponding components; neither may be modified in this repository, and for Kiosk mode only the allowlist delivered by the caller can be adjusted.
- Can be modified: the capabilities provided by this service itself, including the allowlist reception and persistence logic, the assessment duration values, the entering/exiting/interrupted flows, the device control implemented by this service (automatic incoming-call rejection, disabling the distributed softbus, restricting the lock screen, etc.), the common event subscription and handling branches, the error codes and callback codes, the init configuration/system parameters/SA profile, and the unit tests.

## FAQ

**Why does begin return 36700002?**

Only one assessment is supported at a time. Call end to finish the current assessment before calling begin again.

**Is the assessment state kept after the service restarts?**

The values in the system parameters `persist.assessment.*` are kept, but the assessment is not restored: when the service is started again, it exits assessment mode directly and does not continue the countdown, so the application must call `begin` again.

## Guide

[Assessment Configuration Service application development guide](https://gitcode.com/weredust/docs_1670/blob/master/zh-cn/application-dev/assessment/assessment-guide.md)

[Assessment Configuration Service application development APIs](https://gitcode.com/weredust/docs_1670/blob/master/zh-cn/application-dev/reference/apis-assessment-kit/js-apis-customization-assessment.md)

## Contribution

The process and methods of contributing code and documentation are described in [Contribution](https://gitcode.com/openharmony/docs/blob/master/zh-cn/contribute/%E5%8F%82%E4%B8%8E%E8%B4%A1%E7%8C%AE.md).

## License

This component is licensed under [Apache 2.0](LICENSE).

## Changelog

Table 6 Changelog

| Date | Main Changes |
|------|--------------|
| 2026-08 | Initial open source release: provides the assessment mode configuration APIs (`begin`, `end`, `isActive` and `getConfiguration`) and the assessment state callbacks (`onBegin`, `onInterrupted` and `onEnd`), and supports Kiosk control, the assessment duration with automatic release, and assessment state persistence |
| 2026-09 | Integrated with the Ability Runtime APIs for entering and exiting Kiosk mode; added the process state observation of the third-party assessment application; renamed the callback code enum `SECURITY_BREACH` to `ENV_ANOMALY`; removed the references related to anco and low power management; added unit tests for the service and utility classes |

## Repositories

- [**openharmony-sig/customization\_assessment\_configuration\_service**](https://gitcode.com/openharmony-sig/customization_assessment_configuration_service) (this repository)
- [openharmony/ability\_runtime](https://gitcode.com/openharmony/ability_ability_runtime) (Kiosk mode, application state observation)
- [openharmony/ability\_base](https://gitcode.com/openharmony/ability_ability_base) (want and other base definitions)
- [openharmony/telephony\_call\_manager](https://gitcode.com/openharmony/telephony_call_manager) (call state and incoming call control)
- [openharmony/security\_selinux\_adapter](https://gitcode.com/openharmony/security_selinux_adapter) (SELinux policy)
- [openharmony/docs](https://gitcode.com/openharmony/docs) (development guide and API references)
