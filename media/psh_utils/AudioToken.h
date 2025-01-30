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

#include <psh_utils/PowerClientStats.h>
#include <psh_utils/Token.h>

#include <atomic>
#include <memory>
#include <string>

namespace android::media::psh_utils {

class AudioClientToken : public Token {
public:
    AudioClientToken(std::shared_ptr<PowerClientStats> powerClientStats, pid_t pid, uid_t uid,
             const std::string& additional);
    ~AudioClientToken() override;

    // AudioPowerManager may call toString() while AudioToken is in its dtor.
    // It is safe so long as toString is final.
    std::string toString() const final;

private:
    const std::shared_ptr<PowerClientStats> mPowerClientStats;
    const pid_t mPid;
    const std::string mAdditional;
    const size_t mId;
    static constinit std::atomic<size_t> sIdCounter;
};

class AudioThreadToken : public Token {
public:
    AudioThreadToken(
            pid_t tid, const std::string& wakeLockName,
            WakeFlag wakeFlag, const std::string& additional);
    ~AudioThreadToken() override;

    // AudioPowerManager may call toString() while AudioToken is in its dtor.
    // It is safe so long as toString is final.
    std::string toString() const final;

private:
    const pid_t mTid;
    const std::string mWakeLockName;
    const WakeFlag mWakeFlag;
    const std::string mAdditional;
    const size_t mId;
    static constinit std::atomic<size_t> sIdCounter;
};

class AudioTrackToken : public Token {
public:
    AudioTrackToken(
            std::shared_ptr<PowerClientStats> powerClientStats, const std::string& additional);
    ~AudioTrackToken() override;

    // AudioPowerManager may call toString() while AudioToken is in its dtor.
    // It is safe so long as toString is final.
    std::string toString() const final;

private:
    const std::shared_ptr<PowerClientStats> mPowerClientStats;
    const std::string mAdditional;
    const size_t mId;
    static constinit std::atomic<size_t> sIdCounter;
};

} // namespace android::media::psh_utils
