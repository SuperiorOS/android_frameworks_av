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
#include <audio_utils/clock.h>
#include <psh_utils/PowerStats.h>

namespace android::media::psh_utils {

// Determine the best start time from a and b, which is
// min(a, b) if both exist, otherwise the one that exists.
template <typename T>
const T& choose_best_start_time(const T& a, const T& b) {
    if (a) {
        return b ? std::min(a, b) : a;
    } else {
        return b;
    }
}

// subtract two time differences.
template <typename T, typename U>
const T sub_time_diff(const T& diff_a, const T& diff_b, const U& abs_c, const U& abs_d) {
    if (diff_a) {
        return diff_b ? (diff_a - diff_b) : diff_a;
    } else if (diff_b) {
        return diff_b;
    } else {  // no difference exists, use absolute time.
        return abs_c - abs_d;
    }
}

std::string PowerStats::Metadata::toString() const {
    return std::string("start_time_since_boot_ms: ").append(
                    std::to_string(start_time_since_boot_ms))
            .append(" start_time_monotonic_ms: ").append(std::to_string(start_time_monotonic_ms))
            .append(audio_utils_time_string_from_ns(start_time_epoch_ms * 1'000'000).time)
            .append(" duration_ms: ").append(std::to_string(duration_ms))
            .append(" duration_monotonic_ms: ").append(std::to_string(duration_monotonic_ms));
}

PowerStats::Metadata PowerStats::Metadata::operator+=(const Metadata& other) {
    start_time_since_boot_ms = choose_best_start_time(
            start_time_since_boot_ms, other.start_time_since_boot_ms);
    start_time_epoch_ms = choose_best_start_time(
            start_time_epoch_ms, other.start_time_epoch_ms);
    start_time_monotonic_ms = choose_best_start_time(
            start_time_monotonic_ms, other.start_time_monotonic_ms);
    duration_ms += other.duration_ms;
    duration_monotonic_ms += other.duration_monotonic_ms;
    return *this;
}

PowerStats::Metadata PowerStats::Metadata::operator-=(const Metadata& other) {
    // here we calculate duration, if it makes sense.
    duration_ms = sub_time_diff(duration_ms, other.duration_ms,
                                start_time_since_boot_ms, other.start_time_since_boot_ms);
    duration_monotonic_ms = sub_time_diff(
            duration_monotonic_ms, other.duration_monotonic_ms,
            start_time_monotonic_ms, other.start_time_monotonic_ms);
    start_time_since_boot_ms = choose_best_start_time(
            start_time_since_boot_ms, other.start_time_since_boot_ms);
    start_time_epoch_ms = choose_best_start_time(
            start_time_epoch_ms, other.start_time_epoch_ms);
    start_time_monotonic_ms = choose_best_start_time(
            start_time_monotonic_ms, other.start_time_monotonic_ms);
    return *this;
}

PowerStats::Metadata PowerStats::Metadata::operator+(const Metadata& other) const {
    Metadata result = *this;
    result += other;
    return result;
}

PowerStats::Metadata PowerStats::Metadata::operator-(const Metadata& other) const {
    Metadata result = *this;
    result -= other;
    return result;
}

std::string PowerStats::StateResidency::toString() const {
    return std::string(entity_name).append(state_name)
            .append(" ").append(std::to_string(time_ms))
            .append(" ").append(std::to_string(entry_count));
}

PowerStats::StateResidency PowerStats::StateResidency::operator+=(const StateResidency& other) {
    if (entity_name.empty()) entity_name = other.entity_name;
    if (state_name.empty()) state_name = other.state_name;
    time_ms += other.time_ms;
    entry_count += other.entry_count;
    return *this;
}

PowerStats::StateResidency PowerStats::StateResidency::operator-=(const StateResidency& other) {
    if (entity_name.empty()) entity_name = other.entity_name;
    if (state_name.empty()) state_name = other.state_name;
    time_ms -= other.time_ms;
    entry_count -= other.entry_count;
    return *this;
}

PowerStats::StateResidency PowerStats::StateResidency::operator+(
        const StateResidency& other) const {
    StateResidency result = *this;
    result += other;
    return result;
}

PowerStats::StateResidency PowerStats::StateResidency::operator-(
        const StateResidency& other) const {
    StateResidency result = *this;
    result -= other;
    return result;
}

std::string PowerStats::RailEnergy::toString() const {
    return std::string(subsystem_name)
            .append(rail_name)
            .append(" ")
            .append(std::to_string(energy_uws));
}

PowerStats::RailEnergy PowerStats::RailEnergy::operator+=(const RailEnergy& other) {
    if (subsystem_name.empty()) subsystem_name = other.subsystem_name;
    if (rail_name.empty()) rail_name = other.rail_name;
    energy_uws += other.energy_uws;
    return *this;
}

PowerStats::RailEnergy PowerStats::RailEnergy::operator-=(const RailEnergy& other) {
    if (subsystem_name.empty()) subsystem_name = other.subsystem_name;
    if (rail_name.empty()) rail_name = other.rail_name;
    energy_uws -= other.energy_uws;
    return *this;
}

PowerStats::RailEnergy PowerStats::RailEnergy::operator+(const RailEnergy& other) const {
    RailEnergy result = *this;
    result += other;
    return result;
}

PowerStats::RailEnergy PowerStats::RailEnergy::operator-(const RailEnergy& other) const {
    RailEnergy result = *this;
    result -= other;
    return result;
}

std::string PowerStats::toString(const std::string& prefix) const {
    std::string result;
    result.append(prefix).append(metadata.toString()).append("\n");
    result.append(prefix).append(health_stats.toString()).append("\n");
    for (const auto &residency: power_entity_state_residency) {
        result.append(prefix).append(residency.toString()).append("\n");
    }
    for (const auto &energy: rail_energy) {
        result.append(prefix).append(energy.toString()).append("\n");
    }
    return result;
}

std::string PowerStats::normalizedEnergy(const std::string& prefix) const {
    if (metadata.duration_ms == 0) return {};

    std::string result(prefix);
    result.append(audio_utils_time_string_from_ns(
            metadata.start_time_epoch_ms * 1'000'000).time);
    result.append(" duration_boottime: ")
            .append(std::to_string(metadata.duration_ms * 1e-3f))
            .append(" duration_monotonic: ")
            .append(std::to_string(metadata.duration_monotonic_ms * 1e-3f))
            .append("\n");
    if (health_stats.isValid()) {
        result.append(prefix)
                .append(health_stats.normalizedEnergy(metadata.duration_ms * 1e-3f)).append("\n");
    }

    // energy_uws is converted to ave W using recip time in us.
    const float recipTime = 1e-3 / metadata.duration_ms;
    int64_t total_energy = 0;
    for (const auto& energy: rail_energy) {
        total_energy += energy.energy_uws;
        result.append(prefix).append(energy.subsystem_name)
                .append(energy.rail_name)
                .append(" ")
                .append(std::to_string(energy.energy_uws * 1e-6))
                .append(" ")
                .append(std::to_string(energy.energy_uws * recipTime))
                .append("\n");
    }
    if (total_energy != 0) {
        result.append(prefix).append("total J and ave W: ")
                .append(std::to_string(total_energy * 1e-6))
                .append(" ")
                .append(std::to_string(total_energy * recipTime))
                .append("\n");
    }
    return result;
}

// seconds, joules, watts
std::tuple<float, float, float> PowerStats::energyFrom(const std::string& railMatcher) const {
    if (metadata.duration_ms == 0) return {};

    // energy_uws is converted to ave W using recip time in us.
    const float recipTime = 1e-3 / metadata.duration_ms;
    int64_t total_energy = 0;
    for (const auto& energy: rail_energy) {
        if (energy.subsystem_name.find(railMatcher) != std::string::npos
                || energy.rail_name.find(railMatcher) != std::string::npos) {
            total_energy += energy.energy_uws;
        }
    }
    return {metadata.duration_ms * 1e-3, total_energy * 1e-6, total_energy * recipTime};
}

PowerStats PowerStats::operator+=(const PowerStats& other) {
    metadata += other.metadata;
    health_stats += other.health_stats;
    if (power_entity_state_residency.empty()) {
        power_entity_state_residency = other.power_entity_state_residency;
    } else {
        for (size_t i = 0; i < power_entity_state_residency.size(); ++i) {
            power_entity_state_residency[i] += other.power_entity_state_residency[i];
        }
    }
    if (rail_energy.empty()) {
        rail_energy = other.rail_energy;
    } else {
        for (size_t i = 0; i < rail_energy.size(); ++i) {
            rail_energy[i] += other.rail_energy[i];
        }
    }
    return *this;
}

PowerStats PowerStats::operator-=(const PowerStats& other) {
    metadata -= other.metadata;
    health_stats -= other.health_stats;
    if (power_entity_state_residency.empty()) {
        power_entity_state_residency = other.power_entity_state_residency;
    } else {
        for (size_t i = 0; i < power_entity_state_residency.size(); ++i) {
            power_entity_state_residency[i] -= other.power_entity_state_residency[i];
        }
    }
    if (rail_energy.empty()) {
        rail_energy = other.rail_energy;
    } else {
        for (size_t i = 0; i < rail_energy.size(); ++i) {
            rail_energy[i] -= other.rail_energy[i];
        }
    }
    return *this;
}

PowerStats PowerStats::operator+(const PowerStats& other) const {
    PowerStats result = *this;
    result += other;
    return result;
}

PowerStats PowerStats::operator-(const PowerStats& other) const {
    PowerStats result = *this;
    result -= other;
    return result;
}

} // namespace android::media::psh_utils
