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

#include <EngineConfig.h>

#include <android/media/audio/common/AudioHalEngineConfig.h>
#include <system/audio_policy.h>
#include <string>
#include <vector>

namespace android {
namespace capEngineConfig {

static const char *const gCriterionTypeSuffix = "Type";
static const char *const gInputDeviceCriterionName = "AvailableInputDevices";
static const char *const gOutputDeviceCriterionName = "AvailableOutputDevices";
static const char *const gPhoneStateCriterionName = "TelephonyMode";
static const char *const gOutputDeviceAddressCriterionName = "AvailableOutputDevicesAddresses";
static const char *const gInputDeviceAddressCriterionName = "AvailableInputDevicesAddresses";

/**
* Order MUST be aligned with definition of audio_policy_force_use_t within audio_policy.h
*/
static const char *const gForceUseCriterionTag[AUDIO_POLICY_FORCE_USE_CNT] =
{
    [AUDIO_POLICY_FORCE_FOR_COMMUNICATION] =        "ForceUseForCommunication",
    [AUDIO_POLICY_FORCE_FOR_MEDIA] =                "ForceUseForMedia",
    [AUDIO_POLICY_FORCE_FOR_RECORD] =               "ForceUseForRecord",
    [AUDIO_POLICY_FORCE_FOR_DOCK] =                 "ForceUseForDock",
    [AUDIO_POLICY_FORCE_FOR_SYSTEM] =               "ForceUseForSystem",
    [AUDIO_POLICY_FORCE_FOR_HDMI_SYSTEM_AUDIO] =    "ForceUseForHdmiSystemAudio",
    [AUDIO_POLICY_FORCE_FOR_ENCODED_SURROUND] =     "ForceUseForEncodedSurround",
    [AUDIO_POLICY_FORCE_FOR_VIBRATE_RINGING] =      "ForceUseForVibrateRinging"
};

using ParameterValues = std::vector<std::string>;

struct ConfigurableElement {
    std::string path;
};

struct ConfigurableElementValue {
    ConfigurableElement configurableElement;
    std::string value;
};
using ConfigurableElementValues = std::vector<ConfigurableElementValue>;

struct CapSetting {
    std::string configurationName;
    ConfigurableElementValues configurableElementValues;
};
using CapSettings = std::vector<CapSetting>;

struct CapConfiguration {
    std::string name;
    std::string rule;
};

using ConfigurableElementPaths = std::vector<std::string>;
using CapConfigurations = std::vector<CapConfiguration>;

struct CapConfigurableDomain {
    std::string name;
    CapConfigurations configurations;
    CapSettings settings;
};

struct CapCriterion {
    engineConfig::Criterion criterion;
    engineConfig::CriterionType criterionType;
};

using CapCriteria = std::vector<CapCriterion>;
using CapConfigurableDomains = std::vector<CapConfigurableDomain>;

struct CapConfig {
    CapCriteria capCriteria;
    CapConfigurableDomains capConfigurableDomains;
};

/** Result of `parse(const char*)` */
struct ParsingResult {
    /** Parsed config, nullptr if the xml lib could not load the file */
    std::unique_ptr<CapConfig> parsedConfig;
    size_t nbSkippedElement; //< Number of skipped invalid product strategies
};

/** Convert the provided Cap Settings configuration.
 * @return audio policy usage @see Config
 */
ParsingResult convert(const ::android::media::audio::common::AudioHalEngineConfig& aidlConfig);

}
}
