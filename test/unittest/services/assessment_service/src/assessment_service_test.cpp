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

#include "assessment_api_error_code.h"
#include "common_event_support.h"
#include "syspara/parameters.h"

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
 * @tc.name: CleanupCurrentSession02
 * @tc.desc: Test AssessmentService::CleanupCurrentSession
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, CleanupCurrentSession02, TestSize.Level1)
{
    AssessmentService service;
    service.callerToken_ = CreateMockToken();
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
    service.isActive_ = false;
    service.callerToken_ = nullptr;
    auto ret = service.ComputeNextTaskTimeoutLockedUnsafe();
    EXPECT_EQ(ret, defaultValue);

    service.isActive_ = false;
    service.callerToken_ = CreateMockToken();
    ret = service.ComputeNextTaskTimeoutLockedUnsafe();
    EXPECT_EQ(ret, defaultValue);

    service.isActive_ = true;
    service.callerToken_ = nullptr;
    ret = service.ComputeNextTaskTimeoutLockedUnsafe();
    EXPECT_EQ(ret, defaultValue);

    service.isActive_ = true;
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
 * @tc.name: UnsubscribeCommonEvent
 * @tc.desc: Test AssessmentService::UnsubscribeCommonEvent.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, UnsubscribeCommonEvent, TestSize.Level1)
{
    AssessmentService service;
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
 * @tc.name: AssessmentServiceDispatchEvent07
 * @tc.desc: Test AssessmentService::DispatchEvent.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, AssessmentServiceDispatchEvent07, TestSize.Level1)
{
    AssessmentService service;
    OHOS::AAFwk::Want want;
    want.SetAction("assessment.event.confirmation");
    OHOS::EventFwk::CommonEventData eventData(want);
    eventData.SetData("ticket:0");
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
 * @tc.name: Quit
 * @tc.desc: Test AssessmentService::Quit.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, Quit, TestSize.Level1)
{
    auto service = AssessmentService::GetInstance();
    do {
        std::unique_lock<std::mutex> lock(service->mutexSa_);
        EXPECT_NO_FATAL_FAILURE(service->Quit());
    } while (false);
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
 * @tc.name: AppDieHandle02
 * @tc.desc: Test AssessmentService::AppDieHandle.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, AppDieHandle02, TestSize.Level1)
{
    std::string bundleName = "com.test.demo";
    AssessmentService service;
    service.isActive_ = true;
    service.bundleName_ = bundleName;
    service.callerToken_ = CreateMockToken();
    EXPECT_NO_FATAL_FAILURE(service.AppDieHandle(bundleName));
}

