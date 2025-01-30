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

#include "PowerStats.h"
#include <android-base/thread_annotations.h>
#include <memory>
#include <utils/Errors.h> // status_t

namespace android::media::psh_utils {

// Internal providers that fill up the PowerStats state object.
class PowerStatsProvider {
public:
    virtual ~PowerStatsProvider() = default;
    virtual status_t fill(PowerStats* stat) const = 0;
};

class PowerStatsCollector {
public:
    // singleton getter
    static PowerStatsCollector& getCollector();

    // Returns a snapshot of the state.
    // If toleranceNs > 0, we permit the use of a stale snapshot taken within that tolerance.
    std::shared_ptr<const PowerStats> getStats(int64_t toleranceNs = 0)
            EXCLUDES(mMutex, mMutexExclusiveFill);

private:
    PowerStatsCollector();  // use the singleton getter

    // Returns non-empty PowerStats if we have a previous stats snapshot within toleranceNs.
    std::shared_ptr<const PowerStats> checkLastStats(int64_t toleranceNs) const EXCLUDES(mMutex);
    int fill(PowerStats* stats) const;
    void addProvider(std::unique_ptr<PowerStatsProvider>&& powerStatsProvider);

    mutable std::mutex mMutexExclusiveFill;
    mutable std::mutex mMutex;
    // addProvider is called in the ctor, so effectively const.
    std::vector<std::unique_ptr<PowerStatsProvider>> mPowerStatsProviders;
    int64_t mLastFetchNs GUARDED_BY(mMutex) = 0;
    std::shared_ptr<const PowerStats> mLastFetchStats GUARDED_BY(mMutex);
};

} // namespace android::media::psh_utils
