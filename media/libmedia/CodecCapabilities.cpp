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
#define LOG_TAG "CodecCapabilities"

#include <utils/Log.h>
#include <media/CodecCapabilities.h>
#include <media/CodecCapabilitiesUtils.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>

namespace android {

bool CodecCapabilities::SupportsBitrate(Range<int> bitrateRange,
        const sp<AMessage> &format) {
    // consider max bitrate over average bitrate for support
    int32_t maxBitrate = 0;
    format->findInt32(KEY_MAX_BIT_RATE, &maxBitrate);
    int32_t bitrate = 0;
    format->findInt32(KEY_BIT_RATE, &bitrate);

    if (bitrate == 0) {
        bitrate = maxBitrate;
    } else if (maxBitrate != 0) {
        bitrate = std::max(bitrate, maxBitrate);
    }

    if (bitrate > 0) {
        return bitrateRange.contains(bitrate);
    }

    return true;
}

const std::string& CodecCapabilities::getMediaType() {
    return mMediaType;
}

const std::vector<ProfileLevel>& CodecCapabilities::getProfileLevels() {
    return mProfileLevels;
}

}  // namespace android