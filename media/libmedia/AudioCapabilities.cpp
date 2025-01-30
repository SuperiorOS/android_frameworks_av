/*
 * Copyright 2024, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "AudioCapabilities"

#include <android-base/strings.h>
#include <android-base/properties.h>

#include <media/AudioCapabilities.h>
#include <media/CodecCapabilities.h>
#include <media/stagefright/MediaCodecConstants.h>

namespace android {

const Range<int>& AudioCapabilities::getBitrateRange() const {
    return mBitrateRange;
}

const std::vector<int>& AudioCapabilities::getSupportedSampleRates() const {
    return mSampleRates;
}

const std::vector<Range<int>>&
        AudioCapabilities::getSupportedSampleRateRanges() const {
    return mSampleRateRanges;
}

int AudioCapabilities::getMaxInputChannelCount() const {
    int overallMax = 0;
    for (int i = mInputChannelRanges.size() - 1; i >= 0; i--) {
        int lmax = mInputChannelRanges[i].upper();
        if (lmax > overallMax) {
            overallMax = lmax;
        }
    }
    return overallMax;
}

int AudioCapabilities::getMinInputChannelCount() const {
    int overallMin = MAX_INPUT_CHANNEL_COUNT;
    for (int i = mInputChannelRanges.size() - 1; i >= 0; i--) {
        int lmin = mInputChannelRanges[i].lower();
        if (lmin < overallMin) {
            overallMin = lmin;
        }
    }
    return overallMin;
}

const std::vector<Range<int>>&
        AudioCapabilities::getInputChannelCountRanges() const {
    return mInputChannelRanges;
}

// static
std::shared_ptr<AudioCapabilities> AudioCapabilities::Create(std::string mediaType,
        std::vector<ProfileLevel> profLevs, const sp<AMessage> &format) {
    std::shared_ptr<AudioCapabilities> caps(new AudioCapabilities());
    caps->init(mediaType, profLevs, format);
    return caps;
}

void AudioCapabilities::init(std::string mediaType, std::vector<ProfileLevel> profLevs,
        const sp<AMessage> &format) {
    mMediaType = mediaType;
    mProfileLevels = profLevs;
    mError = 0;

    initWithPlatformLimits();
    applyLevelLimits();
    parseFromInfo(format);
}

void AudioCapabilities::initWithPlatformLimits() {
    mBitrateRange = Range<int>(0, INT_MAX);
    mInputChannelRanges.push_back(Range<int>(1, MAX_INPUT_CHANNEL_COUNT));

    const int minSampleRate = base::GetIntProperty("ro.mediacodec.min_sample_rate", 7350);
    const int maxSampleRate = base::GetIntProperty("ro.mediacodec.max_sample_rate", 192000);
    mSampleRateRanges.push_back(Range<int>(minSampleRate, maxSampleRate));
}

bool AudioCapabilities::supports(int sampleRate, int inputChannels) {
    // channels and sample rates are checked orthogonally
    if (inputChannels != 0
            && !std::any_of(mInputChannelRanges.begin(), mInputChannelRanges.end(),
            [inputChannels](const Range<int> &a) { return a.contains(inputChannels); })) {
        return false;
    }
    if (sampleRate != 0
            && !std::any_of(mSampleRateRanges.begin(), mSampleRateRanges.end(),
            [sampleRate](const Range<int> &a) { return a.contains(sampleRate); })) {
        return false;
    }
    return true;
}

bool AudioCapabilities::isSampleRateSupported(int sampleRate) {
    return supports(sampleRate, 0);
}

void AudioCapabilities::limitSampleRates(std::vector<int> rates) {
    std::vector<Range<int>> sampleRateRanges;
    std::sort(rates.begin(), rates.end());
    for (int rate : rates) {
        if (supports(rate, 0 /* channels */)) {
            sampleRateRanges.push_back(Range<int>(rate, rate));
        }
    }
    mSampleRateRanges = intersectSortedDistinctRanges(mSampleRateRanges, sampleRateRanges);
    createDiscreteSampleRates();
}

void AudioCapabilities::createDiscreteSampleRates() {
    mSampleRates.clear();
    for (int i = 0; i < mSampleRateRanges.size(); i++) {
        mSampleRates.push_back(mSampleRateRanges[i].lower());
    }
}

void AudioCapabilities::limitSampleRates(std::vector<Range<int>> rateRanges) {
    sortDistinctRanges(&rateRanges);
    mSampleRateRanges = intersectSortedDistinctRanges(mSampleRateRanges, rateRanges);
    // check if all values are discrete
    for (Range<int> range: mSampleRateRanges) {
        if (range.lower() != range.upper()) {
            mSampleRates.clear();
            return;
        }
    }
    createDiscreteSampleRates();
}

