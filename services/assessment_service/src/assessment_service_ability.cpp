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

#include "assessment_service_ability.h"

#include "hilog_tag_wrapper.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace AAFwk {
REGISTER_SYSTEM_ABILITY_BY_ID(AssessmentServiceAbility, ASSESSMENT_SERVICE_ID, true);

AssessmentServiceAbility::AssessmentServiceAbility(const int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate), service_(nullptr)
{}

AssessmentServiceAbility::~AssessmentServiceAbility()
{
    TAG_LOGD(AAFwkTag::DEFAULT, "called");
}

void AssessmentServiceAbility::OnStart()
{
    TAG_LOGI(AAFwkTag::DEFAULT, "called");
    if (service_ != nullptr) {
        TAG_LOGD(AAFwkTag::DEFAULT, "Assessment service has started");
        return;
    }

    service_ = AssessmentService::GetInstance();
    if (service_ == nullptr) {
        TAG_LOGE(AAFwkTag::DEFAULT, "null instance");
        return;
    }

    if (!service_->Init()) {
        TAG_LOGE(AAFwkTag::DEFAULT, "init failed");
        return;
    }

    if (!Publish(service_)) {
        TAG_LOGE(AAFwkTag::DEFAULT, "Publish failed");
        return;
    }
}

void AssessmentServiceAbility::OnStop()
{
    service_ = nullptr;
}
}  // namespace AAFwk
}  // namespace OHOS
