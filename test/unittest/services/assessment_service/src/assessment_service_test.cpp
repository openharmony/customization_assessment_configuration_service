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

#include "assessment_service_test.h"

#include <cstdint>
#include <gtest/gtest.h>

#include "common_event_support.h"

#define private public
#include "assessment_service.h"
#include "assessment_service_ability.h"
#include "callback_manager.h"
#undef private

#include "mock_assessment_constants.h"
#include "mock_i_remote_object.h"

using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace AAFwk {
namespace TEST {

void AssessmentServiceTest::SetUp()
{
    // Reset singleton instance for testing
    AssessmentService::instance_ = nullptr;
}

sptr<IRemoteObject> CreateMockToken()
{
    return new MockIRemoteObject();
}

/**
 * @tc.name: AssessmentServiceGetInstance
 * @tc.desc: Test AssessmentService::GetInstance returns non-null singleton.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, AssessmentServiceGetInstance, TestSize.Level1)
{
    auto instance1 = AssessmentService::GetInstance();
    EXPECT_NE(instance1, nullptr);

    auto instance2 = AssessmentService::GetInstance();
    EXPECT_EQ(instance1, instance2);
}

/**
 * @tc.name: ConfigCurrentSession
 * @tc.desc: Test AssessmentService::CleanupCurrentSession
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, ConfigCurrentSession, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> token = CreateMockToken();
    sptr<IRemoteObject> callback = CreateMockToken();
    uint32_t duration = 0;
    std::vector<std::string> allowedApps = {"test"};
    EXPECT_NO_FATAL_FAILURE(service.ConfigCurrentSession(token, duration, allowedApps, callback));

    duration = 1;
    EXPECT_NO_FATAL_FAILURE(service.ConfigCurrentSession(token, duration, allowedApps, callback));
}

/**
 * @tc.name: CleanupCurrentSession
 * @tc.desc: Test AssessmentService::CleanupCurrentSession
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, CleanupCurrentSession, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.CleanupCurrentSession());
}

/**
 * @tc.name: ComputeNextTaskTimeoutLockedUnsafe01
 * @tc.desc: Test AssessmentService::ComputeNextTaskTimeoutLockedUnsafe
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, ComputeNextTaskTimeoutLockedUnsafe01, TestSize.Level1)
{
    int32_t defaultValue = 5000;
    AssessmentService service;
    auto ret = service.ComputeNextTaskTimeoutLockedUnsafe();
    EXPECT_EQ(ret, defaultValue);

    service.isActive_ = true;
    ret = service.ComputeNextTaskTimeoutLockedUnsafe();
    EXPECT_EQ(ret, defaultValue);

    sptr<IRemoteObject> token = CreateMockToken();
    service.callerToken_ = token;
    service.endpointCheckPoint_ = 0;
    ret = service.ComputeNextTaskTimeoutLockedUnsafe();
    EXPECT_EQ(ret, 0);

    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    service.endpointCheckPoint_ = now + 1000;

    ret = service.ComputeNextTaskTimeoutLockedUnsafe();
    EXPECT_GT(ret, 0);
    EXPECT_LT(ret, defaultValue);

    service.endpointCheckPoint_ = now + 10000;
    ret = service.ComputeNextTaskTimeoutLockedUnsafe();
    EXPECT_EQ(ret, defaultValue);
}

/**
 * @tc.name: SubscribeCommonEvent
 * @tc.desc: Test AssessmentService::SubscribeCommonEvent.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, SubscribeCommonEvent, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.SubscribeCommonEvent());
    EXPECT_NO_FATAL_FAILURE(service.UnsubscribeCommonEvent());
}

/**
 * @tc.name: AssessmentServiceDispatchEvent01
 * @tc.desc: Test AssessmentService::DispatchEvent.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, AssessmentServiceDispatchEvent01, TestSize.Level1)
{
    AssessmentService service;
    OHOS::AAFwk::Want want;
    want.SetAction("assessment.event.confirmation");
    OHOS::EventFwk::CommonEventData eventData(want);
    EXPECT_NO_FATAL_FAILURE(service.DispatchEvent(eventData));
}

/**
 * @tc.name: AssessmentServiceDispatchEvent02
 * @tc.desc: Test AssessmentService::DispatchEvent.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, AssessmentServiceDispatchEvent02, TestSize.Level1)
{
    AssessmentService service;
    OHOS::AAFwk::Want want;
    want.SetAction(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_ENTER_HIBERNATE);
    OHOS::EventFwk::CommonEventData eventData(want);
    EXPECT_NO_FATAL_FAILURE(service.DispatchEvent(eventData));
}

/**
 * @tc.name: AssessmentServiceDispatchEvent03
 * @tc.desc: Test AssessmentService::DispatchEvent.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, AssessmentServiceDispatchEvent03, TestSize.Level1)
{
    AssessmentService service;
    OHOS::AAFwk::Want want;
    want.SetAction(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_EXIT_HIBERNATE);
    OHOS::EventFwk::CommonEventData eventData(want);
    EXPECT_NO_FATAL_FAILURE(service.DispatchEvent(eventData));
}

/**
 * @tc.name: AssessmentServiceDispatchEvent04
 * @tc.desc: Test AssessmentService::DispatchEvent.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, AssessmentServiceDispatchEvent04, TestSize.Level1)
{
    AssessmentService service;
    OHOS::AAFwk::Want want;
    want.SetAction(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SHUTDOWN);
    OHOS::EventFwk::CommonEventData eventData(want);
    EXPECT_NO_FATAL_FAILURE(service.DispatchEvent(eventData));
}

/**
 * @tc.name: AssessmentServiceDispatchEvent05
 * @tc.desc: Test AssessmentService::DispatchEvent.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, AssessmentServiceDispatchEvent05, TestSize.Level1)
{
    AssessmentService service;
    OHOS::AAFwk::Want want;
    want.SetAction(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_POWER_SAVE_MODE_CHANGED);
    OHOS::EventFwk::CommonEventData eventData(want);
    EXPECT_NO_FATAL_FAILURE(service.DispatchEvent(eventData));
}

/**
 * @tc.name: AssessmentServiceDispatchEvent06
 * @tc.desc: Test AssessmentService::DispatchEvent.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, AssessmentServiceDispatchEvent06, TestSize.Level1)
{
    AssessmentService service;
    OHOS::AAFwk::Want want;
    want.SetAction(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_BOOT_COMPLETED);
    OHOS::EventFwk::CommonEventData eventData(want);
    EXPECT_NO_FATAL_FAILURE(service.DispatchEvent(eventData));
}

/**
 * @tc.name: NotifyBegin
 * @tc.desc: Test AssessmentService::NotifyBegin.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, NotifyBegin, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.NotifyBegin(0, ""));
    service.callerToken_ = CreateMockToken();
    EXPECT_NO_FATAL_FAILURE(service.NotifyBegin(0, ""));
}

/**
 * @tc.name: NotifyInterrupted
 * @tc.desc: Test AssessmentService::NotifyInterrupted.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, NotifyInterrupted, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.NotifyInterrupted(0, ""));
    service.callerToken_ = CreateMockToken();
    EXPECT_NO_FATAL_FAILURE(service.NotifyInterrupted(0, ""));
}

/**
 * @tc.name: NotifyEnd
 * @tc.desc: Test AssessmentService::NotifyEnd.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, NotifyEnd, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.NotifyEnd());
    service.callerToken_ = CreateMockToken();
    EXPECT_NO_FATAL_FAILURE(service.NotifyEnd());
}

/**
 * @tc.name: DoLoop
 * @tc.desc: Test AssessmentService::DoLoop.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, DoLoop, TestSize.Level1)
{
    auto service = AssessmentService::GetInstance();
    auto func = [&]() {
        service->DoLoop();
    };
    service->running_ = true;
    std::thread tx(func);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    do {
        std::unique_lock<std::mutex> lock(service->mutexSa_);
        EXPECT_NO_FATAL_FAILURE(service->Quit());
    } while (false);
    EXPECT_NO_FATAL_FAILURE(tx.join());
}

/**
 * @tc.name: AppDieHandle
 * @tc.desc: Test AssessmentService::AppDieHandle.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, AppDieHandle, TestSize.Level1)
{
    std::string bundleName = "com.test.demo";
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.AppDieHandle(bundleName));
    service.isActive_ = true;
    EXPECT_NO_FATAL_FAILURE(service.AppDieHandle(bundleName));
}

/**
 * @tc.name: HandleBegin
 * @tc.desc: Test AssessmentService::HandleBegin.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, HandleBegin, TestSize.Level1)
{
    AssessmentService service;
    std::string ticket = "123";
    int operation = 0;
    service.examStatus_ = AssessmentExamStatus::IDLE;
    EXPECT_NO_FATAL_FAILURE(service.HandleBegin(ticket, operation));
    
    service.examStatus_ = AssessmentExamStatus::CONFIRMING;
    EXPECT_NO_FATAL_FAILURE(service.HandleBegin(ticket, operation));

    service.ticket_ = ticket;
    EXPECT_NO_FATAL_FAILURE(service.HandleBegin(ticket, 2));

    EXPECT_NO_FATAL_FAILURE(service.HandleBegin(ticket, 0));
}

/**
 * @tc.name: IsActive
 * @tc.desc: Test AssessmentService::IsActive.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, IsActive, TestSize.Level1)
{
    AssessmentService service;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = false;
    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = false;
    int32_t errCode = 0;
    bool actived = false;
    service.IsActive(actived, errCode);
    EXPECT_EQ(actived, false);

    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = true;
    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = false;
    service.IsActive(actived, errCode);
    EXPECT_EQ(actived, false);

    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = true;
    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    service.IsActive(actived, errCode);
    EXPECT_EQ(actived, false);
    EXPECT_EQ(errCode, 0);

    service.isActive_ = true;
    service.IsActive(actived, errCode);
    EXPECT_EQ(actived, true);
    EXPECT_EQ(errCode, 0);
}

/**
 * @tc.name: GetConfiguration
 * @tc.desc: Test AssessmentService::GetConfiguration.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, GetConfiguration, TestSize.Level1)
{
    AssessmentService service;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = false;
    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = false;
    
    uint32_t duration = 0;
    std::vector<std::string> allowedApps;
    int32_t errCode = 0;
    service.GetConfiguration(duration, allowedApps, errCode);
    EXPECT_NE(errCode, 0);

    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = true;
    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = false;
    service.GetConfiguration(duration, allowedApps, errCode);
    EXPECT_NE(errCode, 0);

    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = true;
    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    service.GetConfiguration(duration, allowedApps, errCode);
    EXPECT_EQ(errCode, 0);
}

/**
 * @tc.name: LoadState
 * @tc.desc: Test AssessmentService::LoadState.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, LoadState, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.LoadState());
}

/**
 * @tc.name: SaveState
 * @tc.desc: Test AssessmentService::SaveState.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, SaveState, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.SaveState());
}

/**
 * @tc.name: ClearState
 * @tc.desc: Test AssessmentService::ClearState.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, ClearState, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.ClearState());
}

} // namespace TEST
} // namespace AAFwk
} // namespace OHOS