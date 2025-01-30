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

#include <string>

namespace android::media::psh_utils {

// From hardware/interfaces/health/aidl/android/hardware/health/HealthInfo.aidl

struct HealthStats {
    /**
     * Instantaneous battery voltage in millivolts (mV).
     *
     * Historically, the unit of this field is microvolts (µV), but all
     * clients and implementations uses millivolts in practice, making it
     * the de-facto standard.
     */
    double batteryVoltageMillivolts;
    /**
     * Battery charge value when it is considered to be "full" in µA-h
     */
    double batteryFullChargeUah;
    /**
     * Instantaneous battery capacity in µA-h
     */
    double batteryChargeCounterUah;

    std::string normalizedEnergy(double time) const;

    bool isValid() const { return batteryVoltageMillivolts > 0; }

    // Returns {seconds, joules, watts} from battery counters
    std::tuple<float, float, float> energyFrom(const std::string& s) const;
    std::string toString() const;

    HealthStats operator+=(const HealthStats& other);
    HealthStats operator-=(const HealthStats& other);
    HealthStats operator+(const HealthStats& other) const;
    HealthStats operator-(const HealthStats& other) const;
    bool operator==(const HealthStats& other) const = default;
};

} // namespace android::media::psh_utils
