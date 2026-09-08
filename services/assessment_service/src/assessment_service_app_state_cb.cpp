#include "assessment_service_app_state_cb.h"
#include "hilog_tag_wrapper.h"
namespace OHOS {
namespace AAFWK {
AssessmentServiceAppStateCb::AssessmentServiceAppStateCb()
{
}

AssessmentServiceAppStateCb::~AssessmentServiceAppStateCb()
{
}

    void AssessmentServiceAppStateCb::OnForegroundApplicationChanged(const AppExecFwk::AppStateData& appStateData
    {};

    void AssessmentServiceAppStateCb::OnApplicationStateChanged(const AppExecFwk::AppStateData& appStateData)
    {};

    void AssessmentServiceAppStateCb::OnProcessDied(const AppExecFwk::ProcessData &processData)
    {
        TAG_LOGE(AAFwkTag::DEFAULT, "==== APP Process Died ====");
        TAG_LOGE(AAFwkTag::DEFAULT, "bundleName: %{public}s", processData.bundleName.c_str());
    };
}
}
