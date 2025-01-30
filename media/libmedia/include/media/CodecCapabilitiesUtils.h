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

#ifndef CODEC_CAPABILITIES__UTILS_H_

#define CODEC_CAPABILITIES__UTILS_H_

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include <utils/Log.h>

#include <media/stagefright/foundation/AUtils.h>

namespace android {

struct ProfileLevel {
    uint32_t mProfile;
    uint32_t mLevel;
    bool operator <(const ProfileLevel &o) const {
        return mProfile < o.mProfile || (mProfile == o.mProfile && mLevel < o.mLevel);
    }
};

/**
 * Immutable class for describing the range of two numeric values.
 *
 * To make it immutable, all data are private and all functions are const.
 *
 * From frameworks/base/core/java/android/util/Range.java
 */
template<typename T>
struct Range {
    Range() : lower_(), upper_() {}

    Range(T l, T u) : lower_(l), upper_(u) {}

    constexpr bool empty() const { return lower_ > upper_; }

    T lower() const { return lower_; }

    T upper() const { return upper_; }

    // Check if a value is in the range.
    bool contains(T value) const {
        return lower_ <= value && upper_ >= value;
    }

    bool contains(Range<T> range) const {
        return (range.lower_ >= lower_) && (range.upper_ <= upper_);
    }

    // Clamp a value in the range
    T clamp(T value) const{
        if (value < lower_) {
            return lower_;
        } else if (value > upper_) {
            return upper_;
        } else {
            return value;
        }
    }

    // Return the intersected range
    Range<T> intersect(Range<T> range) const {
        if (lower_ >= range.lower() && range.upper() >= upper_) {
            // range includes this
            return *this;
        } else if (range.lower() >= lower_ && range.upper() <= upper_) {
            // this includes range
            return range;
        } else {
            // if ranges are disjoint returns an empty Range(lower > upper)
            Range<T> result = Range<T>(std::max(lower_, range.lower_),
                    std::min(upper_, range.upper_));
            if (result.empty()) {
                ALOGE("Failed to intersect 2 ranges as they are disjoint");
            }
            return result;
        }
    }

    /**
     * Returns the intersection of this range and the inclusive range
     * specified by {@code [lower, upper]}.
     * <p>
     * See {@link #intersect(Range)} for more details.</p>
     *
     * @param lower a non-{@code null} {@code T} reference
     * @param upper a non-{@code null} {@code T} reference
     * @return the intersection of this range and the other range
     *
     * @throws NullPointerException if {@code lower} or {@code upper} was {@code null}
     * @throws IllegalArgumentException if the ranges are disjoint.
     */
    Range<T> intersect(T lower, T upper) {
        return Range(std::max(lower_, lower), std::min(upper_, upper));
    }

private:
    T lower_;
    T upper_;
};

static const Range<int> POSITIVE_INTEGERS = Range<int>(1, INT_MAX);

// found stuff that is not supported by framework (=> this should not happen)
constexpr int ERROR_CAPABILITIES_UNRECOGNIZED   = (1 << 0);
// found profile/level for which we don't have capability estimates
constexpr int ERROR_CAPABILITIES_UNSUPPORTED    = (1 << 1);
// have not found any profile/level for which we don't have capability estimate
// constexpr int ERROR_NONE_SUPPORTED = (1 << 2);

/**
 * Sorts distinct (non-intersecting) range array in ascending order.
 * From frameworks/base/media/java/android/media/Utils.java
 */
template<typename T>
void sortDistinctRanges(std::vector<Range<T>> *ranges) {
    std::sort(ranges->begin(), ranges->end(),
            [](Range<T> r1, Range<T> r2) {
        if (r1.upper() < r2.lower()) {
            return true;
        } else if (r1.lower() > r2.upper()) {
            return false;
        } else {
            ALOGE("sample rate ranges must be distinct.");
            return false;
        }
    });
}

/**
 * Returns the intersection of two sets of non-intersecting ranges
 * From frameworks/base/media/java/android/media/Utils.java
 * @param one a sorted set of non-intersecting ranges in ascending order
 * @param another another sorted set of non-intersecting ranges in ascending order
 * @return the intersection of the two sets, sorted in ascending order
 */
template<typename T>
std::vector<Range<T>> intersectSortedDistinctRanges(
        const std::vector<Range<T>> &one, const std::vector<Range<T>> &another) {
    std::vector<Range<T>> result;
    int ix = 0;
    for (Range<T> range : another) {
        while (ix < one.size() && one[ix].upper() < range.lower()) {
            ++ix;
        }
        while (ix < one.size() && one[ix].upper() < range.upper()) {
            result.push_back(range.intersect(one[ix]));
            ++ix;
        }
        if (ix == one.size()) {
            break;
        }
        if (one[ix].lower() <= range.upper()) {
            result.push_back(range.intersect(one[ix]));
        }
    }
    return result;
}

// parse string into int range
std::optional<Range<int>> ParseIntRange(const std::string &str);

}  // namespace android

#endif  // CODEC_CAPABILITIES__UTILS_H_