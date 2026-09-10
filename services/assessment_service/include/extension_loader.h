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
 * - **Lazy loading** with configurable retry policy (InitExtensionLoader).
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
 *   loader->SetRetryPolicy({.maxRetries = 3, .retryIntervalMs = 1000});
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

  /**
   * @brief Load the shared library and resolve the "CheckAll" symbol.
   *
   * Retries according to the configured RetryPolicy if dlopen fails.
   * Safe to call multiple times; subsequent calls after a successful load
   * are no-ops.
   * @return true if the library is loaded and the "CheckAll" symbol is
   *         resolved; false if the loader is in a degraded state (load or
   *         symbol resolution failed), in which case InvokeCheckAll passes
   *         by default.
   */
  bool InitExtensionLoader();

  /**
   * @brief Invoke the closed-source CheckAll function.
   * @param allowedApps Bundle names to pass to the extension's check.
   * @return Result of CheckAll, or true (pass) if in degraded mode.
   */
  bool InvokeCheckAll(const std::vector<std::string> &allowedApps);

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

  /**
   * @brief Retry configuration for dlopen attempts. maxRetries is the maximum
   *        number of load attempts (1 = single attempt, no retry).
   */
  struct RetryPolicy {
    int32_t maxRetries = 1;
    int32_t retryIntervalMs = 1000;
  };
  void SetRetryPolicy(const RetryPolicy &policy);

private:
  bool LoadInternal();

  std::string soPath_;
  void *handle_ = nullptr;
  mutable std::mutex mutex_;
  DegradedCallback degradedCallback_;
  RetryPolicy retryPolicy_;
  CHECK_ALL_FUNC checkAllFunc_ = nullptr;
};

} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_AAFWK_ASSESSMENT_EXTENSION_LOADER_H
