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
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "extension_loader.h"

using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace AAFwk {
namespace TEST {
namespace {
// A library name that cannot be resolved on any device, so the loader is
// guaranteed to enter degraded mode whether or not the closed-source extension
// is shipped. Using the production so name here would make every expectation
// below depend on the device under test.
constexpr const char *UNRESOLVABLE_SO = "libassessment_unittest_absent_ext.z.so";
constexpr const char *PRESET_REASON = "PRESET_FAIL_REASON";
// Mirrors RESTRICT_ANCO_APP_FAILED in assessment_service.cpp: the caller only
// treats 1 as a failure, so the degraded sentinel must never equal it.
constexpr int32_t RESTRICT_ANCO_APP_FAILED = 1;
constexpr int32_t RESTRICT_ANCO_APP_DEGRADED = -1;
constexpr int DEGRADED_SYMBOL_COUNT = 5;
constexpr int TIMEOUT_MS = 5000;
constexpr int POLL_INTERVAL_MS = 20;

// Runs fn on a detached worker thread and waits up to TIMEOUT_MS for it to
// finish. A degraded callback that re-enters the loader while it still holds
// mutex_ would otherwise hang the whole test binary instead of failing, which
// is far harder to diagnose in CI than an assertion. The loader is passed as a
// shared_ptr and captured by the thread so that it outlives a stuck worker.
bool RunWithTimeout(const std::shared_ptr<ExtensionLoader> &loader,
    const std::function<void(ExtensionLoader &)> &fn)
{
    auto done = std::make_shared<std::atomic<bool>>(false);
    std::thread([loader, fn, done]() {
        fn(*loader);
        done->store(true);
    }).detach();
    for (int waited = 0; waited < TIMEOUT_MS; waited += POLL_INTERVAL_MS) {
        if (done->load()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
    }
    return done->load();
}
} // namespace

class AssessmentExtensionLoaderTest : public testing::Test {
public:
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

protected:
    void SetUp() override {}

    void TearDown() override {}

    static std::shared_ptr<ExtensionLoader> MakeDegradedLoader()
    {
        auto loader = std::make_shared<ExtensionLoader>(UNRESOLVABLE_SO);
        loader->InitExtensionLoader();
        return loader;
    }
};

/**
 * @tc.name: IsDegradeBeforeInit
 * @tc.desc: Test ExtensionLoader::IsDegrade before InitExtensionLoader is called.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentExtensionLoaderTest, IsDegradeBeforeInit, TestSize.Level1)
{
    ExtensionLoader loader(UNRESOLVABLE_SO);
    EXPECT_TRUE(loader.IsDegrade());
}

/**
 * @tc.name: InitWithUnresolvableSoEntersDegradedMode
 * @tc.desc: Test ExtensionLoader::InitExtensionLoader when dlopen cannot resolve the library.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentExtensionLoaderTest, InitWithUnresolvableSoEntersDegradedMode, TestSize.Level1)
{
    ExtensionLoader loader(UNRESOLVABLE_SO);
    EXPECT_FALSE(loader.InitExtensionLoader());
    EXPECT_TRUE(loader.IsDegrade());
}

/**
 * @tc.name: InitIsRepeatable
 * @tc.desc: Test that ExtensionLoader::InitExtensionLoader is safe to call more than once.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentExtensionLoaderTest, InitIsRepeatable, TestSize.Level1)
{
    ExtensionLoader loader(UNRESOLVABLE_SO);
    EXPECT_FALSE(loader.InitExtensionLoader());
    EXPECT_FALSE(loader.InitExtensionLoader());
    EXPECT_TRUE(loader.IsDegrade());
}

/**
 * @tc.name: InvokeCheckAllDegradePassesAndKeepsFailReason
 * @tc.desc: Test ExtensionLoader::InvokeCheckAll degrades to pass and leaves failReason untouched.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentExtensionLoaderTest, InvokeCheckAllDegradePassesAndKeepsFailReason, TestSize.Level1)
{
    auto loader = MakeDegradedLoader();
    std::string failReason = PRESET_REASON;
    std::vector<std::string> allowedApps {"com.example.assessment"};
    EXPECT_TRUE(loader->InvokeCheckAll(allowedApps, failReason));
    EXPECT_EQ(failReason, PRESET_REASON);
}

/**
 * @tc.name: InvokeActivateAllDegradePassesAndKeepsFailReason
 * @tc.desc: Test ExtensionLoader::InvokeActivateAll degrades to pass and leaves failReason untouched.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentExtensionLoaderTest, InvokeActivateAllDegradePassesAndKeepsFailReason, TestSize.Level1)
{
    auto loader = MakeDegradedLoader();
    std::string failReason = PRESET_REASON;
    std::vector<std::string> allowedApps {"com.example.assessment"};
    EXPECT_TRUE(loader->InvokeActivateAll(allowedApps, failReason));
    EXPECT_EQ(failReason, PRESET_REASON);
}

/**
 * @tc.name: InvokeDeactivateAllDegradePassesAndKeepsFailReason
 * @tc.desc: Test ExtensionLoader::InvokeDeactivateAll degrades to pass and leaves failReason untouched.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentExtensionLoaderTest, InvokeDeactivateAllDegradePassesAndKeepsFailReason, TestSize.Level1)
{
    auto loader = MakeDegradedLoader();
    std::string failReason = PRESET_REASON;
    EXPECT_TRUE(loader->InvokeDeactivateAll(failReason));
    EXPECT_EQ(failReason, PRESET_REASON);
}

/**
 * @tc.name: InvokeIsAwakeAncoDegradeReturnsFalse
 * @tc.desc: Test ExtensionLoader::InvokeIsAwakeAnco degrades to false so the service does not wait for anco.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentExtensionLoaderTest, InvokeIsAwakeAncoDegradeReturnsFalse, TestSize.Level1)
{
    auto loader = MakeDegradedLoader();
    EXPECT_FALSE(loader->InvokeIsAwakeAnco("2"));
}

/**
 * @tc.name: InvokeRestrictAncoAppDegradeIsNotTreatedAsFailure
 * @tc.desc: Test ExtensionLoader::InvokeRestrictAncoApp degrades to -1, which the caller must not read as a failure.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentExtensionLoaderTest, InvokeRestrictAncoAppDegradeIsNotTreatedAsFailure, TestSize.Level1)
{
    auto loader = MakeDegradedLoader();
    int32_t retCode = loader->InvokeRestrictAncoApp();
    EXPECT_EQ(retCode, RESTRICT_ANCO_APP_DEGRADED);
    EXPECT_NE(retCode, RESTRICT_ANCO_APP_FAILED);
}

/**
 * @tc.name: DegradedCallbackReceivesSoPathAndEverySymbol
 * @tc.desc: Test that each degraded Invoke reports the so path and its own symbol name exactly once.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentExtensionLoaderTest, DegradedCallbackReceivesSoPathAndEverySymbol, TestSize.Level1)
{
    auto loader = MakeDegradedLoader();
    std::string gotSoPath;
    std::vector<std::string> gotSymbols;
    loader->SetDegradedCallback([&gotSoPath, &gotSymbols](const std::string &soPath, const std::string &symbol) {
        gotSoPath = soPath;
        gotSymbols.push_back(symbol);
    });
    std::string failReason;
    std::vector<std::string> allowedApps;
    loader->InvokeCheckAll(allowedApps, failReason);
    loader->InvokeIsAwakeAnco("2");
    loader->InvokeRestrictAncoApp();
    loader->InvokeActivateAll(allowedApps, failReason);
    loader->InvokeDeactivateAll(failReason);
    EXPECT_EQ(gotSoPath, UNRESOLVABLE_SO);
    ASSERT_EQ(gotSymbols.size(), static_cast<size_t>(DEGRADED_SYMBOL_COUNT));
    EXPECT_EQ(gotSymbols[0], "CheckAll");
    EXPECT_EQ(gotSymbols[1], "IsAwakeAnco");
    EXPECT_EQ(gotSymbols[2], "RestrictAncoApp");
    EXPECT_EQ(gotSymbols[3], "ActivateAll");
    EXPECT_EQ(gotSymbols[4], "DeactivateAll");
}

/**
 * @tc.name: ClearedDegradedCallbackIsNotInvoked
 * @tc.desc: Test ExtensionLoader::SetDegradedCallback with a null callback stops the notification.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentExtensionLoaderTest, ClearedDegradedCallbackIsNotInvoked, TestSize.Level1)
{
    auto loader = MakeDegradedLoader();
    int callCount = 0;
    loader->SetDegradedCallback([&callCount](const std::string &soPath, const std::string &symbol) {
        (void)soPath;
        (void)symbol;
        ++callCount;
    });
    std::string failReason;
    std::vector<std::string> allowedApps;
    loader->InvokeCheckAll(allowedApps, failReason);
    EXPECT_EQ(callCount, 1);
    loader->SetDegradedCallback(nullptr);
    loader->InvokeCheckAll(allowedApps, failReason);
    EXPECT_EQ(callCount, 1);
}

/**
 * @tc.name: ReentrantDegradedCallbackDoesNotDeadlock
 * @tc.desc: Test that a degraded callback may call back into the loader without self-deadlocking.
 * @tc.type: FUNC
 */
HWTEST_F(AssessmentExtensionLoaderTest, ReentrantDegradedCallbackDoesNotDeadlock, TestSize.Level1)
{
    auto loader = MakeDegradedLoader();
    auto reentryCount = std::make_shared<std::atomic<int>>(0);
    loader->SetDegradedCallback([loader, reentryCount](const std::string &soPath, const std::string &symbol) {
        (void)soPath;
        (void)symbol;
        // Re-enters a mutex-guarded method of the same loader. std::mutex is not
        // recursive, so this deadlocks unless every Invoke releases mutex_
        // before calling the degraded callback.
        if (loader->IsDegrade()) {
            reentryCount->fetch_add(1);
        }
    });
    bool finished = RunWithTimeout(loader, [](ExtensionLoader &target) {
        std::string failReason;
        std::vector<std::string> allowedApps;
        target.InvokeCheckAll(allowedApps, failReason);
        target.InvokeIsAwakeAnco("2");
        target.InvokeRestrictAncoApp();
        target.InvokeActivateAll(allowedApps, failReason);
        target.InvokeDeactivateAll(failReason);
    });
    EXPECT_TRUE(finished);
    EXPECT_EQ(reentryCount->load(), DEGRADED_SYMBOL_COUNT);
}
} // TEST
} // AAFwk
} // OHOS
