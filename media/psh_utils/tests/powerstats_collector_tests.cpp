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

#include <psh_utils/PowerStatsCollector.h>
#include <gtest/gtest.h>
#include <utils/Log.h>

using namespace android::media::psh_utils;

template <typename T>
void inRange(const T& a, const T& b, const T& c) {
    ASSERT_GE(a, std::min(b, c));
    ASSERT_LE(a, std::max(b, c));
}

TEST(powerstat_collector_tests, basic) {
    auto& psc = PowerStatsCollector::getCollector();

    // This test is used for debugging the string through logcat, we validate a non-empty string.
    auto powerStats = psc.getStats();
    ALOGD("%s: %s", __func__, powerStats->toString().c_str());
    EXPECT_FALSE(powerStats->toString().empty());
}

TEST(powerstat_collector_tests, metadata) {
    PowerStats ps1, ps2;

    constexpr uint64_t kDurationMs1 = 5;
    constexpr uint64_t kDurationMs2 = 10;
    ps1.metadata.duration_ms = kDurationMs1;
    ps2.metadata.duration_ms = kDurationMs2;

    constexpr uint64_t kDurationMonotonicMs1 = 3;
    constexpr uint64_t kDurationMonotonicMs2 = 9;
    ps1.metadata.duration_monotonic_ms = kDurationMonotonicMs1;
    ps2.metadata.duration_monotonic_ms = kDurationMonotonicMs2;

    constexpr uint64_t kStartTimeSinceBootMs1 = 1616;
    constexpr uint64_t kStartTimeEpochMs1 = 1121;
    constexpr uint64_t kStartTimeMonotonicMs1 = 1525;
    constexpr uint64_t kStartTimeSinceBootMs2 = 2616;
    constexpr uint64_t kStartTimeEpochMs2 = 2121;
    constexpr uint64_t kStartTimeMonotonicMs2 = 2525;

    ps1.metadata.start_time_since_boot_ms = kStartTimeSinceBootMs1;
    ps1.metadata.start_time_epoch_ms = kStartTimeEpochMs1;
    ps1.metadata.start_time_monotonic_ms = kStartTimeMonotonicMs1;
    ps2.metadata.start_time_since_boot_ms = kStartTimeSinceBootMs2;
    ps2.metadata.start_time_epoch_ms = kStartTimeEpochMs2;
    ps2.metadata.start_time_monotonic_ms = kStartTimeMonotonicMs2;

    PowerStats ps3 = ps1 + ps2;
    PowerStats ps4 = ps2 + ps1;
    EXPECT_EQ(ps3, ps4);
    EXPECT_EQ(kDurationMs1 + kDurationMs2,
            ps3.metadata.duration_ms);
    EXPECT_EQ(kDurationMonotonicMs1 + kDurationMonotonicMs2,
            ps3.metadata.duration_monotonic_ms);

    EXPECT_NO_FATAL_FAILURE(inRange(ps3.metadata.start_time_since_boot_ms,
            kStartTimeSinceBootMs1, kStartTimeSinceBootMs2));
    EXPECT_NO_FATAL_FAILURE(inRange(ps3.metadata.start_time_epoch_ms,
            kStartTimeEpochMs1, kStartTimeEpochMs2));
    EXPECT_NO_FATAL_FAILURE(inRange(ps3.metadata.start_time_monotonic_ms,
            kStartTimeMonotonicMs1, kStartTimeMonotonicMs2));

    PowerStats ps5 = ps2 - ps1;
    EXPECT_EQ(kDurationMs2 - kDurationMs1,
            ps5.metadata.duration_ms);
    EXPECT_EQ(kDurationMonotonicMs2 - kDurationMonotonicMs1,
            ps5.metadata.duration_monotonic_ms);

    EXPECT_NO_FATAL_FAILURE(inRange(ps5.metadata.start_time_since_boot_ms,
            kStartTimeSinceBootMs1, kStartTimeSinceBootMs2));
    EXPECT_NO_FATAL_FAILURE(inRange(ps5.metadata.start_time_epoch_ms,
            kStartTimeEpochMs1, kStartTimeEpochMs2));
    EXPECT_NO_FATAL_FAILURE(inRange(ps5.metadata.start_time_monotonic_ms,
         kStartTimeMonotonicMs1, kStartTimeMonotonicMs2));
}

