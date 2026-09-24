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
#include "mock_assessment_constants.h"

#include <random>
#include <chrono>

#include "hilog_tag_wrapper.h"

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

const std::string& AssessmentServiceUtils::GetDeviceType()
{
    static const std::string deviceType = "phone";
    return deviceType;
}

bool AssessmentServiceUtils::VerifyCallingPermission(
    const std::string &, const uint32_t)
{
    return OHOS::AAFwk::TEST::AssessmentTestConstants().GetInstance().VerifyCallingPermissionReturn;
}

bool AssessmentServiceUtils::CheckDeviceTypeSupported()
{
    return OHOS::AAFwk::TEST::AssessmentTestConstants().GetInstance().CheckDeviceTypeSupported;
}

uint64_t AssessmentServiceUtils::GenerateRandomExamId()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    return gen();
}

uint64_t AssessmentServiceUtils::GetCurrentAssessmentTimeStamp()
{
    auto currentTimeStamp = std::chrono::system_clock::now().time_since_epoch();
    auto currentTimeStampInMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimeStamp).count();
    return currentTimeStampInMs;
}
}  // namespace AAFwk
}  // namespace OHOS