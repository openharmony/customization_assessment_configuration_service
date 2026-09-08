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

#ifndef ASSESSMENT_SERVICE_APP_STATE_CB_H
#define ASSESSMENT_SERVICE_APP_STATE_CB_H

#include "application_state_observer_stub.h"
namespace OHOS {
namespace AAFwk {

using AssessmentAppStateFunc = std::function<void(const std::string&)>;

class AssessmentServiceAppStateCb : public AppExecFwk::ApplicationStateObserverStub {
public:
    AssessmentServiceAppStateCb(AssessmentAppStateFunc func)
        : func_(func) {}
    ~AssessmentServiceAppStateCb() {}

    void OnForegroundApplicationChanged(const AppExecFwk::AppStateData& appStateData) override;
    void OnApplicationStateChanged(const AppExecFwk::AppStateData& appStateData) override;
    void OnProcessDied(const AppExecFwk::ProcessData &processData) override;
private:
    AssessmentAppStateFunc func_;
};
} // namespace AAFwk
} // namespace OHOS
#endif // ASSESSMENT_SERVICE_APP_STATE_CB_H