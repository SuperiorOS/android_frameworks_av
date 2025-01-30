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

#define LOG_TAG "jthread_tests"

#include <mediautils/jthread.h>

#include <gtest/gtest.h>

#include <atomic>

using namespace android::mediautils;

namespace {
TEST(jthread_tests, dtor) {
    std::atomic_int x = 0;
    std::atomic_bool is_stopped = false;
    {
        auto jt = jthread([&](stop_token stok) {
            while (!stok.stop_requested()) {
                if (x.load() < std::numeric_limits<int>::max())
                    x++;
            }
            is_stopped = true;
        });
        while (x.load() < 1000)
            ;
    }
    // Check we triggered a stop on dtor
    ASSERT_TRUE(is_stopped.load());
    // Check we actually ran
    ASSERT_GE(x.load(), 1000);
}
TEST(jthread_tests, request_stop) {
    std::atomic_int x = 0;
    std::atomic_bool is_stopped = false;
    auto jt = jthread([&](stop_token stok) {
        while (!stok.stop_requested()) {
            if (x.load() < std::numeric_limits<int>::max())
                x++;
        }
        is_stopped = true;
    });
    while (x.load() < 1000)
        ;
    // request stop manually
    ASSERT_TRUE(jt.request_stop());
    // busy loop till thread acks
    while (!is_stopped.load())
        ;
    // Check we triggered a stop on dtor
    ASSERT_TRUE(is_stopped.load());
    // Check we actually ran
    ASSERT_GE(x.load(), 1000);
}

}  // namespace
