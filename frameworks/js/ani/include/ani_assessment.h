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

#ifndef ANI_ASSESSMENT_H
#define ANI_ASSESSMENT_H

#include "ani.h"
#include "ani_assessment_callback.h"

namespace OHOS {
namespace AbilityRuntime {

class AniAssessment {
public:
    AniAssessment();
    ~AniAssessment();

    static ani_status Init(ani_env* env, ani_namespace nsp);
    static ani_boolean IsActiveAni(ani_env* env);
    static ani_object GetConfigurationAni(ani_env* env);
    static ani_status EndAni(ani_env* env, ani_object context);
    static ani_status BeginAni(ani_env* env,
                               ani_long nativeObj,
                               ani_object context,
                               ani_object config,
                               ani_object beginCallback,
                               ani_object interruptedCallback,
                               ani_object endCallback);

private:
    ani_status OnBeginAni(ani_env* env,
                          ani_object context,
                          ani_object config,
                          ani_object beginCallback,
                          ani_object interruptedCallback,
                          ani_object endCallback);
    ani_status CheckParamsOfBegin(ani_env* env,
                                  ani_object context,
                                  ani_object config,
                                  std::string& selfBundleName,
                                  sptr<IRemoteObject>& token);
    static ani_object CreateAssessmentConfigObject(ani_env* env,
                                                   uint32_t duration,
                                                   const std::vector<std::string>& allowedApps);
    sptr<AAFwk::AniAssessmentCallback> CreateAniAssessmentCallback(ani_env* env,
                                                                   ani_object beginCallback,
                                                                   ani_object interruptedCallback,
                                                                   ani_object endCallback);
};

} // namespace AbilityRuntime
} // namespace OHOS
#endif // ANI_ASSESSMENT_H