TEST(powerstat_collector_tests, state_residency) {
    PowerStats ps1, ps2;

    constexpr uint64_t kTimeMs1 = 5;
    constexpr uint64_t kTimeMs2 = 10;
    constexpr uint64_t kEntryCount1 = 15;
    constexpr uint64_t kEntryCount2 = 18;

    ps1.power_entity_state_residency.emplace_back(
            PowerStats::StateResidency{"", "", kTimeMs1, kEntryCount1});
    ps2.power_entity_state_residency.emplace_back(
            PowerStats::StateResidency{"", "", kTimeMs2, kEntryCount2});

    PowerStats ps3 = ps1 + ps2;
    PowerStats ps4 = ps2 + ps1;
    EXPECT_EQ(ps3, ps4);
    EXPECT_EQ(kTimeMs1 + kTimeMs2,
            ps3.power_entity_state_residency[0].time_ms);
    EXPECT_EQ(kEntryCount1 + kEntryCount2,
            ps3.power_entity_state_residency[0].entry_count);

    PowerStats ps5 = ps2 - ps1;
    EXPECT_EQ(kTimeMs2 - kTimeMs1,
            ps5.power_entity_state_residency[0].time_ms);
    EXPECT_EQ(kEntryCount2 - kEntryCount1,
            ps5.power_entity_state_residency[0].entry_count);
}

TEST(powerstat_collector_tests, rail_energy) {
    PowerStats ps1, ps2;

    constexpr uint64_t kEnergyUws1 = 5;
    constexpr uint64_t kEnergyUws2 = 10;

    ps1.rail_energy.emplace_back(
            PowerStats::RailEnergy{"", "", kEnergyUws1});
    ps2.rail_energy.emplace_back(
            PowerStats::RailEnergy{"", "", kEnergyUws2});

    PowerStats ps3 = ps1 + ps2;
    PowerStats ps4 = ps2 + ps1;
    EXPECT_EQ(ps3, ps4);
    EXPECT_EQ(kEnergyUws1 + kEnergyUws2,
            ps3.rail_energy[0].energy_uws);

    PowerStats ps5 = ps2 - ps1;
    EXPECT_EQ(kEnergyUws2 - kEnergyUws1,
            ps5.rail_energy[0].energy_uws);
}

TEST(powerstat_collector_tests, health_stats) {
    PowerStats ps1, ps2;

    constexpr double kBatteryChargeCounterUah1 = 21;
    constexpr double kBatteryChargeCounterUah2 = 25;
    ps1.health_stats.batteryChargeCounterUah = kBatteryChargeCounterUah1;
    ps2.health_stats.batteryChargeCounterUah = kBatteryChargeCounterUah2;

    constexpr double kBatteryFullChargeUah1 = 32;
    constexpr double kBatteryFullChargeUah2 = 33;
    ps1.health_stats.batteryFullChargeUah = kBatteryFullChargeUah1;
    ps2.health_stats.batteryFullChargeUah = kBatteryFullChargeUah2;

    constexpr double kBatteryVoltageMillivolts1 = 42;
    constexpr double kBatteryVoltageMillivolts2 = 43;
    ps1.health_stats.batteryVoltageMillivolts = kBatteryVoltageMillivolts1;
    ps2.health_stats.batteryVoltageMillivolts = kBatteryVoltageMillivolts2;

    PowerStats ps3 = ps1 + ps2;
    PowerStats ps4 = ps2 + ps1;
    EXPECT_EQ(ps3, ps4);
    EXPECT_EQ(kBatteryChargeCounterUah1 + kBatteryChargeCounterUah2,
              ps3.health_stats.batteryChargeCounterUah);

    EXPECT_NO_FATAL_FAILURE(inRange(ps3.health_stats.batteryFullChargeUah,
             kBatteryFullChargeUah1, kBatteryFullChargeUah2));
    EXPECT_NO_FATAL_FAILURE(inRange(ps3.health_stats.batteryVoltageMillivolts,
             kBatteryVoltageMillivolts1, kBatteryVoltageMillivolts2));

    PowerStats ps5 = ps2 - ps1;
    EXPECT_EQ(kBatteryChargeCounterUah2 - kBatteryChargeCounterUah1,
              ps5.health_stats.batteryChargeCounterUah);

    EXPECT_NO_FATAL_FAILURE(inRange(ps5.health_stats.batteryFullChargeUah,
            kBatteryFullChargeUah1, kBatteryFullChargeUah2));
    EXPECT_NO_FATAL_FAILURE(inRange(ps5.health_stats.batteryVoltageMillivolts,
            kBatteryVoltageMillivolts1, kBatteryVoltageMillivolts2));
}
