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

#define LOG_TAG "audio_powerstat_benchmark"
#include <cutils/properties.h>
#include <utils/Log.h>

#include <psh_utils/PerformanceFixture.h>

#include <algorithm>
#include <android-base/strings.h>
#include <random>
#include <thread>
#include <vector>

/*
Pixel 9 XL Pro
---------------------------------------------------------------------------------------------------------------
Benchmark                                               Time                        CPU              Iteration
---------------------------------------------------------------------------------------------------------------
audio_powerstats_benchmark:
  #MemoryFixture/CacheAccess/64/0/0/1             5.195761589711465 ns       5.183635029038574 ns    135160912
  #MemoryFixture/CacheAccess/128/0/0/1            10.37270431027728 ns      10.341754343667125 ns     67574354
  #MemoryFixture/CacheAccess/256/0/0/1           20.767353363364098 ns      20.708496782017836 ns     33809541
  #MemoryFixture/CacheAccess/512/0/0/1            41.53473855852046 ns       41.45724926375999 ns     16900399
  #MemoryFixture/CacheAccess/1024/0/0/1           82.89673650172568 ns       82.68064919937272 ns      8462177
  #MemoryFixture/CacheAccess/2048/0/0/1          165.77648929323732 ns      165.45127650324827 ns      4227878
  #MemoryFixture/CacheAccess/4096/0/0/1           331.9272979248067 ns       331.0722959129879 ns      2114919
  #MemoryFixture/CacheAccess/8192/0/0/1           663.8090302013887 ns       662.2813002594532 ns      1054528
  #MemoryFixture/CacheAccess/16384/0/0/1         1327.4224893455748 ns      1324.0114138292752 ns       529095
  #MemoryFixture/CacheAccess/32768/0/0/1         2657.1037276954685 ns      2651.8974883509522 ns       263970
  #MemoryFixture/CacheAccess/65536/0/0/1          5314.170125835522 ns        5305.20127734871 ns       131679
  #MemoryFixture/CacheAccess/131072/0/0/1        10624.517848490625 ns      10602.467739493763 ns        66056
  #MemoryFixture/CacheAccess/262144/0/0/1         21271.09560700047 ns      21224.851075464823 ns        32916
  #MemoryFixture/CacheAccess/524288/0/0/1         42556.76641626909 ns       42444.65628786041 ns        16508
  #MemoryFixture/CacheAccess/1048576/0/0/1         85440.6313100312 ns       85076.15703685701 ns         8221
  #MemoryFixture/CacheAccess/2097152/0/0/1       170908.37391089948 ns      169402.58059051324 ns         4132
  #MemoryFixture/CacheAccess/4194304/0/0/1        373635.4350000207 ns      372955.40777777723 ns         1800
  #MemoryFixture/CacheAccess/8388608/0/0/1        685101.2127660838 ns       681594.2330754338 ns         1034
  #MemoryFixture/CacheAccess/16777216/0/0/1       1588009.158696047 ns      1581510.0217391246 ns          460
  #MemoryFixture/CacheAccess/33554432/0/0/1      2721626.5387591715 ns       2708149.166666674 ns          258
  #MemoryFixture/CacheAccess/67108864/0/0/1       5433705.728680161 ns       5413515.914728661 ns          129
  #MemoryFixture/CacheAccess/64/1/0/1             5.201213751848433 ns       5.180967321755995 ns    135673202
  #MemoryFixture/CacheAccess/128/1/0/1           10.386209252693448 ns      10.351378045484042 ns     67486234
  #MemoryFixture/CacheAccess/256/1/0/1           20.742666405371747 ns      20.686210353062208 ns     33848169
  #MemoryFixture/CacheAccess/512/1/0/1            41.52781367071582 ns        41.4438085044977 ns     16870391
  #MemoryFixture/CacheAccess/1024/1/0/1           83.03849985460687 ns        82.6732197351271 ns      8442915
  #MemoryFixture/CacheAccess/2048/1/0/1          166.02706801365886 ns      165.60695561393828 ns      4226169
  #MemoryFixture/CacheAccess/4096/1/0/1          332.05696890075365 ns       331.3395040136246 ns      2112679
  #MemoryFixture/CacheAccess/8192/1/0/1           664.3009073119205 ns       662.5811781316487 ns      1055315
  #MemoryFixture/CacheAccess/16384/1/0/1         1329.0792867154223 ns      1325.0185471435798 ns       527251
  #MemoryFixture/CacheAccess/32768/1/0/1         2652.9089904482526 ns      2645.5388137876826 ns       264236
  #MemoryFixture/CacheAccess/65536/1/0/1          5312.635002724743 ns       5300.412875575496 ns       132064
  #MemoryFixture/CacheAccess/131072/1/0/1        10625.299202810635 ns      10594.376178351697 ns        65982
  #MemoryFixture/CacheAccess/262144/1/0/1        21270.763359464152 ns      21206.192921372138 ns        33029
  #MemoryFixture/CacheAccess/524288/1/0/1         42496.14168177758 ns      42381.692498487435 ns        16530
  #MemoryFixture/CacheAccess/1048576/1/0/1        85425.34302253627 ns       85063.54206182965 ns         8119
  #MemoryFixture/CacheAccess/2097152/1/0/1        170961.8011639407 ns      169732.18840931158 ns         4124
  #MemoryFixture/CacheAccess/4194304/1/0/1       440086.77439029363 ns      439010.07195122127 ns         1640
  #MemoryFixture/CacheAccess/8388608/1/0/1        677684.5246376056 ns        675888.058937202 ns         1035
  #MemoryFixture/CacheAccess/16777216/1/0/1      1571417.6297115851 ns      1566423.4922394755 ns          451
  #MemoryFixture/CacheAccess/33554432/1/0/1       2723418.325581582 ns      2708535.8062015465 ns          258
  #MemoryFixture/CacheAccess/67108864/1/0/1       5435209.372094963 ns       5413991.821705426 ns          129
  #MemoryFixture/CacheAccess/64/2/0/1             5.204700890931938 ns       5.183036658664568 ns    135406460
  #MemoryFixture/CacheAccess/128/2/0/1           10.376355078178406 ns      10.348754235460566 ns     67518337
  #MemoryFixture/CacheAccess/256/2/0/1           20.726238269323797 ns      20.670377070062745 ns     33834057
  #MemoryFixture/CacheAccess/512/2/0/1            41.49336362336435 ns       41.38084547087122 ns     16890919
  #MemoryFixture/CacheAccess/1024/2/0/1           82.96761236822086 ns        82.7676652782051 ns      8440753
  #MemoryFixture/CacheAccess/2048/2/0/1          165.90277344671065 ns      165.63781837950896 ns      4223301
  #MemoryFixture/CacheAccess/4096/2/0/1          332.08415463794364 ns      331.07455763248197 ns      2110971
  #MemoryFixture/CacheAccess/8192/2/0/1            662.569068830069 ns       661.1656746088121 ns      1055382
  #MemoryFixture/CacheAccess/16384/2/0/1         1327.7767031437843 ns       1324.268331214332 ns       528552
  #MemoryFixture/CacheAccess/32768/2/0/1          2654.714983405558 ns      2647.9097623853727 ns       264546
  #MemoryFixture/CacheAccess/65536/2/0/1          5304.774145979664 ns       5290.380748589911 ns       131554
  #MemoryFixture/CacheAccess/131072/2/0/1          10631.0978039924 ns      10602.125724165107 ns        65938
  #MemoryFixture/CacheAccess/262144/2/0/1        21258.936606489668 ns      21202.585867458216 ns        33016
  #MemoryFixture/CacheAccess/524288/2/0/1         42460.85577331506 ns      42355.304775195626 ns        16481
  #MemoryFixture/CacheAccess/1048576/2/0/1        85428.60153206348 ns       85160.29036964968 ns         8224
  #MemoryFixture/CacheAccess/2097152/2/0/1       170233.91140170072 ns      169409.06511740564 ns         4131
  #MemoryFixture/CacheAccess/4194304/2/0/1        402022.9022378908 ns      401018.78327444167 ns         1698
  #MemoryFixture/CacheAccess/8388608/2/0/1        677677.2908216701 ns       675843.1642512042 ns         1035
  #MemoryFixture/CacheAccess/16777216/2/0/1      1554294.1339100641 ns       1549490.831533465 ns          463
  #MemoryFixture/CacheAccess/33554432/2/0/1       2722937.453488912 ns       2709007.093023249 ns          258
  #MemoryFixture/CacheAccess/67108864/2/0/1      5435791.6511618495 ns       5415184.511627913 ns          129
  #MemoryFixture/CacheAccess/64/0/2/1             5.198916380394579 ns       5.178607363536682 ns    135270162
  #MemoryFixture/CacheAccess/128/0/2/1            10.39285189976819 ns      10.361873548918089 ns     67571996
  #MemoryFixture/CacheAccess/256/0/2/1            20.75920269645178 ns       20.69937217558235 ns     33765810
  #MemoryFixture/CacheAccess/512/0/2/1            41.51556112567147 ns      41.372342254073665 ns     16947585
  #MemoryFixture/CacheAccess/1024/0/2/1           82.96516513710067 ns       82.78508396196703 ns      8459485
  #MemoryFixture/CacheAccess/2048/0/2/1          166.19624228249646 ns      165.74873814180103 ns      4221552
  #MemoryFixture/CacheAccess/4096/0/2/1           331.7269469760912 ns      330.91601941885546 ns      2111762
  #MemoryFixture/CacheAccess/8192/0/2/1           663.8953077112178 ns       662.4540856828183 ns      1059419
  #MemoryFixture/CacheAccess/16384/0/2/1          1326.843343898467 ns       1323.254749209487 ns       527193
  #MemoryFixture/CacheAccess/32768/0/2/1         2656.6743123958327 ns      2651.2139933123217 ns       264069
  #MemoryFixture/CacheAccess/65536/0/2/1          5316.515245822549 ns       5306.343742646622 ns       131741
  #MemoryFixture/CacheAccess/131072/0/2/1         10623.00981164003 ns      10584.927048634307 ns        66044
  #MemoryFixture/CacheAccess/262144/0/2/1        21210.760294289023 ns      21148.895028460767 ns        33028
  #MemoryFixture/CacheAccess/524288/0/2/1         42522.49017237178 ns       42412.58560628969 ns        16535
  #MemoryFixture/CacheAccess/1048576/0/2/1        86800.15632693251 ns       86501.64057114806 ns         8124
  #MemoryFixture/CacheAccess/2097152/0/2/1       177705.58147680553 ns      177010.52665832458 ns         3995
  #MemoryFixture/CacheAccess/4194304/0/2/1        449051.5944408481 ns        448237.065698044 ns         1583
  #MemoryFixture/CacheAccess/8388608/0/2/1       1389931.2189924652 ns      1386035.8565891527 ns          516
  #MemoryFixture/CacheAccess/16777216/0/2/1       4438020.074999826 ns       4420751.918750021 ns          160
  #MemoryFixture/CacheAccess/33554432/0/2/1     1.730178560976084E7 ns    1.7182623121951293E7 ns           41
  #MemoryFixture/CacheAccess/67108864/0/2/1     5.283456416664952E7 ns     5.258297450000053E7 ns           12
  #MemoryFixture/CacheAccess/64/1/2/1             5.204121573005314 ns       5.179569988739665 ns    135600170
  #MemoryFixture/CacheAccess/128/1/2/1           10.387460007442089 ns      10.354541036512236 ns     67512560
  #MemoryFixture/CacheAccess/256/1/2/1            20.77893771735786 ns      20.727591539055314 ns     33750321
  #MemoryFixture/CacheAccess/512/1/2/1             41.4739992379063 ns      41.315239742240664 ns     16908639
  #MemoryFixture/CacheAccess/1024/1/2/1           82.95454097741914 ns        82.7976946763163 ns      8446970
  #MemoryFixture/CacheAccess/2048/1/2/1          165.86154320354674 ns      165.43862697234525 ns      4233855
  #MemoryFixture/CacheAccess/4096/1/2/1           331.8942415618145 ns      331.28362462222265 ns      2109704
  #MemoryFixture/CacheAccess/8192/1/2/1           663.6968508366361 ns        662.640989545053 ns      1057011
  #MemoryFixture/CacheAccess/16384/1/2/1          1328.002697434852 ns         1325.3893625606 ns       527909
  #MemoryFixture/CacheAccess/32768/1/2/1          2656.826225607798 ns       2651.831997636693 ns       264032
  #MemoryFixture/CacheAccess/65536/1/2/1          5313.403312198365 ns      5296.6562514184625 ns       132178
  #MemoryFixture/CacheAccess/131072/1/2/1        10603.411688232678 ns      10580.430523642488 ns        66152
  #MemoryFixture/CacheAccess/262144/1/2/1         21213.68814698657 ns       21160.64858647625 ns        33038
  #MemoryFixture/CacheAccess/524288/1/2/1         42446.96972817497 ns       42358.49900102921 ns        16517
  #MemoryFixture/CacheAccess/1048576/1/2/1        85427.89922199522 ns       85099.99477267203 ns         8226
  #MemoryFixture/CacheAccess/2097152/1/2/1       179576.28781830988 ns      178747.97179230847 ns         4006
  #MemoryFixture/CacheAccess/4194304/1/2/1        453971.4271099782 ns      453200.38874680526 ns         1564
  #MemoryFixture/CacheAccess/8388608/1/2/1        1413810.749999729 ns      1409767.6830708506 ns          508
  #MemoryFixture/CacheAccess/16777216/1/2/1       4481396.176099637 ns       4463691.635220161 ns          159
  #MemoryFixture/CacheAccess/33554432/1/2/1    1.7363190725006916E7 ns    1.7271956449999947E7 ns           40
  #MemoryFixture/CacheAccess/67108864/1/2/1     5.310257300000861E7 ns     5.283166808333325E7 ns           12
  #MemoryFixture/CacheAccess/64/2/2/1             5.194585073441566 ns       5.169936491706671 ns    135797225
  #MemoryFixture/CacheAccess/128/2/2/1           10.375776978615239 ns      10.351150907177248 ns     67504271
  #MemoryFixture/CacheAccess/256/2/2/1            20.73537619800892 ns      20.682392672592464 ns     33819437
  #MemoryFixture/CacheAccess/512/2/2/1            41.46680809632523 ns       41.35825233901641 ns     16913944
  #MemoryFixture/CacheAccess/1024/2/2/1           82.84606240770246 ns        82.6134204462559 ns      8446202
  #MemoryFixture/CacheAccess/2048/2/2/1          165.87278324509214 ns      165.32429617950757 ns      4232933
  #MemoryFixture/CacheAccess/4096/2/2/1           331.2406082982817 ns       330.5629607515063 ns      2116922
  #MemoryFixture/CacheAccess/8192/2/2/1            663.159315900038 ns       661.6959690484747 ns      1056103
  #MemoryFixture/CacheAccess/16384/2/2/1         1327.5666883524236 ns      1324.3426747207684 ns       528758
  #MemoryFixture/CacheAccess/32768/2/2/1         2654.9809699966722 ns       2648.132907418347 ns       264372
  #MemoryFixture/CacheAccess/65536/2/2/1           5314.47981229803 ns       5303.806613499892 ns       131912
  #MemoryFixture/CacheAccess/131072/2/2/1         10606.19082560071 ns      10585.181869071148 ns        66097
  #MemoryFixture/CacheAccess/262144/2/2/1        21187.819721722273 ns      21154.040502117125 ns        33060
  #MemoryFixture/CacheAccess/524288/2/2/1         42442.62465239758 ns       42311.67198645875 ns        16542
  #MemoryFixture/CacheAccess/1048576/2/2/1        85539.82432104382 ns       85200.15385547924 ns         8248
  #MemoryFixture/CacheAccess/2097152/2/2/1         180928.070870083 ns      180122.69382093282 ns         3965
  #MemoryFixture/CacheAccess/4194304/2/2/1       456790.68648981495 ns       455950.1693600544 ns         1547
  #MemoryFixture/CacheAccess/8388608/2/2/1       1427351.7287124782 ns      1423232.2613861358 ns          505
  #MemoryFixture/CacheAccess/16777216/2/2/1       4513772.829113805 ns       4495839.088607608 ns          158
  #MemoryFixture/CacheAccess/33554432/2/2/1    1.7454686475002743E7 ns    1.7364546800000016E7 ns           40
  #MemoryFixture/CacheAccess/67108864/2/2/1    5.2963768833327174E7 ns     5.266439433333403E7 ns           12

 */
