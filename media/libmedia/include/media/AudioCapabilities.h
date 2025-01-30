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

#ifndef AUDIO_CAPABILITIES_H_

#define AUDIO_CAPABILITIES_H_

#include <media/CodecCapabilitiesUtils.h>
#include <media/stagefright/foundation/AMessage.h>

#include <system/audio.h>

#include <utils/StrongPointer.h>

namespace android {

struct AudioCapabilities {
    /**
     * Create AudioCapabilities.
     */
    static std::shared_ptr<AudioCapabilities> Create(std::string mediaType,
            std::vector<ProfileLevel> profLevs, const sp<AMessage> &format);

    /**
     * Returns the range of supported bitrates in bits/second.
     */
    const Range<int>& getBitrateRange() const;

    /**
     * Returns the array of supported sample rates if the codec
     * supports only discrete values. Otherwise, it returns an empty array.
     * The array is sorted in ascending order.
     */
    const std::vector<int>& getSupportedSampleRates() const;

    /**
     * Returns the array of supported sample rate ranges.  The
     * array is sorted in ascending order, and the ranges are
     * distinct.
     */
    const std::vector<Range<int>>& getSupportedSampleRateRanges() const;

    /**
     * Returns the maximum number of input channels supported.
     * The returned value should be between 1 and 255.
     *
     * Through {@link android.os.Build.VERSION_CODES#R}, this method indicated support
     * for any number of input channels between 1 and this maximum value.
     *
     * As of {@link android.os.Build.VERSION_CODES#S},
     * the implied lower limit of 1 channel is no longer valid.
     * As of {@link android.os.Build.VERSION_CODES#S}, {@link #getMaxInputChannelCount} is
     * superseded by {@link #getInputChannelCountRanges},
     * which returns an array of ranges of channels.
     * The {@link #getMaxInputChannelCount} method will return the highest value
     * in the ranges returned by {@link #getInputChannelCountRanges}
     */
    int getMaxInputChannelCount() const;

    /**
     * Returns the minimum number of input channels supported.
     * This is often 1, but does vary for certain mime types.
     *
     * This returns the lowest channel count in the ranges returned by
     * {@link #getInputChannelCountRanges}.
     */
    int getMinInputChannelCount() const;

    /**
     * Returns an array of ranges representing the number of input channels supported.
     * The codec supports any number of input channels within this range.
     *
     * This supersedes the {@link #getMaxInputChannelCount} method.
     *
     * For many codecs, this will be a single range [1..N], for some N.
     *
     * The returned array cannot be empty.
     */
    const std::vector<Range<int>>& getInputChannelCountRanges() const;

    /**
     * Query whether the sample rate is supported by the codec.
     */
    bool isSampleRateSupported(int sampleRate);

    /* For internal use only. Not exposed as a public API */
    void getDefaultFormat(sp<AMessage> &format);

    /* For internal use only. Not exposed as a public API */
    bool supportsFormat(const sp<AMessage> &format);

private:
    static constexpr int MAX_INPUT_CHANNEL_COUNT = 30;
    static constexpr uint32_t MAX_NUM_CHANNELS = FCC_LIMIT;

    int mError;
    std::string mMediaType;
    std::vector<ProfileLevel> mProfileLevels;

    Range<int> mBitrateRange;

    std::vector<int> mSampleRates;
    std::vector<Range<int>> mSampleRateRanges;
    std::vector<Range<int>> mInputChannelRanges;

    /* no public constructor */
    AudioCapabilities() {}
    void init(std::string mediaType, std::vector<ProfileLevel> profLevs,
            const sp<AMessage> &format);
    void initWithPlatformLimits();
    bool supports(int sampleRate, int inputChannels);
    void limitSampleRates(std::vector<int> rates);
    void createDiscreteSampleRates();
    void limitSampleRates(std::vector<Range<int>> rateRanges);
    void applyLevelLimits();
    void applyLimits(const std::vector<Range<int>> &inputChannels,
            const std::optional<Range<int>> &bitRates);
    void parseFromInfo(const sp<AMessage> &format);

    friend struct CodecCapabilities;
};

}  // namespace android

#endif // AUDIO_CAPABILITIES_H_