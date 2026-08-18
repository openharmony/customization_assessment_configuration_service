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

#include "assessment_service_load_callback.h"

#include "assessment_service_client.h"
#include "hilog_tag_wrapper.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace AAFwk {
void AssessmentServiceLoadCallback::OnLoadSystemAbilitySuccess(int32_t systemAbilityId,
                                                               const sptr<IRemoteObject> &remoteObject)
{
    TAG_LOGI(AAFwkTag::DEFAULT, "OnLoadSystemAbilitySuccess, systemAbilityId: %{public}d", systemAbilityId);
    auto client = AssessmentServiceClient::GetInstance();
    if (client != nullptr) {
        client->OnLoadSystemAbilitySuccess(remoteObject);
    }
}

void AssessmentServiceLoadCallback::OnLoadSystemAbilityFail(int32_t systemAbilityId)
{
    TAG_LOGE(AAFwkTag::DEFAULT, "OnLoadSystemAbilityFail, systemAbilityId: %{public}d", systemAbilityId);
    auto client = AssessmentServiceClient::GetInstance();
    if (client != nullptr) {
        client->OnLoadSystemAbilityFail();
    }
}
} // namespace AAFwk
} // namespace OHOS