float result = 0;

using android::media::psh_utils::CoreClass;
using android::media::psh_utils::CORE_LITTLE;
using android::media::psh_utils::CORE_MID;
using android::media::psh_utils::CORE_BIG;

enum Direction {
    DIRECTION_FORWARD,
    DIRECTION_BACKWARD,
    DIRECTION_RANDOM,
};

std::string toString(Direction direction) {
    switch (direction) {
        case DIRECTION_FORWARD: return "DIRECTION_FORWARD";
        case DIRECTION_BACKWARD: return "DIRECTION_BACKWARD";
        case DIRECTION_RANDOM: return "DIRECTION_RANDOM";
        default: return "DIRECTION_UNKNOWN";
    }
}

enum Content {
    CONTENT_ZERO,
    CONTENT_RANDOM,
};

std::string toString(Content content) {
    switch (content) {
        case CONTENT_ZERO: return "CONTENT_ZERO";
        case CONTENT_RANDOM: return "CONTENT_RANDOM";
        default: return "CONTENT_UNKNOWN";
    }
}

class MemoryFixture : public android::media::psh_utils::PerformanceFixture {
public:
    void SetUp(benchmark::State& state) override {
        mCount = state.range(0) / (sizeof(uint32_t) + sizeof(float));
        state.SetComplexityN(mCount * 2);  // 2 access per iteration.

        // create src distribution
        mSource.resize(mCount);
        const auto content = static_cast<Content>(state.range(3));
        if (content == CONTENT_RANDOM) {
            std::minstd_rand gen(mCount);
            std::uniform_real_distribution<float> dis(-1.f, 1.f);
            for (size_t i = 0; i < mCount; i++) {
                mSource[i] = dis(gen);
            }
        }

        // create direction
        mIndex.resize(mCount);
        const auto direction = static_cast<Direction>(state.range(2));
        switch (direction) {
            case DIRECTION_BACKWARD:
                for (size_t i = 0; i < mCount; i++) {
                    mIndex[i] = i;  // it is also possible to go in the reverse direction
                }
                break;
            case DIRECTION_FORWARD:
            case DIRECTION_RANDOM:
                for (size_t i = 0; i < mCount; i++) {
                    mIndex[i] = i;  // it is also possible to go in the reverse direction
                }
                if (direction == DIRECTION_RANDOM) {
                    std::random_device rd;
                    std::mt19937 g(rd());
                    std::shuffle(mIndex.begin(), mIndex.end(), g);
                }
                break;
        }

        // set up the profiler
        const auto coreClass = static_cast<CoreClass>(state.range(1));

        // It would be best if we could override SetName() but it is too late at this point,
        // so we set the state label here for clarity.
        state.SetLabel(toString(coreClass).append("/")
            .append(toString(direction)).append("/")
            .append(toString(content)));

        if (property_get_bool("persist.audio.benchmark_profile", false)) {
            startProfiler(coreClass);
        }
    }
    size_t mCount = 0;
    std::vector<uint32_t> mIndex;
    std::vector<float> mSource;
};

BENCHMARK_DEFINE_F(MemoryFixture, CacheAccess)(benchmark::State &state) {
    float accum = 0;
    while (state.KeepRunning()) {
        for (size_t i = 0; i < mCount; ++i) {
            accum += mSource[mIndex[i]];
        }
        benchmark::ClobberMemory();
    }
    result += accum; // not optimized
}

BENCHMARK_REGISTER_F(MemoryFixture, CacheAccess)->ArgsProduct({
    benchmark::CreateRange(64, 64<<20, /* multi = */2),
    {CORE_LITTLE, CORE_MID, CORE_BIG},
    {DIRECTION_FORWARD, DIRECTION_RANDOM},
    {CONTENT_RANDOM},
});

BENCHMARK_MAIN();
