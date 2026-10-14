#ifndef OHOS_AAFwk_ASSESSMENT_MODEL_STATUS_CALLBACK_H
#define OHOS_AAFwk_ASSESSMENT_MODEL_STATUS_CALLBACK_H

#include <functional>
#include "ipc_object_stub.h"
#include "message_option.h"
#include "message_parcel.h"

namespace OHOS {
namespace AAFwk {

using AssessmentModelStatusCallback = std::function<void(int32_t status, int32_t error, uint32_t paogress)>;

class AssessmentModelStatusCallbackStub : public IPCObjectStub {
public:
    explicit AssessmentModelStatusCallbackStub(AssessmentModelStatusCallback callback);
    ~AssessmentModelStatusCallbackStub() = default;

    virtual int OnRemoteRequest(uint32_t code, MessageParcel& data,
        MessageParcel& reply, MessageOption& option) override;

private:
    AssessmentModelStatusCallback callback_;
};
} // namespace AAFwk
} // namespace OHOS
#endif