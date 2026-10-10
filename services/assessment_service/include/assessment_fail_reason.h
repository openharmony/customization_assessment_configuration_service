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

#ifndef OHOS_AAFWK_ASSESSMENT_FAIL_REASON_H
#define OHOS_AAFWK_ASSESSMENT_FAIL_REASON_H

namespace OHOS {
namespace AAFwk {

// Vocabulary of the failure reasons carried in the std::string &failReason
// out-parameter of EnvChecker::CheckAll, ProcessController::Activate/Deactivate and
// the closed-source extension entry points.
//
// It lives in a header because the reasons are produced outside
// assessment_service.cpp -- by env_checker.cpp, by process_controller.cpp and by
// the closed-source extension -- while the TRACE_* constants of the trace events
// are local to assessment_service.cpp and therefore not visible there.
//
// A reason is forwarded verbatim as the errorReason of a trace event, and
// translated into the callback message by AssessmentFailReasonToErrMsg(), so that
// every environment failure can keep reporting the shared
// AssessmentErrorCode::ENV_ANOMALY code while still telling the concrete cause
// apart. The closed-source extension fills the same vocabulary, which lets one
// entry point report which of its sub-items (virtual machine, file transfer,
// screen reader, ...) failed without the open-source side knowing about them.

// Environment checks implemented in the open-source tree.
inline constexpr const char *const FAIL_REASON_SCREEN_RECORDING = "SCREEN_RECORDING_DETECTED";
inline constexpr const char *const FAIL_REASON_SCREEN_CASTING = "SCREEN_CASTING_DETECTED";
inline constexpr const char *const FAIL_REASON_MULTI_SCREEN = "MULTI_SCREEN_DETECTED";
inline constexpr const char *const FAIL_REASON_IN_CALL = "IN_CALL_DETECTED";
inline constexpr const char *const FAIL_REASON_VIRTUAL_MACHINE = "VM_DETECTED";
inline constexpr const char *const FAIL_REASON_EDM_ADMIN = "EDM_ADMIN_PRESENT";

// Process control sub-items driven from the open-source tree.
inline constexpr const char *const FAIL_REASON_SOFTBUS_CONTROL = "ENABLE_SOFTBUS_FAILED";
inline constexpr const char *const FAIL_REASON_CALL_OBSERVER = "LISTEN_CALLING_STATE_FAILED";
inline constexpr const char *const FAIL_REASON_SCREEN_LISTENER = "REGISTER_SCREEN_LISTENER_FAILED";

// Fallback used when the closed-source extension reports a failure but leaves
// failReason empty, so a trace event and a callback message are never blank.
inline constexpr const char *const FAIL_REASON_ENABLE_VM = "ENABLE_VM_FAILED";

// Assessment interruption triggered by an external screen being plugged in.
inline constexpr const char *const EXIT_REASON_EXTERNAL_MONITOR = "EXTERNAL_MONITOR";

} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_AAFWK_ASSESSMENT_FAIL_REASON_H
