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

#ifndef OHOS_ASSESSMENT_UTILS_H
#define OHOS_ASSESSMENT_UTILS_H

#include <string>
#include <cstdint>

namespace OHOS {
namespace AAFwk {

constexpr const char* PERMISSION_ASSESSMENT_CONFIGURATION = "ohos.permission.ASSESSMENT_CONFIGURATION";

class AssessmentServiceUtils {
public:
    static std::string GenerateRandomString(size_t length);
    static uint64_t GenerateRandomExamId();
    static uint64_t GetCurrentAssessmentTimeStamp();
    static bool VerifyCallingPermission(
        const std::string &permissionName, const uint32_t specifyTokenId = 0);
    /**
     * @brief Returns the product device type (e.g. "2in1", "phone", "tablet").
     *
     * The value is read from system parameters once and cached, since the
     * device type never changes at runtime. The returned reference is to a
     * function-local static that lives for the whole program, so it is safe
     * to hold; callers must treat it as read-only.
     */
    static const std::string& GetDeviceType();
    static bool CheckDeviceTypeSupported();
};

} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_ASSESSMENT_UTILS_H