void AudioCapabilities::applyLevelLimits() {
    std::vector<int> sampleRates;
    std::optional<Range<int>> sampleRateRange;
    std::optional<Range<int>> bitRates;
    int maxChannels = MAX_INPUT_CHANNEL_COUNT;

    // const char *mediaType = mMediaType.c_str();
    if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_MPEG)) {
        sampleRates = {
                8000, 11025, 12000,
                16000, 22050, 24000,
                32000, 44100, 48000 };
        bitRates = Range<int>(8000, 320000);
        maxChannels = 2;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_AMR_NB)) {
        sampleRates = { 8000 };
        bitRates = Range<int>(4750, 12200);
        maxChannels = 1;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_AMR_WB)) {
        sampleRates = { 16000 };
        bitRates = Range<int>(6600, 23850);
        maxChannels = 1;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_AAC)) {
        sampleRates = {
                7350, 8000,
                11025, 12000, 16000,
                22050, 24000, 32000,
                44100, 48000, 64000,
                88200, 96000 };
        bitRates = Range<int>(8000, 510000);
        maxChannels = 48;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_VORBIS)) {
        bitRates = Range<int>(32000, 500000);
        sampleRateRange = Range<int>(8000, 192000);
        maxChannels = 255;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_OPUS)) {
        bitRates = Range<int>(6000, 510000);
        sampleRates = { 8000, 12000, 16000, 24000, 48000 };
        maxChannels = 255;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_RAW)) {
        sampleRateRange = Range<int>(1, 192000);
        bitRates = Range<int>(1, 10000000);
        maxChannels = MAX_NUM_CHANNELS;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_FLAC)) {
        sampleRateRange = Range<int>(1, 655350);
        // lossless codec, so bitrate is ignored
        maxChannels = 255;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_G711_ALAW)
            || base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_G711_MLAW)) {
        sampleRates = { 8000 };
        bitRates = Range<int>(64000, 64000);
        // platform allows multiple channels for this format
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_MSGSM)) {
        sampleRates = { 8000 };
        bitRates = Range<int>(13000, 13000);
        maxChannels = 1;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_AC3)) {
        maxChannels = 6;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_EAC3)) {
        maxChannels = 16;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_EAC3_JOC)) {
        sampleRates = { 48000 };
        bitRates = Range<int>(32000, 6144000);
        maxChannels = 16;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_AC4)) {
        sampleRates = { 44100, 48000, 96000, 192000 };
        bitRates = Range<int>(16000, 2688000);
        maxChannels = 24;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_DTS)) {
        sampleRates = { 44100, 48000 };
        bitRates = Range<int>(96000, 1524000);
        maxChannels = 6;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_DTS_HD)) {
        for (ProfileLevel profileLevel: mProfileLevels) {
            switch (profileLevel.mProfile) {
                case DTS_HDProfileLBR:
                    sampleRates = { 22050, 24000, 44100, 48000 };
                    bitRates = Range<int>(32000, 768000);
                    break;
                case DTS_HDProfileHRA:
                case DTS_HDProfileMA:
                    sampleRates = { 44100, 48000, 88200, 96000, 176400, 192000 };
                    bitRates = Range<int>(96000, 24500000);
                    break;
                default:
                    ALOGW("Unrecognized profile %d for %s", profileLevel.mProfile,
                            mMediaType.c_str());
                    mError |= ERROR_CAPABILITIES_UNRECOGNIZED;
                    sampleRates = { 44100, 48000, 88200, 96000, 176400, 192000 };
                    bitRates = Range<int>(96000, 24500000);
            }
        }
        maxChannels = 8;
    } else if (base::EqualsIgnoreCase(mMediaType, MIMETYPE_AUDIO_DTS_UHD)) {
        for (ProfileLevel profileLevel: mProfileLevels) {
            switch (profileLevel.mProfile) {
                case DTS_UHDProfileP2:
                    sampleRates = { 48000 };
                    bitRates = Range<int>(96000, 768000);
                    maxChannels = 10;
                    break;
                case DTS_UHDProfileP1:
                    sampleRates = { 44100, 48000, 88200, 96000, 176400, 192000 };
                    bitRates = Range<int>(96000, 24500000);
                    maxChannels = 32;
                    break;
                default:
                    ALOGW("Unrecognized profile %d for %s", profileLevel.mProfile,
                            mMediaType.c_str());
                    mError |= ERROR_CAPABILITIES_UNRECOGNIZED;
                    sampleRates = { 44100, 48000, 88200, 96000, 176400, 192000 };
                    bitRates = Range<int>(96000, 24500000);
                    maxChannels = 32;
            }
        }
    } else {
        ALOGW("Unsupported mediaType %s", mMediaType.c_str());
        mError |= ERROR_CAPABILITIES_UNSUPPORTED;
    }

    // restrict ranges
    if (!sampleRates.empty()) {
        limitSampleRates(sampleRates);
    } else if (sampleRateRange) {
        std::vector<Range<int>> rateRanges = { sampleRateRange.value() };
        limitSampleRates(rateRanges);
    }

    Range<int> channelRange = Range<int>(1, maxChannels);
    std::vector<Range<int>> inputChannels = { channelRange };
    applyLimits(inputChannels, bitRates);
}

