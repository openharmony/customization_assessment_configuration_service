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

#include "assessment_utils.h"

#include <random>

#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "hilog_tag_wrapper.h"
#include "syspara/parameters.h"

namespace OHOS {
namespace AAFwk {

std::string AssessmentServiceUtils::GenerateRandomString(size_t length)
{
    const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);

    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; i++) {
        result += chars[dis(gen)];
    }

    return result;
}

bool AssessmentServiceUtils::VerifyCallingPermission(
    const std::string &permissionName, const uint32_t specifyTokenId)
{
    TAG_LOGD(AAFwkTag::DEFAULT, "permission %{public}s, specifyTokenId: %{public}u",
        permissionName.c_str(), specifyTokenId);
    auto callerToken = specifyTokenId == 0 ? IPCSkeleton::GetCallingTokenID() : specifyTokenId;
    TAG_LOGD(AAFwkTag::DEFAULT, "Token: %{public}u", callerToken);
    int32_t ret = Security::AccessToken::AccessTokenKit::VerifyAccessToken(callerToken, permissionName, false);
    if (ret != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
        TAG_LOGW(AAFwkTag::DEFAULT, "%{public}s: PERMISSION_DENIED", permissionName.c_str());
        return false;
    }
    TAG_LOGD(AAFwkTag::DEFAULT, "verify Token success");
    return true;
}

bool AssessmentServiceUtils::CheckDeviceTypeSupported()
{
    std::string deviceType = OHOS::system::GetParameter("const.product.devicetype", "");
    if (deviceType == "2in1" || deviceType == "phone" || deviceType == "tablet") {
        return true;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment not supported device_type: %{public}s", deviceType.c_str());
    return false;
}
}  // namespace AAFwk
}  // namespace OHOS