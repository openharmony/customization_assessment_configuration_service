#ifndef ASSESSMENT_SERVICE_APP_STATE_CB_H
#define ASSESSMENT_SERVICE_APP_STATE_CB_H

#include "application_state_observer_stub.h"
namespace OHOS {
namespace AAFWK {
class AssessmentServiceAppStateCb : public AppExecFwk::ApplicationStateObserverStub {
    public:
        AssessmentServiceAppStateCb();
        ~AssessmentServiceAppStateCb();

        void OnForegroundApplicationChanged(const AppExecFwk::AppStateData& appStateData) override;
        void OnApplicationStateChanged(const AppExecFwk::AppStateData& appStateData) override;
        void OnProcessDied(const AppExecFwk::ProcessData &processData) override;
};
}
}
#endif