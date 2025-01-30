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

#include "HealthStats.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace android::media::psh_utils {

// See powerstats_util.proto and powerstats_util.pb.h

struct PowerStats {
    struct Metadata {
        // Represents the start time measured in milliseconds since boot of the
        // interval or point in time when stats were gathered.
        uint64_t start_time_since_boot_ms;

        // Represents the start time measured in milliseconds since epoch of the
        // interval or point in time when stats were gathered.
        uint64_t start_time_epoch_ms;

        // In monotonic clock.
        uint64_t start_time_monotonic_ms;

        // If PowerStats represent an interval, the duration field will be set will
        // the millisecond duration of stats collection. It will be unset for point
        // stats.
        // This is in boottime.
        uint64_t duration_ms;

        // This is in monotonic time, and does not include suspend.
        uint64_t duration_monotonic_ms;

        std::string toString() const;

        Metadata operator+=(const Metadata& other);
        Metadata operator-=(const Metadata& other);
        Metadata operator+(const Metadata& other) const;
        Metadata operator-(const Metadata& other) const;
        bool operator==(const Metadata& other) const = default;
    };

    struct StateResidency {
        std::string entity_name;
        std::string state_name;
        uint64_t time_ms;
        uint64_t entry_count;

        std::string toString() const;

        StateResidency operator+=(const StateResidency& other);
        StateResidency operator-=(const StateResidency& other);
        StateResidency operator+(const StateResidency& other) const;
        StateResidency operator-(const StateResidency& other) const;
        bool operator==(const StateResidency& other) const = default;
    };

    struct RailEnergy {
        std::string subsystem_name;
        std::string rail_name;
        uint64_t energy_uws;

        std::string toString() const;
        RailEnergy operator+=(const RailEnergy& other);
        RailEnergy operator-=(const RailEnergy& other);
        RailEnergy operator+(const RailEnergy& other) const;
        RailEnergy operator-(const RailEnergy& other) const;
        bool operator==(const RailEnergy& other) const = default;
    };

    HealthStats health_stats;

    std::string normalizedEnergy(const std::string& prefix = {}) const;

    // Returns {seconds, joules, watts} from all rails containing a matching string.
    std::tuple<float, float, float> energyFrom(const std::string& railMatcher) const;
    std::string toString(const std::string& prefix = {}) const;

    PowerStats operator+=(const PowerStats& other);
    PowerStats operator-=(const PowerStats& other);
    PowerStats operator+(const PowerStats& other) const;
    PowerStats operator-(const PowerStats& other) const;
    bool operator==(const PowerStats& other) const = default;

    Metadata metadata{};
    // These are sorted by name.
    std::vector<StateResidency> power_entity_state_residency;
    std::vector<RailEnergy> rail_energy;
};

} // namespace android::media::psh_utils
