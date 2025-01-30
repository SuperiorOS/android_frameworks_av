/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android-base/thread_annotations.h>
#include <mutex>
#include <utils/Log.h>
#include <utils/Timers.h>

namespace android::media::psh_utils {

struct DefaultServiceTraits {
    static constexpr int64_t kThresholdRetryNs = 1'000'000'000;
    static constexpr int64_t kMaxRetries = 5;
    static constexpr const char* kServiceVersion = "/default";
    static constexpr bool kShowLog = true;
};

template<typename Service, typename ServiceTraits = DefaultServiceTraits>
std::shared_ptr<Service> getServiceSingleton() {
    [[clang::no_destroy]] static constinit std::mutex m;
    [[clang::no_destroy]] static constinit std::shared_ptr<Service> service GUARDED_BY(m);
    static int64_t nextTryNs GUARDED_BY(m) = 0;
    static int64_t tries GUARDED_BY(m) = 0;

    std::lock_guard l(m);
    if (service
            || tries > ServiceTraits::kMaxRetries  // try too many times
            || systemTime(SYSTEM_TIME_BOOTTIME) < nextTryNs) {  // try too frequently.
        return service;
    }

    const auto serviceName = std::string(Service::descriptor)
            .append(ServiceTraits::kServiceVersion);
    service = Service::fromBinder(
            ::ndk::SpAIBinder(AServiceManager_checkService(serviceName.c_str())));

    if (!service) {
        // If failed, set a time limit before retry.
        // No need to log an error, it is already done.
        nextTryNs = systemTime(SYSTEM_TIME_BOOTTIME) + ServiceTraits::kThresholdRetryNs;
        ALOGV_IF(ServiceTraits::kShowLog, "service:%s  retries:%lld of %lld  nextTryNs:%lld",
                Service::descriptor, (long long)tries,
                (long long)kMaxRetries, (long long)nextTryNs);
        ++tries;
    }

    return service;
}

} // namespace android::media::psh_utils
