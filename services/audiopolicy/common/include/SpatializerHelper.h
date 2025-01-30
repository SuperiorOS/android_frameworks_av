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

#include <com_android_media_audio.h>
#include <cutils/properties.h>

namespace android {

class SpatializerHelper {
  public:
    /**
     * @brief Check if the stereo spatialization feature turned on by:
     *        - sysprop "ro.audio.stereo_spatialization_enabled" is true
     *        - com_android_media_audio_stereo_spatialization flag is on
     *
     * @return true if the stereo spatialization feature is enabled
     * @return false if the stereo spatialization feature is not enabled
     */
    static bool isStereoSpatializationFeatureEnabled() {
        static const bool stereoSpatializationEnabled =
                property_get_bool("ro.audio.stereo_spatialization_enabled", false) &&
                com_android_media_audio_stereo_spatialization();
        return stereoSpatializationEnabled;
    }
};

} // namespace android
