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
#ifndef OHOS_ASSESSMENT_MODEL_STATUS_CALLBACK_H
#define OHOS_ASSESSMENT_MODEL_STATUS_CALLBACK_H

#include <functional>
#include "ipc_object_stub.h"
#include "message_option.h"
#include "message_parcel.h"

namespace OHOS {
namespace AAFwk {

using AssessmentModelStatusCallback = std::function<void(int32_t status, int32_t error, uint32_t progress)>;

class AssessmentModelStatusCallbackStub : public IPCObjectStub {
public:
    explicit AssessmentModelStatusCallbackStub(AssessmentModelStatusCallback callback);
    ~AssessmentModelStatusCallbackStub() = default;

    virtual int OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply,
        MessageOption& option) override;

private:
    AssessmentModelStatusCallback callback_;
};
} // namespace AAFwk
} // namespace OHOS
#endif