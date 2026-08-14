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

#ifndef OHOS_ASSESSMENT_HILOG_TAG_WRAPPER_H
#define OHOS_ASSESSMENT_HILOG_TAG_WRAPPER_H

#include "hilog/log.h"

namespace OHOS {
namespace AAFwk {
enum class AAFwkLogTag : uint32_t {
    DEFAULT = 0xD003950,
};
} // namespace AAFwk
} // namespace OHOS

using AAFwkTag = OHOS::AAFwk::AAFwkLogTag;

constexpr uint32_t ASSESSMENT_LOG_DOMAIN = 0xD003950;
constexpr const char* ASSESSMENT_LOG_TAG = "AssessmentService";

#define ASSESSMENT_PRINT_LOG(level, tag, fmt, ...) \
    do { \
        AAFwkTag logTag = tag; \
        ((void)HILOG_IMPL(LOG_CORE, level, static_cast<uint32_t>(logTag), \
            ASSESSMENT_LOG_TAG, "[%{public}s:%{public}d]" fmt, \
            __builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__, \
            __LINE__, ##__VA_ARGS__)); \
    } while (0)

#define TAG_LOGD(tag, fmt, ...) ASSESSMENT_PRINT_LOG(LOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define TAG_LOGI(tag, fmt, ...) ASSESSMENT_PRINT_LOG(LOG_INFO, tag, fmt, ##__VA_ARGS__)
#define TAG_LOGW(tag, fmt, ...) ASSESSMENT_PRINT_LOG(LOG_WARN, tag, fmt, ##__VA_ARGS__)
#define TAG_LOGE(tag, fmt, ...) ASSESSMENT_PRINT_LOG(LOG_ERROR, tag, fmt, ##__VA_ARGS__)
#define TAG_LOGF(tag, fmt, ...) ASSESSMENT_PRINT_LOG(LOG_FATAL, tag, fmt, ##__VA_ARGS__)

#endif // OHOS_ASSESSMENT_HILOG_TAG_WRAPPER_H
