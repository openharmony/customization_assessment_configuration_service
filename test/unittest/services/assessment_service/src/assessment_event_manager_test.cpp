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

#define private public
#include "assessment_event_manager.h"
#undef private

#include "common_event_support.h"
#include "mock_i_remote_object.h"

using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace AAFwk {
namespace TEST {

class AssessmentEventManagerTest : public testing::Test {
public:
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

protected:
    void SetUp() override {}

    void TearDown() override {};
};

sptr<IRemoteObject> CreateMockToken()
{
    return new MockIRemoteObject();
}

/**
 * @tc.name: Create
 * @tc.desc: Test AssessmentEventObserver::Create.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentEventManagerTest, Create, TestSize.Level1)
{
    OHOS::EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_ENTER_HIBERNATE);
    OHOS::EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);

    auto svr = AssessmentEventObserver::Create(subscribeInfo,
        [](const OHOS::EventFwk::CommonEventData&) {});
    EXPECT_NE(svr, nullptr);
}

/**
 * @tc.name: GetPtr
 * @tc.desc: Test AssessmentEventObserver::GetPtr.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentEventManagerTest, GetPtr, TestSize.Level1)
{
    OHOS::EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_ENTER_HIBERNATE);
    OHOS::EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);

    auto svr = AssessmentEventObserver::Create(subscribeInfo,
        [](const OHOS::EventFwk::CommonEventData&) {});
    EXPECT_NE(svr, nullptr);

    auto ret = svr->GetPtr();
    EXPECT_NE(ret, nullptr);
}

/**
 * @tc.name: Subscribe
 * @tc.desc: Test AssessmentEventObserver::Subscribe.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentEventManagerTest, Subscribe, TestSize.Level1)
{
    OHOS::EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_ENTER_HIBERNATE);
    OHOS::EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);

    auto svr = AssessmentEventObserver::Create(subscribeInfo,
        [](const OHOS::EventFwk::CommonEventData&) {});
    EXPECT_NE(svr, nullptr);

    EXPECT_NO_FATAL_FAILURE(svr->Subscribe());
    EXPECT_NO_FATAL_FAILURE(svr->Unsubscribe());
}

/**
 * @tc.name: Unsubscribe
 * @tc.desc: Test AssessmentEventObserver::Unsubscribe.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentEventManagerTest, Unsubscribe, TestSize.Level1)
{
    OHOS::EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_ENTER_HIBERNATE);
    OHOS::EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);

    auto svr = AssessmentEventObserver::Create(subscribeInfo,
        [](const OHOS::EventFwk::CommonEventData&) {});
    EXPECT_NE(svr, nullptr);
    EXPECT_NO_FATAL_FAILURE(svr->Unsubscribe());
}

/**
 * @tc.name: OnReceiveEvent
 * @tc.desc: Test AssessmentEventObserver::OnReceiveEvent.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentEventManagerTest, OnReceiveEvent, TestSize.Level1)
{
    OHOS::EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_ENTER_HIBERNATE);
    OHOS::EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);

    auto svr = AssessmentEventObserver::Create(subscribeInfo,
        [](const OHOS::EventFwk::CommonEventData&) {});
    EXPECT_NE(svr, nullptr);
    
    OHOS::EventFwk::CommonEventData data;
    EXPECT_NO_FATAL_FAILURE(svr->OnReceiveEvent(data));
}

/**
 * @tc.name: OnReceiveEvent02
 * @tc.desc: Test AssessmentEventObserver::OnReceiveEvent.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentEventManagerTest, OnReceiveEvent02, TestSize.Level1)
{
    OHOS::EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_ENTER_HIBERNATE);
    OHOS::EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);

    auto svr = AssessmentEventObserver::Create(subscribeInfo, nullptr);
    EXPECT_NE(svr, nullptr);
    
    OHOS::EventFwk::CommonEventData data;
    EXPECT_NO_FATAL_FAILURE(svr->OnReceiveEvent(data));
}

/**
 * @tc.name: AssessmentAbilityConnection
 * @tc.desc: Test AssessmentAbilityConnection::AssessmentAbilityConnection.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentEventManagerTest, AssessmentAbilityConnection, TestSize.Level1)
{
    std::string bundleName = "com.test.demo";
    std::string abilityName = "";
    std::string cmdStr = "";
    AssessmentAbilityConnection conn(bundleName, abilityName, cmdStr);
    EXPECT_EQ(bundleName, conn.bundleName_);
}

/**
 * @tc.name: OnAbilityConnectDone
 * @tc.desc: Test AssessmentAbilityConnection::OnAbilityConnectDone.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentEventManagerTest, OnAbilityConnectDone, TestSize.Level1)
{
    std::string bundleName = "com.test.demo";
    std::string abilityName = "";
    std::string cmdStr = "";
    AssessmentAbilityConnection conn(bundleName, abilityName, cmdStr);
    int32_t resultCode = 0;
    AppExecFwk::ElementName element;
    EXPECT_NO_FATAL_FAILURE(conn.OnAbilityConnectDone(element, nullptr, resultCode));
}

/**
 * @tc.name: OnAbilityConnectDone02
 * @tc.desc: Test AssessmentAbilityConnection::OnAbilityConnectDone.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentEventManagerTest, OnAbilityConnectDone02, TestSize.Level1)
{
    std::string bundleName = "com.test.demo";
    std::string abilityName = "";
    std::string cmdStr = "";
    AssessmentAbilityConnection conn(bundleName, abilityName, cmdStr);
    int32_t resultCode = 0;
    AppExecFwk::ElementName element;
    sptr<IRemoteObject> token = CreateMockToken();
    EXPECT_NO_FATAL_FAILURE(conn.OnAbilityConnectDone(element, token, resultCode));
}

/**
 * @tc.name: OnAbilityDisconnectDone
 * @tc.desc: Test AssessmentAbilityConnection::OnAbilityDisconnectDone.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentEventManagerTest, OnAbilityDisconnectDone, TestSize.Level1)
{
    std::string bundleName = "com.test.demo";
    std::string abilityName = "";
    std::string cmdStr = "";
    AssessmentAbilityConnection conn(bundleName, abilityName, cmdStr);
    int32_t resultCode = 0;
    AppExecFwk::ElementName element;
    EXPECT_NO_FATAL_FAILURE(conn.OnAbilityDisconnectDone(element, resultCode));
}
} // TEST
} // AAFwk
} // OHOS