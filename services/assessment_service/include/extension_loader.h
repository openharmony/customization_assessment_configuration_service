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

#ifndef OHOS_AAFWK_ASSESSMENT_EXTENSION_LOADER_H
#define OHOS_AAFWK_ASSESSMENT_EXTENSION_LOADER_H

#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace OHOS {
namespace AAFwk {

/**
 * @class ExtensionLoader
 * @brief Dynamic library loader for closed-source assessment extensions.
 *
 * ExtensionLoader encapsulates the lifecycle of a shared library (.so) that
 * provides environment-checking capabilities not available in the open-source
 * tree (e.g. virtual-machine detection). It handles:
 *
 * - **Lazy loading**: InitExtensionLoader dlopens the .so once at service
 *   startup. If the load fails, the loader enters degraded mode for the
 *   entire SA lifetime (no retry).
 * - **Symbol resolution** for the "CheckAll" entry point via dlsym.
 * - **Graceful degradation**: if the .so or symbol cannot be resolved, the
 *   loader enters a degraded state where InvokeCheckAll returns true (pass
 *   by default) instead of crashing, and a DegradedCallback is invoked to
 *   notify the caller.
 * - **Thread safety**: all public methods are guarded by an internal mutex.
 * - **RAII cleanup**: the destructor calls dlclose on the loaded handle.
 *
 * Typical usage:
 * @code
 *   auto loader = std::make_unique<ExtensionLoader>("libassessment_ext.z.so");
 *   loader->SetDegradedCallback([](auto &so, auto &sym) { LOGW("%s:%s degraded", so, sym); });
 *   loader->InitExtensionLoader();
 *   bool safe = loader->InvokeCheckAll(allowedApps);
 * @endcode
 *
 * @note This class is non-copyable and non-movable.
 */
class ExtensionLoader {
public:
    explicit ExtensionLoader(const std::string &soPath);
    ~ExtensionLoader();

    ExtensionLoader(const ExtensionLoader &) = delete;
    ExtensionLoader &operator=(const ExtensionLoader &) = delete;
    ExtensionLoader(ExtensionLoader &&) = delete;
    ExtensionLoader &operator=(ExtensionLoader &&) = delete;

    typedef bool (*CHECK_ALL_FUNC)(const std::vector<std::string> &allowedApps);

    typedef bool (*IS_AWAKE_ANCO_FUNC)(std::string ancoState);

    typedef void (*RESTRICT_ANCO_APP_FUNC)();

    /**
     * @brief Load the shared library and resolve symbols.
     *
     * Performs a single dlopen attempt. If it fails, the loader enters
     * degraded mode for the lifetime of the SA (no retry).
     * Safe to call multiple times; subsequent calls after a successful load
     * are no-ops.
     * @return true if the library is loaded and all symbols are resolved;
     *         false if the loader is in a degraded state, in which case
     *         Invoke* methods degrade gracefully.
     */
    bool InitExtensionLoader();

    /**
     * @brief Invoke the closed-source CheckAll function.
     * @param allowedApps Bundle names to pass to the extension's check.
     * @return Result of CheckAll, or true (pass) if in degraded mode.
     */
    bool InvokeCheckAll(const std::vector<std::string> &allowedApps);

    /**
     * @brief Invoke the closed-source IsAwakeAnco function.
     * @param ancoState check the anco state.
     * @return Result of IsAwakeAnco, or true (pass) if anco freeze.
     */
    bool InvokeIsAwakeAnco(std::string ancoState);

    /**
     * @brief Invoke the closed-source RestrictAncoApp function.
     */
    void InvokeRestrictAncoApp();

    /**
     * @brief Check whether the loader is in degraded mode.
     * @return true if the CheckAll symbol could not be resolved.
     */
    bool IsDegrade() const;

    /**
     * @brief Callback type invoked when a symbol fails to resolve.
     * @param soPath  Path of the shared library.
     * @param symbol  Name of the unresolved symbol.
     */
    using DegradedCallback = std::function<void(const std::string &soPath, const std::string &symbol)>;
    void SetDegradedCallback(DegradedCallback callback);

private:
    bool LoadInternal();

    std::string soPath_;
    void *handle_ = nullptr;
    mutable std::mutex mutex_;
    DegradedCallback degradedCallback_;
    CHECK_ALL_FUNC checkAllFunc_ = nullptr;
    IS_AWAKE_ANCO_FUNC isAwakeAncoFunc_ = nullptr;
    RESTRICT_ANCO_APP_FUNC restrictAncoAppFunc = nullptr;
};

} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_AAFWK_ASSESSMENT_EXTENSION_LOADER_H
