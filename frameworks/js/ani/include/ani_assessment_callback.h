/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
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

#ifndef ANI_ASSESSMENT_CALLBACK_H
#define ANI_ASSESSMENT_CALLBACK_H

#include "ani.h"
#include "ipc_object_stub.h"
#include "message_option.h"
#include "message_parcel.h"
#include "event_handler.h"

namespace OHOS {
namespace AAFwk {

class AniAssessmentCallback : public IPCObjectStub {
public:
    AniAssessmentCallback(ani_env* env, ani_ref onBeginRef, ani_ref onInterruptedRef, ani_ref onEndRef);
    virtual ~AniAssessmentCallback();

    int OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override;

    ani_ref GetOnBeginRef() const;
    ani_ref GetOnInterruptedRef() const;
    ani_ref GetOnEndRef() const;

    void CallJsOnBegin(int32_t code, const std::string& message);
    void CallJsOnInterrupted(int32_t reason, const std::string& message);
    void CallJsOnEnd();

private:
    ani_env* env_ = nullptr;
    ani_vm* vm_ = nullptr;
    ani_ref onBeginRef_ = nullptr;
    ani_ref onInterruptedRef_ = nullptr;
    ani_ref onEndRef_ = nullptr;
    std::shared_ptr<AppExecFwk::EventHandler> eventHandler_ = nullptr;
};

} // namespace AAFwk
} // namespace OHOS
#endif // ANI_ASSESSMENT_CALLBACK_H
