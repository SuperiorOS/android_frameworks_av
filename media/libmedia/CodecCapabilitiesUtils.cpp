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
#define LOG_TAG "CodecCapabilitiesUtils"
#include <utils/Log.h>

#include <algorithm>
#include <cmath>
#include <regex>
#include <string>
#include <vector>

#include <media/CodecCapabilitiesUtils.h>

#include <media/stagefright/foundation/AUtils.h>

namespace android {

std::optional<Range<int>> ParseIntRange(const std::string &str) {
    if (str.empty()) {
        ALOGW("could not parse empty integer range");
        return std::nullopt;
    }
    int lower, upper;
    std::regex regex("([0-9]+)-([0-9]+)");
    std::smatch match;
    if (std::regex_match(str, match, regex)) {
        lower = std::atoi(match[1].str().c_str());
        upper = std::atoi(match[2].str().c_str());
    } else if (std::atoi(str.c_str()) != 0) {
        lower = upper = std::atoi(str.c_str());
    } else {
        ALOGW("could not parse integer range: %s", str.c_str());
        return std::nullopt;
    }
    return std::make_optional<Range<int>>(lower, upper);
}

}  // namespace android