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
#include <gtest/gtest.h>

#include "assessment_utils.h"
#include "syspara/parameters.h"

#define private public

using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace AAFwk {
namespace TEST {

class AssessmentUtilsTest : public testing::Test {
public:
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

protected:
    void SetUp() override {}

    void TearDown() override {};
};

/**
 * @tc.name: AssessmentUtilsGenerateRandomString
 * @tc.desc: Test AssessmentUtils::GenerateRandomString.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentUtilsTest, AssessmentUtilsGenerateRandomString, TestSize.Level1)
{
    size_t w = 16;
    std::string z = OHOS::AAFwk::AssessmentServiceUtils::GenerateRandomString(w);
    EXPECT_EQ(z.length(), w);
}

/**
 * @tc.name: CheckDeviceTypeSupported
 * @tc.desc: Test AssessmentUtils::CheckDeviceTypeSupported.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentUtilsTest, CheckDeviceTypeSupported, TestSize.Level1)
{
    auto deviceType = OHOS::system::GetParameter("const.product.devicetype", "");
    bool exp = deviceType == "2in1" || deviceType == "phone" || deviceType == "tablet";
    bool ret = OHOS::AAFwk::AssessmentServiceUtils::CheckDeviceTypeSupported();
    EXPECT_EQ(ret, exp);
}

/**
 * @tc.name: VerifyCallingPermission01
 * @tc.desc: Test AssessmentUtils::VerifyCallingPermission.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentUtilsTest, VerifyCallingPermission01, TestSize.Level1)
{
    std::string permission = "ohos.not.found";
    EXPECT_NO_FATAL_FAILURE(AssessmentServiceUtils::VerifyCallingPermission(permission, 0));
}

/**
 * @tc.name: VerifyCallingPermission02
 * @tc.desc: Test AssessmentUtils::VerifyCallingPermission.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentUtilsTest, VerifyCallingPermission02, TestSize.Level1)
{
    std::string permission = "ohos.not.found";
    EXPECT_NO_FATAL_FAILURE(AssessmentServiceUtils::VerifyCallingPermission(permission, 1));
}

/**
 * @tc.name: VerifyCallingPermission03
 * @tc.desc: Test AssessmentUtils::VerifyCallingPermission.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentUtilsTest, VerifyCallingPermission03, TestSize.Level1)
{
    std::string permission = "ohos.permission.ASSESSMENT_CONFIGURATION";
    EXPECT_NO_FATAL_FAILURE(AssessmentServiceUtils::VerifyCallingPermission(permission, 0));
}

/**
 * @tc.name: VerifyCallingPermission04
 * @tc.desc: Test AssessmentUtils::VerifyCallingPermission.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentUtilsTest, VerifyCallingPermission04, TestSize.Level1)
{
    std::string permission = "ohos.permission.ASSESSMENT_CONFIGURATION";
    EXPECT_NO_FATAL_FAILURE(AssessmentServiceUtils::VerifyCallingPermission(permission, 1));
}
} // TEST
} // AAFwk
} // OHOS