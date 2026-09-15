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
#include "assessment_service_app_state_cb.h"
#undef private

using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace AAFwk {
namespace TEST {

class AssessmentServiceAppStateTest : public testing::Test {
public:
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

protected:
    void SetUp() override {}

    void TearDown() override {};
};

/**
 * @tc.name: OnForegroundApplicationChanged
 * @tc.desc: Test AssessmentServiceAppStateTest::OnForegroundApplicationChanged.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceAppStateTest, OnForegroundApplicationChanged, TestSize.Level1)
{
    auto func = [](const std::string &) {};
    AssessmentServiceAppStateCb cb(func);
    AppExecFwk::AppStateData data;
    EXPECT_NO_FATAL_FAILURE(cb.OnForegroundApplicationChanged(data));
}

/**
 * @tc.name: OnApplicationStateChanged
 * @tc.desc: Test AssessmentServiceAppStateTest::OnApplicationStateChanged.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceAppStateTest, OnApplicationStateChanged, TestSize.Level1)
{
    auto func = [](const std::string &) {};
    AssessmentServiceAppStateCb cb(func);
    AppExecFwk::AppStateData data;
    EXPECT_NO_FATAL_FAILURE(cb.OnApplicationStateChanged(data));
}

/**
 * @tc.name: OnProcessDied
 * @tc.desc: Test AssessmentServiceAppStateTest::OnProcessDied.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceAppStateTest, OnProcessDied, TestSize.Level1)
{
    auto func = [](const std::string &) {};
    AssessmentServiceAppStateCb cb(func);
    AppExecFwk::ProcessData data;
    EXPECT_NO_FATAL_FAILURE(cb.OnProcessDied(data));
}
} // TEST
} // AAFwk
} // OHOS