void AudioCapabilities::applyLimits(
        const std::vector<Range<int>> &inputChannels,
        const std::optional<Range<int>> &bitRates) {
    // clamp & make a local copy
    std::vector<Range<int>> inputChannelsCopy(inputChannels.size());
    for (int i = 0; i < inputChannels.size(); i++) {
        int lower = inputChannels[i].clamp(1);
        int upper = inputChannels[i].clamp(MAX_INPUT_CHANNEL_COUNT);
        inputChannelsCopy[i] = Range<int>(lower, upper);
    }

    // sort, intersect with existing, & save channel list
    sortDistinctRanges(&inputChannelsCopy);
    mInputChannelRanges = intersectSortedDistinctRanges(inputChannelsCopy, mInputChannelRanges);

    if (bitRates) {
        mBitrateRange = mBitrateRange.intersect(bitRates.value());
    }
}

void AudioCapabilities::parseFromInfo(const sp<AMessage> &format) {
    int maxInputChannels = MAX_INPUT_CHANNEL_COUNT;
    std::vector<Range<int>> channels = { Range<int>(1, maxInputChannels) };
    std::optional<Range<int>> bitRates = POSITIVE_INTEGERS;

    AString rateAString;
    if (format->findString("sample-rate-ranges", &rateAString)) {
        std::vector<std::string> rateStrings = base::Split(std::string(rateAString.c_str()), ",");
        std::vector<Range<int>> rateRanges;
        for (std::string rateString : rateStrings) {
            std::optional<Range<int>> rateRange = ParseIntRange(rateString);
            if (!rateRange) {
                continue;
            }
            rateRanges.push_back(rateRange.value());
        }
        limitSampleRates(rateRanges);
    }

    // we will prefer channel-ranges over max-channel-count
    AString valueStr;
    if (format->findString("channel-ranges", &valueStr)) {
        std::vector<std::string> channelStrings = base::Split(std::string(valueStr.c_str()), ",");
        std::vector<Range<int>> channelRanges;
        for (std::string channelString : channelStrings) {
            std::optional<Range<int>> channelRange = ParseIntRange(channelString);
            if (!channelRange) {
                continue;
            }
            channelRanges.push_back(channelRange.value());
        }
        channels = channelRanges;
    } else if (format->findString("channel-range", &valueStr)) {
        std::optional<Range<int>> oneRange = ParseIntRange(std::string(valueStr.c_str()));
        if (oneRange) {
            channels = { oneRange.value() };
        }
    } else if (format->findString("max-channel-count", &valueStr)) {
        maxInputChannels = std::atoi(valueStr.c_str());
        if (maxInputChannels == 0) {
            channels = { Range<int>(0, 0) };
        } else {
            channels = { Range<int>(1, maxInputChannels) };
        }
    } else if ((mError & ERROR_CAPABILITIES_UNSUPPORTED) != 0) {
        maxInputChannels = 0;
        channels = { Range<int>(0, 0) };
    }

    if (format->findString("bitrate-range", &valueStr)) {
        std::optional<Range<int>> parsedBitrate = ParseIntRange(valueStr.c_str());
        if (parsedBitrate) {
            bitRates = bitRates.value().intersect(parsedBitrate.value());
        }
    }

    applyLimits(channels, bitRates);
}

void AudioCapabilities::getDefaultFormat(sp<AMessage> &format) {
    // report settings that have only a single choice
    if (mBitrateRange.lower() == mBitrateRange.upper()) {
        format->setInt32(KEY_BIT_RATE, mBitrateRange.lower());
    }
    if (getMaxInputChannelCount() == 1) {
        // mono-only format
        format->setInt32(KEY_CHANNEL_COUNT, 1);
    }
    if (!mSampleRates.empty() && mSampleRates.size() == 1) {
        format->setInt32(KEY_SAMPLE_RATE, mSampleRates[0]);
    }
}

bool AudioCapabilities::supportsFormat(const sp<AMessage> &format) {
    int32_t sampleRate;
    format->findInt32(KEY_SAMPLE_RATE, &sampleRate);
    int32_t channels;
    format->findInt32(KEY_CHANNEL_COUNT, &channels);

    if (!supports(sampleRate, channels)) {
        return false;
    }

    if (!CodecCapabilities::SupportsBitrate(mBitrateRange, format)) {
        return false;
    }

    // nothing to do for:
    // KEY_CHANNEL_MASK: codecs don't get this
    // KEY_IS_ADTS:      required feature for all AAC decoders
    return true;
}

}  // namespace android