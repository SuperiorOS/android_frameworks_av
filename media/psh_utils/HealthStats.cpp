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
#include <psh_utils/HealthStats.h>

namespace android::media::psh_utils {

template <typename T>
const T& choose_voltage(const T& a, const T& b) {
   return std::max(a, b);  // we use max here, could use avg.
}

std::string HealthStats::toString() const {
    std::string result;
    const float batteryVoltage = batteryVoltageMillivolts * 1e-3f;  // Volts
    const float charge = batteryChargeCounterUah * (3600 * 1e-6);  // Joules = Amp-Second
    result.append("{Net Battery V: ")
            .append(std::to_string(batteryVoltage))
            .append(" J: ")
            .append(std::to_string(charge))
            .append("}");
    return result;
}

std::string HealthStats::normalizedEnergy(double timeSec) const {
    std::string result;
    const float batteryVoltage = batteryVoltageMillivolts * 1e-3f;   // Volts
    const float charge = -batteryChargeCounterUah * (3600 * 1e-6f);  // Joules = Amp-Second
    const float watts = charge * batteryVoltage / timeSec;
    result.append("{Net Battery V: ")
            .append(std::to_string(batteryVoltage))
            .append(" J: ")
            .append(std::to_string(charge))
            .append(" W: ")
            .append(std::to_string(watts))
            .append("}");
    return result;
}

HealthStats HealthStats::operator+=(const HealthStats& other) {
    batteryVoltageMillivolts = choose_voltage(
            batteryVoltageMillivolts, other.batteryVoltageMillivolts);
    batteryFullChargeUah = std::max(batteryFullChargeUah, other.batteryFullChargeUah);
    batteryChargeCounterUah += other.batteryChargeCounterUah;
    return *this;
}

HealthStats HealthStats::operator-=(const HealthStats& other) {
    batteryVoltageMillivolts = choose_voltage(
            batteryVoltageMillivolts, other.batteryVoltageMillivolts);
    batteryFullChargeUah = std::max(batteryFullChargeUah, other.batteryFullChargeUah);
    batteryChargeCounterUah -= other.batteryChargeCounterUah;
    return *this;
}

HealthStats HealthStats::operator+(const HealthStats& other) const {
    HealthStats result = *this;
    result += other;
    return result;
}

HealthStats HealthStats::operator-(const HealthStats& other) const {
    HealthStats result = *this;
    result -= other;
    return result;
}

} // namespace android::media::psh_utils
