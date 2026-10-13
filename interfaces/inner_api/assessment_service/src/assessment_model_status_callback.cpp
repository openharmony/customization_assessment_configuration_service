#include "assessment_model_status_callback.h"
#include "hilog_tag_wrapper.h"

namespace OHOS {
namespace AAFwk {
namespace {
enum class AssessmentModelStatusCallbackCode {
    ON_MODEL_STATUS_CHANGED = 0,
};
}

AssessmentModelStatusCallbackStub::AssessmentModelStatusCallbackStub(AssessmentModelStatusCallback callback)
    : IPCObjectStub(u"AssessmentModelStatusCallbackStub", false), callback_(callback)
{}

int AssessmentModelStatusCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "OnRemoteRequest code: %{public}u", code);

    if(code == static_cast<uint32_t>(AssessmentModelStatusCallbackCode::ON_MODEL_STATUS_CHANGED)) {
        int32_t status = data.ReadInt32();
        int32_t error = data.ReadInt32();
        uint32_t progress = data.ReadUint32();

        if(callback_) {
            callback_(status, error, progress);
        }
    }
    return 0;
}
} // namespace AAFwk
} // namespace OHOS