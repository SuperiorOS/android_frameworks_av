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

#include <psh_utils/PowerStatsCollector.h>

#include <benchmark/benchmark.h>

/*
 Pixel 9 Pro XL
 (tolerance is the amount of time a cached value is valid).
------------------------------------------------------------------------------------------
 Benchmark                            Time                      CPU             Iteration
------------------------------------------------------------------------------------------
audio_powerstatscollector_benchmark:
  #BM_StatsToleranceMs/0      6.346578290999787E7 ns            2069264.56 ns          100
  #BM_StatsToleranceMs/50      454.12461256065177 ns     203.1644161064639 ns      2615571
  #BM_StatsToleranceMs/100     167.74983887731364 ns    101.99598388920647 ns      5436852
  #BM_StatsToleranceMs/200     102.57950838168422 ns     79.40969988086803 ns      7600815
  #BM_StatsToleranceMs/500      86.87348495571898 ns     75.24841434306252 ns      9789318
*/

// We check how expensive it is to query stats depending
// on the tolerance to reuse the cached values.
// A tolerance of 0 means we always fetch stats.
static void BM_StatsToleranceMs(benchmark::State& state) {
    auto& collector = android::media::psh_utils::PowerStatsCollector::getCollector();
    const int64_t toleranceNs = state.range(0) * 1'000'000;
    while (state.KeepRunning()) {
        collector.getStats(toleranceNs);
        benchmark::ClobberMemory();
    }
}

// Here we test various time tolerances (given in milliseconds here)
BENCHMARK(BM_StatsToleranceMs)->Arg(0)->Arg(50)->Arg(100)->Arg(200)->Arg(500);

BENCHMARK_MAIN();
