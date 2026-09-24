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
#include <cerrno>
#include <chrono>

#include <sys/random.h>
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

uint64_t AssessmentServiceUtils::GenerateRandomExamId()
{
    uint64_t result = 0;
    ssize_t ret = getrandom(&result, sizeof(result), GRND_NONBLOCK);
    if (ret != sizeof(result)) {
        TAG_LOGE(AAFwkTag::DEFAULT, "GenerateRandomExamId failed, ret=%{public}zd, errno=%{public}d", ret, errno);
        return 0;
    }
    return result;
}

uint64_t AssessmentServiceUtils::GetCurrentAssessmentTimeStamp()
{
    auto currentTimeStamp = std::chrono::system_clock::now().time_since_epoch();
    auto currentTimeStampInMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimeStamp).count();
    return currentTimeStampInMs;
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

const std::string& AssessmentServiceUtils::GetDeviceType()
{
    static const std::string deviceType = OHOS::system::GetParameter("const.product.devicetype", "");
    return deviceType;
}

bool AssessmentServiceUtils::CheckDeviceTypeSupported()
{
    const std::string& deviceType = GetDeviceType();
    if (deviceType == "2in1" || deviceType == "phone" || deviceType == "tablet") {
        return true;
    }
    TAG_LOGI(AAFwkTag::DEFAULT, "assessment not supported device_type: %{public}s", deviceType.c_str());
    return false;
}
}  // namespace AAFwk
}  // namespace OHOS
