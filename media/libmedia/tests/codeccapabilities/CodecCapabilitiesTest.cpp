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

//#define LOG_NDEBUG 0
#define LOG_TAG "CodecCapabilitiesTest"

#include <utils/Log.h>

#include <memory>

#include <gtest/gtest.h>

#include <binder/Parcel.h>

#include <media/CodecCapabilities.h>
#include <media/CodecCapabilitiesUtils.h>
#include <media/MediaCodecInfo.h>

#include <media/stagefright/MediaCodecConstants.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AString.h>

using namespace android;

class AudioCapsAacTest : public testing::Test {
protected:
    AudioCapsAacTest() {
        std::string mediaType = MIMETYPE_AUDIO_AAC;

        sp<AMessage> details = new AMessage;
        details->setString("bitrate-range", "8000-960000");
        details->setString("max-channel-count", "8");
        details->setString("sample-rate-ranges",
                "7350,8000,11025,12000,16000,22050,24000,32000,44100,48000");

        std::vector<ProfileLevel> profileLevel{
            ProfileLevel(2, 0),
            ProfileLevel(5, 0),
            ProfileLevel(29, 0),
            ProfileLevel(23, 0),
            ProfileLevel(39, 0),
            ProfileLevel(20, 0),
            ProfileLevel(42, 0),
        };

        audioCaps = AudioCapabilities::Create(mediaType, profileLevel, details);
    }

    std::shared_ptr<AudioCapabilities> audioCaps;
};

TEST_F(AudioCapsAacTest, AudioCaps_Aac_Bitrate) {
    const Range<int>& bitrateRange = audioCaps->getBitrateRange();
    EXPECT_EQ(bitrateRange.lower(), 8000) << "bitrate range1 does not match. lower: "
            << bitrateRange.lower();
    EXPECT_EQ(bitrateRange.upper(), 510000) << "bitrate range1 does not match. upper: "
            << bitrateRange.upper();
}

TEST_F(AudioCapsAacTest, AudioCaps_Aac_InputChannelCount) {
    int maxInputChannelCount = audioCaps->getMaxInputChannelCount();
    EXPECT_EQ(maxInputChannelCount, 8);
    int minInputChannelCount = audioCaps->getMinInputChannelCount();
    EXPECT_EQ(minInputChannelCount, 1);
}

TEST_F(AudioCapsAacTest, AudioCaps_Aac_SupportedSampleRates) {
    const std::vector<int>& sampleRates = audioCaps->getSupportedSampleRates();
    EXPECT_EQ(sampleRates, std::vector<int>({7350, 8000, 11025, 12000, 16000, 22050,
            24000, 32000, 44100, 48000}));

    EXPECT_FALSE(audioCaps->isSampleRateSupported(6000))
            << "isSampleRateSupported returned true for unsupported sample rate";
    EXPECT_TRUE(audioCaps->isSampleRateSupported(8000))
            << "isSampleRateSupported returned false for supported sample rate";
    EXPECT_TRUE(audioCaps->isSampleRateSupported(12000))
            << "isSampleRateSupported returned false for supported sample rate";
    EXPECT_FALSE(audioCaps->isSampleRateSupported(44000))
            << "isSampleRateSupported returned true for unsupported sample rate";
    EXPECT_TRUE(audioCaps->isSampleRateSupported(48000))
            << "isSampleRateSupported returned true for unsupported sample rate";
}

class AudioCapsRawTest : public testing::Test {
protected:
    AudioCapsRawTest() {
        std::string mediaType = MIMETYPE_AUDIO_RAW;

        sp<AMessage> details = new AMessage;
        details->setString("bitrate-range", "1-10000000");
        details->setString("channel-ranges", "1,2,3,4,5,6,7,8,9,10,11,12");
        details->setString("sample-rate-ranges", "8000-192000");

        std::vector<ProfileLevel> profileLevel;

        audioCaps = AudioCapabilities::Create(mediaType, profileLevel, details);
    }

    std::shared_ptr<AudioCapabilities> audioCaps;
};

TEST_F(AudioCapsRawTest, AudioCaps_Raw_Bitrate) {
    const Range<int>& bitrateRange = audioCaps->getBitrateRange();
    EXPECT_EQ(bitrateRange.lower(), 1);
    EXPECT_EQ(bitrateRange.upper(), 10000000);
}

TEST_F(AudioCapsRawTest, AudioCaps_Raw_InputChannelCount) {
    int maxInputChannelCount = audioCaps->getMaxInputChannelCount();
    EXPECT_EQ(maxInputChannelCount, 12);
    int minInputChannelCount = audioCaps->getMinInputChannelCount();
    EXPECT_EQ(minInputChannelCount, 1);
}

TEST_F(AudioCapsRawTest, AudioCaps_Raw_InputChannelCountRanges) {
    const std::vector<Range<int>>& inputChannelCountRanges
            = audioCaps->getInputChannelCountRanges();
    std::vector<Range<int>> expectedOutput({{1,1}, {2,2}, {3,3}, {4,4}, {5,5},
            {6,6}, {7,7}, {8,8}, {9,9}, {10,10}, {11,11}, {12,12}});
    ASSERT_EQ(inputChannelCountRanges.size(), expectedOutput.size());
    for (int i = 0; i < inputChannelCountRanges.size(); i++) {
        EXPECT_EQ(inputChannelCountRanges.at(i).lower(), expectedOutput.at(i).lower());
        EXPECT_EQ(inputChannelCountRanges.at(i).upper(), expectedOutput.at(i).upper());
    }
}

TEST_F(AudioCapsRawTest, AudioCaps_Raw_SupportedSampleRates) {
    const std::vector<Range<int>>& sampleRateRanges = audioCaps->getSupportedSampleRateRanges();
    EXPECT_EQ(sampleRateRanges.size(), 1);
    EXPECT_EQ(sampleRateRanges.at(0).lower(), 8000);
    EXPECT_EQ(sampleRateRanges.at(0).upper(), 192000);

    EXPECT_EQ(audioCaps->isSampleRateSupported(7000), false);
    EXPECT_EQ(audioCaps->isSampleRateSupported(10000), true);
    EXPECT_EQ(audioCaps->isSampleRateSupported(193000), false);
}
