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
#include "callback_manager.h"
#undef private

#include "mock_i_remote_object.h"

using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace AAFwk {
namespace TEST {

class AssessmentCallbackManagerTest : public testing::Test {
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
 * @tc.name: AssessmentCallbackManagerTestCallbackManagerGetInstance
 * @tc.desc: Test CallbackManager::GetInstance.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentCallbackManagerTest, CallbackManagerGetInstance, TestSize.Level1)
{
    auto &instance1 = CallbackManager::GetInstance();
    auto &instance2 = CallbackManager::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

/**
 * @tc.name: RegisterCallback
 * @tc.desc: Test CallbackManager::RegisterCallback.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentCallbackManagerTest, RegisterCallback, TestSize.Level1)
{
    auto &instance = CallbackManager::GetInstance();

    sptr<IRemoteObject> token = CreateMockToken();
    sptr<IRemoteObject> callback = CreateMockToken();

    int32_t ret1 = instance.RegisterCallback(nullptr, nullptr);
    EXPECT_EQ(ret1, -1);

    int32_t ret2 = instance.RegisterCallback(token, nullptr);
    EXPECT_EQ(ret2, -1);

    int32_t ret3 = instance.RegisterCallback(token, callback);
    EXPECT_EQ(ret3, 0);
    instance.ClearAllCallbacks();
}

/**
 * @tc.name: ClearAllCallbacks
 * @tc.desc: Test CallbackManager::ClearAllCallbacks.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentCallbackManagerTest, ClearAllCallbacks, TestSize.Level1)
{
    auto &instance = CallbackManager::GetInstance();
    sptr<IRemoteObject> token = CreateMockToken();
    sptr<IRemoteObject> callback = CreateMockToken();
    instance.RegisterCallback(token, callback);

    EXPECT_NO_FATAL_FAILURE(instance.ClearAllCallbacks());
    int sz = instance.callbacks_.size();
    EXPECT_EQ(sz, 0);
}

/**
 * @tc.name: ClearAllCallbacks
 * @tc.desc: Test CallbackManager::ClearAllCallbacks.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentCallbackManagerTest, UnregisterCallback, TestSize.Level1)
{
    auto &instance = CallbackManager::GetInstance();
    sptr<IRemoteObject> token = CreateMockToken();
    sptr<IRemoteObject> callback = CreateMockToken();
    instance.RegisterCallback(token, callback);

    int ret = instance.UnregisterCallback(nullptr);
    EXPECT_EQ(ret, -1);

    sptr<IRemoteObject> token2 = CreateMockToken();
    ret = instance.UnregisterCallback(token2);
    EXPECT_EQ(ret, 0);

    ret = instance.UnregisterCallback(token);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: GetCallback
 * @tc.desc: Test CallbackManager::GetCallback.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentCallbackManagerTest, GetCallback, TestSize.Level1)
{
    auto &instance = CallbackManager::GetInstance();
    sptr<IRemoteObject> token = CreateMockToken();
    sptr<IRemoteObject> callback = CreateMockToken();
    instance.RegisterCallback(token, callback);

    auto ret = instance.GetCallback(nullptr);
    EXPECT_EQ(ret, nullptr);

    sptr<IRemoteObject> token2 = CreateMockToken();
    auto ret1 = instance.GetCallback(token2);
    EXPECT_EQ(ret1, nullptr);

    auto ret2 = instance.GetCallback(token);
    EXPECT_EQ(ret2, callback);
}

/**
 * @tc.name: OnBegin
 * @tc.desc: Test CallbackManager::OnBegin.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentCallbackManagerTest, OnBegin, TestSize.Level1)
{
    auto &instance = CallbackManager::GetInstance();
    sptr<IRemoteObject> token = CreateMockToken();
    sptr<IRemoteObject> callback = CreateMockToken();
    instance.RegisterCallback(token, callback);

    sptr<IRemoteObject> token1 = CreateMockToken();
    EXPECT_NO_FATAL_FAILURE(instance.OnBegin(token1, 0, ""));
    EXPECT_NO_FATAL_FAILURE(instance.OnBegin(token, 0, ""));
}

/**
 * @tc.name: OnInterrupted
 * @tc.desc: Test CallbackManager::OnInterrupted.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentCallbackManagerTest, OnInterrupted, TestSize.Level1)
{
    auto &instance = CallbackManager::GetInstance();
    sptr<IRemoteObject> token = CreateMockToken();
    sptr<IRemoteObject> callback = CreateMockToken();
    instance.RegisterCallback(token, callback);

    sptr<IRemoteObject> token1 = CreateMockToken();
    EXPECT_NO_FATAL_FAILURE(instance.OnInterrupted(token1, 0, ""));
    EXPECT_NO_FATAL_FAILURE(instance.OnInterrupted(token, 0, ""));
}

/**
 * @tc.name: OnEnd
 * @tc.desc: Test CallbackManager::OnEnd.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentCallbackManagerTest, OnEnd, TestSize.Level1)
{
    auto &instance = CallbackManager::GetInstance();
    sptr<IRemoteObject> token = CreateMockToken();
    sptr<IRemoteObject> callback = CreateMockToken();
    instance.RegisterCallback(token, callback);

    sptr<IRemoteObject> token1 = CreateMockToken();
    EXPECT_NO_FATAL_FAILURE(instance.OnEnd(token1));
    EXPECT_NO_FATAL_FAILURE(instance.OnEnd(token));
}

} // TEST
} // AAFwk
} // OHOS