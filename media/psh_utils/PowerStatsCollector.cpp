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
#include <android-base/logging.h>
#include <psh_utils/PowerStatsCollector.h>
#include "PowerStatsProvider.h"
#include <utils/Timers.h>

namespace android::media::psh_utils {

PowerStatsCollector::PowerStatsCollector() {
    addProvider(std::make_unique<PowerEntityResidencyDataProvider>());
    addProvider(std::make_unique<RailEnergyDataProvider>());
    addProvider(std::make_unique<HealthStatsDataProvider>());
}

/* static */
PowerStatsCollector& PowerStatsCollector::getCollector() {
    [[clang::no_destroy]] static PowerStatsCollector psc;
    return psc;
}

std::shared_ptr<const PowerStats> PowerStatsCollector::getStats(int64_t toleranceNs) {
    // Check if there is a cached PowerStats result available.
    // As toleranceNs may be different between callers, it may be that some callers
    // are blocked on mMutexExclusiveFill for a new stats result, while other callers
    // may find the current cached result acceptable (within toleranceNs).
    if (toleranceNs > 0) {
        auto result = checkLastStats(toleranceNs);
        if (result) return result;
    }

    // Take the mMutexExclusiveFill to ensure only one thread is filling.
    std::lock_guard lg1(mMutexExclusiveFill);
    // As obtaining a new PowerStats snapshot might take some time,
    // check again to see if another waiting thread filled the cached result for us.
    if (toleranceNs > 0) {
        auto result = checkLastStats(toleranceNs);
        if (result) return result;
    }
    auto result = std::make_shared<PowerStats>();
    (void)fill(result.get());
    std::lock_guard lg2(mMutex);
    mLastFetchNs = systemTime(SYSTEM_TIME_BOOTTIME);
    mLastFetchStats = result;
    return result;
}

std::shared_ptr<const PowerStats> PowerStatsCollector::checkLastStats(int64_t toleranceNs) const {
    if (toleranceNs > 0) {
        // see if we can return an old result.
        std::lock_guard lg(mMutex);
        if (mLastFetchStats && systemTime(SYSTEM_TIME_BOOTTIME) - mLastFetchNs < toleranceNs) {
            return mLastFetchStats;
        }
    }
    return {};
}

void PowerStatsCollector::addProvider(std::unique_ptr<PowerStatsProvider>&& powerStatsProvider) {
    mPowerStatsProviders.emplace_back(std::move(powerStatsProvider));
}

int PowerStatsCollector::fill(PowerStats* stats) const {
    if (!stats) {
        LOG(ERROR) << __func__ << ": bad args; stat is null";
        return 1;
    }

    for (const auto& provider : mPowerStatsProviders) {
        (void) provider->fill(stats); // on error, we continue to proceed.
    }

    // boot time follows wall clock time, but starts from boot.
    stats->metadata.start_time_since_boot_ms = systemTime(SYSTEM_TIME_BOOTTIME) / 1'000'000;

    // wall clock time
    stats->metadata.start_time_epoch_ms = systemTime(SYSTEM_TIME_REALTIME) / 1'000'000;

    // monotonic time follows boot time, but does not include any time suspended.
    stats->metadata.start_time_monotonic_ms = systemTime() / 1'000'000;
    return 0;
}

} // namespace android::media::psh_utils