/**
 * @tc.name: AppDieHandle03
 * @tc.desc: Test AssessmentService::AppDieHandle.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, AppDieHandle03, TestSize.Level1)
{
    std::string bundleName = "com.test.demo";
    AssessmentService service;
    service.isActive_ = true;
    service.bundleName_ = bundleName;
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
    service.examStatus_ = AssessmentExamStatus::CONFIRMING;
    EXPECT_NO_FATAL_FAILURE(service.HandleBegin(ticket, 2));

    service.ticket_ = ticket;
    service.examStatus_ = AssessmentExamStatus::CONFIRMING;
    EXPECT_NO_FATAL_FAILURE(service.HandleBegin(ticket, 0));

    service.ticket_ = ticket;
    service.examStatus_ = AssessmentExamStatus::CONFIRMING;
    EXPECT_NO_FATAL_FAILURE(service.HandleBegin(ticket, 1));
}

/**
 * @tc.name: BeginInvalidParams
 * @tc.desc: Test AssessmentService::Begin.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, BeginInvalidParams, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> callerToken = nullptr;
    uint32_t duration = 0;
    std::vector<std::string> allowedApps;
    sptr<IRemoteObject> callback = nullptr;
    int32_t errCode = 0;

    service.Begin(callerToken, duration, allowedApps, callback, errCode);
    EXPECT_EQ(errCode, static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_PARAMS));

    callerToken = CreateMockToken();
    service.Begin(callerToken, duration, allowedApps, callback, errCode);
    EXPECT_EQ(errCode, static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_PARAMS));

    callback = CreateMockToken();
    service.Begin(callerToken, duration, allowedApps, callback, errCode);
    EXPECT_EQ(errCode, static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_PARAMS));

    allowedApps.push_back("com.exam.demo");
    service.Begin(callerToken, duration, allowedApps, callback, errCode);
    EXPECT_NE(errCode, static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_PARAMS));
}

/**
 * @tc.name: BeginInvalidDeviceType
 * @tc.desc: Test AssessmentService::Begin.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, BeginInvalidDeviceType, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> callerToken = CreateMockToken();
    uint32_t duration = 0;
    std::vector<std::string> allowedApps = {"com.exam.demo"};
    sptr<IRemoteObject> callback = CreateMockToken();
    int32_t errCode = 0;

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = false;
    service.Begin(callerToken, duration, allowedApps, callback, errCode);
    EXPECT_EQ(errCode, static_cast<int32_t>(AssessmentApiErrCode::ERR_CAPABILITY_NOT_SUPPORT));
}

/**
 * @tc.name: BeginInvalidPermission
 * @tc.desc: Test AssessmentService::Begin.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, BeginInvalidPermission, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> callerToken = CreateMockToken();
    uint32_t duration = 0;
    std::vector<std::string> allowedApps = {"com.exam.demo"};
    sptr<IRemoteObject> callback = CreateMockToken();
    int32_t errCode = 0;

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = false;
    service.Begin(callerToken, duration, allowedApps, callback, errCode);
    EXPECT_EQ(errCode, static_cast<int32_t>(AssessmentApiErrCode::ERR_PERMISSION_DENIED));
}

/**
 * @tc.name: BeginAlreadyActived
 * @tc.desc: Test AssessmentService::Begin.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, BeginAlreadyActived, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> callerToken = CreateMockToken();
    uint32_t duration = 0;
    std::vector<std::string> allowedApps = {"com.exam.demo"};
    sptr<IRemoteObject> callback = CreateMockToken();
    int32_t errCode = 0;

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = true;

    service.isActive_ = true;
    service.Begin(callerToken, duration, allowedApps, callback, errCode);
    EXPECT_EQ(errCode, static_cast<int32_t>(AssessmentApiErrCode::ERR_ASSESSMENT_ALREADY_ACTIVE));
}

/**
 * @tc.name: BeginConfirm
 * @tc.desc: Test AssessmentService::Begin.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, BeginConfirm, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> callerToken = CreateMockToken();
    uint32_t duration = 0;
    std::vector<std::string> allowedApps = {"com.exam.demo"};
    sptr<IRemoteObject> callback = CreateMockToken();
    int32_t errCode = 0;

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = true;

    service.isActive_ = false;
    service.examStatus_ = AssessmentExamStatus::CONFIRMING;
    EXPECT_NO_FATAL_FAILURE(service.Begin(callerToken, duration, allowedApps, callback, errCode));
}

/**
 * @tc.name: BeginConfirm02
 * @tc.desc: Test AssessmentService::Begin.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, BeginConfirm02, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> callerToken = CreateMockToken();
    uint32_t duration = 0;
    std::vector<std::string> allowedApps = {"com.exam.demo"};
    sptr<IRemoteObject> callback = CreateMockToken();
    int32_t errCode = 0;

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = true;

    service.isActive_ = false;
    service.examStatus_ = AssessmentExamStatus::CONFIRMING;
    service.callerToken_ = CreateMockToken();
    EXPECT_NO_FATAL_FAILURE(service.Begin(callerToken, duration, allowedApps, callback, errCode));
}

/**
 * @tc.name: BeginConfirm03
 * @tc.desc: Test AssessmentService::Begin.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, BeginConfirm03, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> callerToken = CreateMockToken();
    uint32_t duration = 0;
    std::vector<std::string> allowedApps = {"com.exam.demo"};
    sptr<IRemoteObject> callback = CreateMockToken();
    int32_t errCode = 0;

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = true;

    service.isActive_ = false;
    service.examStatus_ = AssessmentExamStatus::IDLE;
    service.callerToken_ = CreateMockToken();
    EXPECT_NO_FATAL_FAILURE(service.Begin(callerToken, duration, allowedApps, callback, errCode));
}

/**
 * @tc.name: EndInvalidParams
 * @tc.desc: Test AssessmentService::End.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, EndInvalidParams, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> callerToken = nullptr;
    int32_t errCode = 0;

    service.End(callerToken, errCode);
    EXPECT_EQ(errCode, static_cast<int32_t>(AssessmentApiErrCode::ERR_INVALID_PARAMS));
}

/**
 * @tc.name: EndInvalidDeviceType
 * @tc.desc: Test AssessmentService::End.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, EndInvalidDeviceType, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> callerToken = CreateMockToken();
    int32_t errCode = 0;

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = false;
    service.End(callerToken, errCode);
    EXPECT_EQ(errCode, static_cast<int32_t>(AssessmentApiErrCode::ERR_CAPABILITY_NOT_SUPPORT));
}

/**
 * @tc.name: EndInvalidPermission
 * @tc.desc: Test AssessmentService::End.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, EndInvalidPermission, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> callerToken = CreateMockToken();
    int32_t errCode = 0;

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = false;
    service.End(callerToken, errCode);
    EXPECT_EQ(errCode, static_cast<int32_t>(AssessmentApiErrCode::ERR_PERMISSION_DENIED));
}

/**
 * @tc.name: EndInvalidState
 * @tc.desc: Test AssessmentService::End.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, EndInvalidState, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> callerToken = CreateMockToken();
    int32_t errCode = 0;

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = true;
    service.isActive_ = false;
    service.End(callerToken, errCode);
    EXPECT_EQ(errCode, static_cast<int32_t>(AssessmentApiErrCode::ERR_ASSESSMENT_NOT_ACTIVE));
}

/**
 * @tc.name: EndIntervalError
 * @tc.desc: Test AssessmentService::End.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, EndIntervalError, TestSize.Level1)
{
    AssessmentService service;
    sptr<IRemoteObject> callerToken = CreateMockToken();
    int32_t errCode = 0;

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = true;
    service.isActive_ = true;
    service.End(callerToken, errCode);
    EXPECT_EQ(errCode, static_cast<int32_t>(AssessmentApiErrCode::ERR_INTERNAL_ERROR));
}

/**
 * @tc.name: IsActive
 * @tc.desc: Test AssessmentService::IsActive.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, IsActive, TestSize.Level1)
{
    AssessmentService service;
    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = false;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = false;
    int32_t errCode = 0;
    bool actived = false;
    service.IsActive(actived, errCode);
    EXPECT_EQ(actived, false);

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = false;
    service.IsActive(actived, errCode);
    EXPECT_EQ(actived, false);

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = true;
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
    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = false;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = false;
    
    uint32_t duration = 0;
    std::vector<std::string> allowedApps;
    int32_t errCode = 0;
    service.GetConfiguration(duration, allowedApps, errCode);
    EXPECT_NE(errCode, 0);

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = false;
    service.GetConfiguration(duration, allowedApps, errCode);
    EXPECT_NE(errCode, 0);

    AssessmentTestConstants::GetInstance().CheckDeviceTypeSupported = true;
    AssessmentTestConstants::GetInstance().VerifyCallingPermissionReturn = true;
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
 * @tc.name: LoadState02
 * @tc.desc: Test AssessmentService::LoadState.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, LoadState02, TestSize.Level1)
{
    AssessmentService service;
    OHOS::system::SetParameter("persist.assessment.is_active", "true");
    OHOS::system::SetParameter("persist.assessment.duration", "2000");
    OHOS::system::SetParameter("persist.assessment.allowed_apps", "com.demo1|com.demo2");
    EXPECT_NO_FATAL_FAILURE(service.LoadState());
}

/**
 * @tc.name: LoadState03
 * @tc.desc: Test AssessmentService::LoadState.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, LoadState03, TestSize.Level1)
{
    AssessmentService service;
    OHOS::system::SetParameter("persist.assessment.is_active", "false");
    OHOS::system::SetParameter("persist.assessment.duration", "");
    OHOS::system::SetParameter("persist.assessment.allowed_apps", "|");
    EXPECT_NO_FATAL_FAILURE(service.LoadState());
}

/**
 * @tc.name: LoadState04
 * @tc.desc: Test AssessmentService::LoadState.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, LoadState04, TestSize.Level1)
{
    AssessmentService service;
    OHOS::system::SetParameter("persist.assessment.is_active", "");
    OHOS::system::SetParameter("persist.assessment.duration", "2000");
    OHOS::system::SetParameter("persist.assessment.allowed_apps", "com.demo1");
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
 * @tc.name: SaveState02
 * @tc.desc: Test AssessmentService::SaveState.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, SaveState02, TestSize.Level1)
{
    AssessmentService service;
    service.currentConfig_.allowedApps.push_back("com.demo1");
    service.currentConfig_.allowedApps.push_back("com.demo2");
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

/**
 * @tc.name: Init
 * @tc.desc: Test AssessmentService::Init.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, Init, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.Init());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    service.running_ = false;
    service.thread_.join();
}

/**
 * @tc.name: InitSubsystems
 * @tc.desc: Test AssessmentService::InitSubsystems.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, InitSubsystems, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.InitSubsystems());
}

/**
 * @tc.name: TimeoutLockedUnsafe
 * @tc.desc: Test AssessmentService::TimeoutLockedUnsafe.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, TimeoutLockedUnsafe, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.TimeoutLockedUnsafe());
}

/**
 * @tc.name: ConfirmationBeginLockedUnsafe
 * @tc.desc: Test AssessmentService::ConfirmationBeginLockedUnsafe.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, ConfirmationBeginLockedUnsafe, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.ConfirmationBeginLockedUnsafe());
}

/**
 * @tc.name: InvokeSystemDialog
 * @tc.desc: Test AssessmentService::InvokeSystemDialog.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, InvokeSystemDialog, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.InvokeSystemDialog());
}

/**
 * @tc.name: CheckEndpointAndExecuteTaskLockedUnsafe
 * @tc.desc: Test AssessmentService::CheckEndpointAndExecuteTaskLockedUnsafe.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, CheckEndpointAndExecuteTaskLockedUnsafe, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.CheckEndpointAndExecuteTaskLockedUnsafe());
}

/**
 * @tc.name: CheckEndpointAndExecuteTaskLockedUnsafe02
 * @tc.desc: Test AssessmentService::CheckEndpointAndExecuteTaskLockedUnsafe.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, CheckEndpointAndExecuteTaskLockedUnsafe02, TestSize.Level1)
{
    AssessmentService service;
    service.isActive_ = true;
    EXPECT_NO_FATAL_FAILURE(service.CheckEndpointAndExecuteTaskLockedUnsafe());
}

/**
 * @tc.name: CheckEndpointAndExecuteTaskLockedUnsafe03
 * @tc.desc: Test AssessmentService::CheckEndpointAndExecuteTaskLockedUnsafe.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, CheckEndpointAndExecuteTaskLockedUnsafe03, TestSize.Level1)
{
    AssessmentService service;
    service.isActive_ = true;
    service.callerToken_ = CreateMockToken();
    EXPECT_NO_FATAL_FAILURE(service.CheckEndpointAndExecuteTaskLockedUnsafe());
}

/**
 * @tc.name: CheckEndpointAndExecuteTaskLockedUnsafe04
 * @tc.desc: Test AssessmentService::CheckEndpointAndExecuteTaskLockedUnsafe.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, CheckEndpointAndExecuteTaskLockedUnsafe04, TestSize.Level1)
{
    AssessmentService service;
    service.isActive_ = true;
    service.callerToken_ = CreateMockToken();
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    service.endpointCheckPoint_ = now + 1000;
    EXPECT_NO_FATAL_FAILURE(service.CheckEndpointAndExecuteTaskLockedUnsafe());
}

/**
 * @tc.name: Destroy
 * @tc.desc: Test AssessmentService::Destroy.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, Destroy, TestSize.Level1)
{
    AssessmentService service;
    EXPECT_NO_FATAL_FAILURE(service.Destroy());
}

/**
 * @tc.name: CancelBeginLockedUnsafe
 * @tc.desc: Test AssessmentService::CancelBeginLockedUnsafe.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentServiceTest, CancelBeginLockedUnsafe, TestSize.Level1)
{
    AssessmentService service;
    service.callerToken_ = CreateMockToken();
    EXPECT_NO_FATAL_FAILURE(service.CancelBeginLockedUnsafe());
}
} // namespace TEST
} // namespace AAFwk
} // namespace OHOS