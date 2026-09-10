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
#include "assessment_service.h"
#include "assessment_service_ability.h"
#undef private

using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace AAFwk {
namespace TEST {

class AssessmentServiceAbilityTest : public testing::Test {
public:
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

protected:
    void SetUp() override {}

    void TearDown() override {};
};

/**
 * @tc.name: AssessmentServiceAbility
 * @tc.desc: Test AssessmentServiceAbility::AssessmentServiceAbility.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceAbilityTest, AssessmentServiceAbility, TestSize.Level1)
{
    int32_t said = 8660;
    AssessmentServiceAbility sa(said, false);
    EXPECT_EQ(sa.service_, nullptr);
}

/**
 * @tc.name: OnStart
 * @tc.desc: Test AssessmentServiceAbility::OnStart.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceAbilityTest, OnStart, TestSize.Level1)
{
    int32_t said = 8660;
    AssessmentServiceAbility sa(said, false);
    sa.service_ = AssessmentService::GetInstance();
    EXPECT_NO_FATAL_FAILURE(sa.OnStart());
}

/**
 * @tc.name: OnStop
 * @tc.desc: Test AssessmentServiceAbility::OnStop.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceAbilityTest, OnStop, TestSize.Level1)
{
    int32_t said = 8660;
    AssessmentServiceAbility sa(said, false);
    EXPECT_NO_FATAL_FAILURE(sa.OnStop());
    EXPECT_EQ(sa.service_, nullptr);
}

} // TEST
} // AAFwk
} // OHOS