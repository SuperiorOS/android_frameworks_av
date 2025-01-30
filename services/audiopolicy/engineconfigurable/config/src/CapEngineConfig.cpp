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

#include <cinttypes>
#include <cstdint>
#include <istream>
#include <map>
#include <sstream>
#include <stdarg.h>
#include <string>
#include <string>
#include <vector>

#define LOG_TAG "APM::AudioPolicyEngine/CapConfig"
//#define LOG_NDEBUG 0

#include "CapEngineConfig.h"
#include <TypeConverter.h>
#include <Volume.h>
#include <cutils/properties.h>
#include <media/AidlConversion.h>
#include <media/AidlConversionCppNdk.h>
#include <media/AidlConversionUtil.h>
#include <media/TypeConverter.h>
#include <media/convert.h>
#include <system/audio_config.h>
#include <utils/Log.h>

namespace android {

using utilities::convertTo;
using media::audio::common::AudioDeviceAddress;
using media::audio::common::AudioDeviceDescription;
using media::audio::common::AudioHalCapCriterion;
using media::audio::common::AudioHalCapParameter;
using media::audio::common::AudioHalCapRule;
using media::audio::common::AudioSource;
using media::audio::common::AudioStreamType;
using media::audio::common::AudioHalCapCriterionV2;
using ::android::base::unexpected;

namespace capEngineConfig {

static constexpr const char *gLegacyOutputDevicePrefix = "AUDIO_DEVICE_OUT_";
static constexpr const char *gLegacyInputDevicePrefix = "AUDIO_DEVICE_IN_";
static constexpr const char *gLegacyStreamPrefix = "AUDIO_STREAM_";
static constexpr const char *gLegacySourcePrefix = "AUDIO_SOURCE_";
static constexpr const char *gPolicyParamPrefix = "/Policy/policy/";

namespace {

ConversionResult<std::string> truncatePrefixToLower(const std::string& legacyName,
                                                    const std::string& legacyPrefix) {
    std::size_t pos = legacyName.find(legacyPrefix);
    if (pos == std::string::npos) {
        return unexpected(BAD_VALUE);
    }
    std::string capName = legacyName.substr(pos + legacyPrefix.length());
    std::transform(capName.begin(), capName.end(), capName.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return capName;
}

ConversionResult<std::string> truncatePrefix(const std::string& name,  const std::string& prefix) {
    std::size_t pos = name.find(prefix);
    if (pos == std::string::npos) {
        return unexpected(BAD_VALUE);
    }
    std::string capName = name.substr(pos + prefix.length());
    return capName;
}

ConversionResult<std::string> aidl2legacy_AudioHalCapCriterionV2_CapName(
        const AudioHalCapCriterionV2& aidl) {
    switch (aidl.getTag()) {
        case AudioHalCapCriterionV2::availableInputDevices:
            return gInputDeviceCriterionName;
        case AudioHalCapCriterionV2::availableOutputDevices:
            return gOutputDeviceCriterionName;
        case AudioHalCapCriterionV2::availableInputDevicesAddresses:
            return gInputDeviceAddressCriterionName;
        case AudioHalCapCriterionV2::availableOutputDevicesAddresses:
            return gOutputDeviceAddressCriterionName;
        case AudioHalCapCriterionV2::telephonyMode:
            return gPhoneStateCriterionName;
        case AudioHalCapCriterionV2::forceConfigForUse: {
            auto aidlCriterion = aidl.get<AudioHalCapCriterionV2::forceConfigForUse>();
            return gForceUseCriterionTag[VALUE_OR_RETURN(
                    aidl2legacy_AudioPolicyForceUse_audio_policy_force_use_t(
                            aidlCriterion.forceUse))];
        }
        default:
            return unexpected(BAD_VALUE);
    }
}

ConversionResult<std::string> aidl2legacy_AudioHalCapCriterionV2TypeDevice_CapCriterionValue(
        const AudioDeviceDescription& aidl) {
    audio_devices_t legacyDeviceType = VALUE_OR_RETURN(
            aidl2legacy_AudioDeviceDescription_audio_devices_t(aidl));
    bool isOut = audio_is_output_devices(legacyDeviceType);
    std::string legacyTypeLiteral;
    if (!::android::DeviceConverter::toString(legacyDeviceType, legacyTypeLiteral)) {
        ALOGE("%s Invalid strategy device type %d", __func__, legacyDeviceType);
        return unexpected(BAD_VALUE);
    }
    return truncatePrefix(legacyTypeLiteral,
            isOut ? gLegacyOutputDevicePrefix : gLegacyInputDevicePrefix);
}

ConversionResult<std::string> aidl2legacy_AudioHalCapCriterionV2Type_CapCriterionValue(
        const AudioHalCapCriterionV2::Type& aidl) {
    switch (aidl.getTag()) {
        case AudioHalCapCriterionV2::Type::availableDevicesType:
            return aidl2legacy_AudioHalCapCriterionV2TypeDevice_CapCriterionValue(
                    aidl.get<AudioHalCapCriterionV2::Type::availableDevicesType>());
        case AudioHalCapCriterionV2::Type::availableDevicesAddressesType:
            return aidl.get<AudioHalCapCriterionV2::Type::availableDevicesAddressesType>().template
                    get<AudioDeviceAddress::id>();
        case AudioHalCapCriterionV2::Type::telephonyModeType:
            return toString(aidl.get<AudioHalCapCriterionV2::Type::telephonyModeType>());
        case AudioHalCapCriterionV2::Type::forcedConfigType:
            return toString(aidl.get<AudioHalCapCriterionV2::Type::forcedConfigType>());
        default:
            return unexpected(BAD_VALUE);
    }
}

ConversionResult<std::string> aidl2legacy_AudioHalCapRule_CapRule(
        const AudioHalCapRule& aidlRule) {
    std::string rule;
    switch (aidlRule.compoundRule) {
        case AudioHalCapRule::CompoundRule::ANY:
            rule += "Any";
            break;
        case AudioHalCapRule::CompoundRule::ALL:
            rule += "All";
            break;
        default:
            return unexpected(BAD_VALUE);
    }
    rule += "{";
    if (!aidlRule.nestedRules.empty()) {
        for (const auto& nestedRule: aidlRule.nestedRules) {
            rule += VALUE_OR_FATAL(aidl2legacy_AudioHalCapRule_CapRule(nestedRule));
        }
        if (!aidlRule.criterionRules.empty()) {
            rule += ",";
        }
    }
    bool isFirstCriterionRule = true;
    for (const auto& criterionRule: aidlRule.criterionRules) {
        if (!isFirstCriterionRule) {
            rule += ",";
        }
        isFirstCriterionRule = false;
        std::string selectionCriterion = VALUE_OR_RETURN(
                aidl2legacy_AudioHalCapCriterionV2_CapName(criterionRule.criterion));
        std::string matchesWhen;
        std::string value = VALUE_OR_RETURN(
                aidl2legacy_AudioHalCapCriterionV2Type_CapCriterionValue(
                        criterionRule.criterionTypeValue));

        switch (criterionRule.matchingRule) {
            case AudioHalCapRule::MatchingRule::IS:
                matchesWhen = "Is";
                break;
            case AudioHalCapRule::MatchingRule::IS_NOT:
                matchesWhen = "IsNot";
                break;
            case AudioHalCapRule::MatchingRule::INCLUDES:
                matchesWhen = "Includes";
                break;
            case AudioHalCapRule::MatchingRule::EXCLUDES:
                matchesWhen = "Excludes";
                break;
            default:
                return unexpected(BAD_VALUE);
        }
        rule += selectionCriterion + " " + matchesWhen + " " + value;
    }
    rule += "}";
    return rule;
}

ConversionResult<CapConfiguration> aidl2legacy_AudioHalCapConfiguration_CapConfiguration(
        const media::audio::common::AudioHalCapConfiguration& aidl) {
    CapConfiguration legacy;
    legacy.name = aidl.name;
    legacy.rule = VALUE_OR_FATAL(aidl2legacy_AudioHalCapRule_CapRule(aidl.rule));
    return legacy;
}

ConversionResult<ConfigurableElementValue> aidl2legacy_ParameterSetting_ConfigurableElementValue(
        const AudioHalCapParameter& aidl) {
    ConfigurableElementValue legacy;
    std::string literalValue;
    switch (aidl.getTag()) {
        case AudioHalCapParameter::selectedStrategyDevice: {
            auto strategyDevice = aidl.get<AudioHalCapParameter::selectedStrategyDevice>();
            literalValue = std::to_string(strategyDevice.isSelected);
            audio_devices_t legacyType = VALUE_OR_RETURN(
                    aidl2legacy_AudioDeviceDescription_audio_devices_t(strategyDevice.device));
            std::string legacyTypeLiteral;
            if (!::android::OutputDeviceConverter::toString(legacyType, legacyTypeLiteral)) {
                ALOGE("%s Invalid device type %d", __func__, legacyType);
                return unexpected(BAD_VALUE);
            }
            std::string deviceLiteral = VALUE_OR_RETURN(
                    truncatePrefixToLower(legacyTypeLiteral, gLegacyOutputDevicePrefix));
            if (deviceLiteral == "default") {
                deviceLiteral = "stub";
            }
            legacy.configurableElement.path = std::string(gPolicyParamPrefix)
                    + "product_strategies/vx_" + std::to_string(strategyDevice.id)
                    + "/selected_output_devices/mask/" + deviceLiteral;
            break;
        }
        case AudioHalCapParameter::strategyDeviceAddress: {
            auto strategyAddress = aidl.get<AudioHalCapParameter::strategyDeviceAddress>();
            legacy.configurableElement.path = std::string(gPolicyParamPrefix)
                    + "product_strategies/vx_" + std::to_string(strategyAddress.id)
                    + "/device_address";
            literalValue = strategyAddress.deviceAddress.get<AudioDeviceAddress::id>();
            break;
        }
        case AudioHalCapParameter::selectedInputSourceDevice: {
            auto inputSourceDevice = aidl.get<AudioHalCapParameter::selectedInputSourceDevice>();
            literalValue = std::to_string(inputSourceDevice.isSelected);
            audio_devices_t legacyType = VALUE_OR_RETURN(
                    aidl2legacy_AudioDeviceDescription_audio_devices_t(inputSourceDevice.device));
            std::string legacyTypeLiteral;
            if (!::android::InputDeviceConverter::toString(legacyType, legacyTypeLiteral)) {
                ALOGE("%s Invalid input source device type %d", __func__, legacyType);
                return unexpected(BAD_VALUE);
            }
            std::string deviceLiteral = VALUE_OR_RETURN(
                    truncatePrefixToLower(legacyTypeLiteral, gLegacyInputDevicePrefix));
            if (deviceLiteral == "default") {
                deviceLiteral = "stub";
            }
            audio_source_t legacySource = VALUE_OR_RETURN(aidl2legacy_AudioSource_audio_source_t(
                    inputSourceDevice.inputSource));
            std::string inputSourceLiteral;
            if (!::android::SourceTypeConverter::toString(legacySource, inputSourceLiteral)) {
                ALOGE("%s Invalid input source  %d", __func__, legacySource);
                return unexpected(BAD_VALUE);
            }
            inputSourceLiteral = VALUE_OR_RETURN(
                    truncatePrefixToLower(inputSourceLiteral, gLegacySourcePrefix));
            legacy.configurableElement.path = std::string(gPolicyParamPrefix) + "input_sources/"
                    + inputSourceLiteral + "/applicable_input_device/mask/" + deviceLiteral;
            break;
        }
        case AudioHalCapParameter::streamVolumeProfile: {
            auto streamVolumeProfile = aidl.get<AudioHalCapParameter::streamVolumeProfile>();
            audio_stream_type_t legacyStreamType = VALUE_OR_RETURN(
                    aidl2legacy_AudioStreamType_audio_stream_type_t(streamVolumeProfile.stream));
            std::string legacyStreamLiteral;
            if (!::android::StreamTypeConverter::toString(legacyStreamType, legacyStreamLiteral)) {
                ALOGE("%s Invalid stream type  %d", __func__, legacyStreamType);
                return unexpected(BAD_VALUE);
            }
            legacyStreamLiteral = VALUE_OR_RETURN(
                    truncatePrefixToLower(legacyStreamLiteral, gLegacyStreamPrefix));

            audio_stream_type_t legacyProfile = VALUE_OR_RETURN(
                    aidl2legacy_AudioStreamType_audio_stream_type_t(streamVolumeProfile.profile));
            std::string legacyProfileLiteral;
            if (!::android::StreamTypeConverter::toString(legacyProfile, legacyProfileLiteral)) {
                ALOGE("%s Invalid profile %d", __func__, legacyProfile);
                return unexpected(BAD_VALUE);
            }
            literalValue = VALUE_OR_RETURN(
                    truncatePrefixToLower(legacyProfileLiteral, gLegacyStreamPrefix));
            legacy.configurableElement.path = std::string(gPolicyParamPrefix) + "streams/"
                    + legacyStreamLiteral + "/applicable_volume_profile/volume_profile";
            break;
        }
        default:
            return unexpected(BAD_VALUE);
    }
    legacy.value = literalValue;
    return legacy;
}

ConversionResult<CapSetting> aidl2legacy_AudioHalCapConfiguration_CapSetting(
        const media::audio::common::AudioHalCapConfiguration& aidl) {
    CapSetting legacy;
    legacy.configurationName = aidl.name;
    legacy.configurableElementValues = VALUE_OR_RETURN(convertContainer<ConfigurableElementValues>(
            aidl.parameterSettings, aidl2legacy_ParameterSetting_ConfigurableElementValue));
    return legacy;
}

ConversionResult<CapConfigurableDomain> aidl2legacy_AudioHalCapDomain_CapConfigurableDomain(
        const media::audio::common::AudioHalCapDomain& aidl) {
    CapConfigurableDomain legacy;
    legacy.name = aidl.name;
    legacy.configurations = VALUE_OR_RETURN(convertContainer<CapConfigurations>(
            aidl.configurations,
            aidl2legacy_AudioHalCapConfiguration_CapConfiguration));
    legacy.settings = VALUE_OR_RETURN(convertContainer<CapSettings>(
            aidl.configurations,
            aidl2legacy_AudioHalCapConfiguration_CapSetting));
    return legacy;
}

ConversionResult<CapCriterion> aidl2legacy_AudioHalCapCriterionV2_Criterion(
            const AudioHalCapCriterionV2& aidl) {
    CapCriterion capCriterion;
    engineConfig::Criterion& criterion = capCriterion.criterion;
    engineConfig::CriterionType& criterionType = capCriterion.criterionType;

    auto loadForceUseCriterion = [](const auto &aidlCriterion, auto &criterion,
            auto &criterionType) -> status_t {
        uint32_t legacyForceUse = VALUE_OR_RETURN_STATUS(
                aidl2legacy_AudioPolicyForceUse_audio_policy_force_use_t(
                        aidlCriterion.forceUse));
        criterion.typeName = criterionType.name;
        criterionType.name = criterion.typeName + gCriterionTypeSuffix;
        criterionType.isInclusive =
                (aidlCriterion.logic == AudioHalCapCriterionV2::LogicalDisjunction::INCLUSIVE);
        criterion.name = gForceUseCriterionTag[legacyForceUse];
        criterion.defaultLiteralValue = toString(aidlCriterion.defaultValue);
        if (aidlCriterion.values.empty()) {
            return BAD_VALUE;
        }
        for (auto &value : aidlCriterion.values) {
            uint32_t legacyForcedConfig = VALUE_OR_RETURN_STATUS(
                    aidl2legacy_AudioPolicyForcedConfig_audio_policy_forced_cfg_t(value));
            criterionType.valuePairs.push_back({legacyForcedConfig, 0, toString(value)});
        }
        return NO_ERROR;
    };

    auto loadDevicesCriterion = [](const auto &aidlCriterion, auto &criterion,
            auto &criterionType) -> status_t {
        criterionType.name = criterion.name + gCriterionTypeSuffix;
        criterionType.isInclusive =
                (aidlCriterion.logic == AudioHalCapCriterionV2::LogicalDisjunction::INCLUSIVE);
        criterion.typeName = criterionType.name;
        int shift = 0;
        if (aidlCriterion.values.empty()) {
            return BAD_VALUE;
        }
        for (const auto &value : aidlCriterion.values) {
            audio_devices_t legacyDeviceType = VALUE_OR_RETURN_STATUS(
                    aidl2legacy_AudioDeviceDescription_audio_devices_t(value));
            bool isOut = audio_is_output_devices(legacyDeviceType);
            std::string legacyTypeLiteral;
            if (!::android::DeviceConverter::toString(legacyDeviceType, legacyTypeLiteral)) {
                ALOGE("%s Invalid device type %d", __func__, legacyDeviceType);
                return BAD_VALUE;
            }
            std::string deviceLiteral = VALUE_OR_RETURN_STATUS(truncatePrefix(legacyTypeLiteral,
                    isOut ? gLegacyOutputDevicePrefix : gLegacyInputDevicePrefix));
            uint64_t pfwCriterionValue = 1 << shift++;
            criterionType.valuePairs.push_back(
                    {pfwCriterionValue, static_cast<int32_t>(legacyDeviceType), deviceLiteral});
            ALOGV("%s: adding %" PRIu64 " %d %s %s", __func__, pfwCriterionValue, legacyDeviceType,
                    toString(value.type).c_str(), deviceLiteral.c_str());
        }
        return NO_ERROR;
    };

    auto loadDeviceAddressesCriterion = [](const auto &aidlCriterion, auto &criterion,
            auto &criterionType) -> status_t {
        criterionType.name = criterion.name + gCriterionTypeSuffix;
        criterionType.isInclusive =
                (aidlCriterion.logic == AudioHalCapCriterionV2::LogicalDisjunction::INCLUSIVE);
        criterion.typeName = criterionType.name;
        int shift = 0;
        for (auto &value : aidlCriterion.values) {
            uint64_t pfwCriterionValue = 1 << shift++;
            if (value.getTag() != AudioDeviceAddress::id) {
                return BAD_VALUE;
            }
            std::string address = value.template get<AudioDeviceAddress::id>();
            criterionType.valuePairs.push_back({pfwCriterionValue, 0, address});
        }
        return NO_ERROR;
    };

    switch (aidl.getTag()) {
        case AudioHalCapCriterionV2::availableInputDevices: {
            auto aidlCriterion = aidl.get<AudioHalCapCriterionV2::availableInputDevices>();
            criterion.name = gInputDeviceCriterionName;
            if (loadDevicesCriterion(aidlCriterion, criterion, criterionType) != NO_ERROR) {
                return unexpected(BAD_VALUE);
            }
            break;
        }
        case AudioHalCapCriterionV2::availableOutputDevices: {
            auto aidlCriterion = aidl.get<AudioHalCapCriterionV2::availableOutputDevices>();
            criterion.name = gOutputDeviceCriterionName;
            if (loadDevicesCriterion(aidlCriterion, criterion, criterionType) != NO_ERROR) {
                return unexpected(BAD_VALUE);
            }
            break;
        }
        case AudioHalCapCriterionV2::availableInputDevicesAddresses: {
            auto aidlCriterion =
                    aidl.get<AudioHalCapCriterionV2::availableInputDevicesAddresses>();
            criterion.name = gInputDeviceAddressCriterionName;
            if (loadDeviceAddressesCriterion(aidlCriterion, criterion, criterionType) != NO_ERROR) {
                return unexpected(BAD_VALUE);
            }
            break;
        }
        case AudioHalCapCriterionV2::availableOutputDevicesAddresses: {
            auto aidlCriterion =
                    aidl.get<AudioHalCapCriterionV2::availableOutputDevicesAddresses>();
            criterion.name = gOutputDeviceAddressCriterionName;
            if (loadDeviceAddressesCriterion(aidlCriterion, criterion, criterionType) != NO_ERROR) {
                return unexpected(BAD_VALUE);
            }
            break;
        }
        case AudioHalCapCriterionV2::telephonyMode: {
            auto aidlCriterion = aidl.get<AudioHalCapCriterionV2::telephonyMode>();
            criterion.name = gPhoneStateCriterionName;
            criterionType.name = criterion.name  + gCriterionTypeSuffix;
            criterionType.isInclusive =
                    (aidlCriterion.logic == AudioHalCapCriterionV2::LogicalDisjunction::INCLUSIVE);
            criterion.typeName = criterionType.name;
            criterion.defaultLiteralValue = toString( aidlCriterion.defaultValue);
            if (aidlCriterion.values.empty()) {
                return unexpected(BAD_VALUE);
            }
            for (auto &value : aidlCriterion.values) {
                uint32_t legacyMode =
                        VALUE_OR_RETURN(aidl2legacy_AudioMode_audio_mode_t(value));
                criterionType.valuePairs.push_back({legacyMode, 0, toString(value)});
            }
            break;
        }
        case AudioHalCapCriterionV2::forceConfigForUse: {
            auto aidlCriterion = aidl.get<AudioHalCapCriterionV2::forceConfigForUse>();
            if (loadForceUseCriterion(aidlCriterion, criterion, criterionType) != NO_ERROR) {
                return unexpected(BAD_VALUE);
            }
            break;
        }
        default:
            return unexpected(BAD_VALUE);
    }
    return capCriterion;
}

}  // namespace

ParsingResult convert(const ::android::media::audio::common::AudioHalEngineConfig& aidlConfig) {
    auto config = std::make_unique<capEngineConfig::CapConfig>();

    if (!aidlConfig.capSpecificConfig.has_value() ||
            !aidlConfig.capSpecificConfig.value().domains.has_value()) {
        ALOGE("%s: no Cap Engine config", __func__);
        return ParsingResult{};
    }
    for (auto& aidlCriteria: aidlConfig.capSpecificConfig.value().criteriaV2.value()) {
        if (aidlCriteria.has_value()) {
            if (auto conv = aidl2legacy_AudioHalCapCriterionV2_Criterion(aidlCriteria.value());
                    conv.ok()) {
                config->capCriteria.push_back(std::move(conv.value()));
            } else {
                return ParsingResult{};
            }
        }
    }
    size_t skippedElement = 0;
    for (auto& aidlDomain: aidlConfig.capSpecificConfig.value().domains.value()) {
        if (aidlDomain.has_value()) {
            if (auto conv = aidl2legacy_AudioHalCapDomain_CapConfigurableDomain(aidlDomain.value());
                    conv.ok()) {
                config->capConfigurableDomains.push_back(std::move(conv.value()));
            } else {
                return ParsingResult{};
            }
        } else {
            skippedElement += 1;
        }
    }
    return {.parsedConfig=std::move(config), .nbSkippedElement=skippedElement};
}
} // namespace capEngineConfig
} // namespace android
