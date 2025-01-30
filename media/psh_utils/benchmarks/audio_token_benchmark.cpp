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

#define LOG_TAG "audio_token_benchmark"
#include <utils/Log.h>

#include <psh_utils/Token.h>

#include <benchmark/benchmark.h>

/*
 Pixel 9 Pro XL
------------------------------------------------------------------------------------------
 Benchmark                            Time                      CPU             Iteration
------------------------------------------------------------------------------------------
audio_token_benchmark:
  #BM_ClientToken     494.6548907301575 ns     492.4932166101717 ns      1376819
  #BM_ThreadToken    140.34316175293938 ns    139.91778452790845 ns      5000397
  #BM_TrackToken      944.0571625384163 ns     893.7912613357879 ns       643096
*/

static void BM_ClientToken(benchmark::State& state) {
    constexpr pid_t kPid = 10;
    constexpr uid_t kUid = 100;
    while (state.KeepRunning()) {
        auto token = android::media::psh_utils::createAudioClientToken(
                kPid, kUid);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(BM_ClientToken);

static void BM_ThreadToken(benchmark::State& state) {
    constexpr pid_t kTid = 20;
    constexpr const char* kWakeLockTag = "thread";
    while (state.KeepRunning()) {
        auto token = android::media::psh_utils::createAudioThreadToken(
                kTid, kWakeLockTag);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(BM_ThreadToken);

static void BM_TrackToken(benchmark::State& state) {
    constexpr pid_t kPid = 10;
    constexpr uid_t kUid = 100;
    auto clientToken = android::media::psh_utils::createAudioClientToken(
                kPid, kUid);
    while (state.KeepRunning()) {
        auto token = android::media::psh_utils::createAudioTrackToken(kUid);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(BM_TrackToken);

BENCHMARK_MAIN();
