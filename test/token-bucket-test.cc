/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2006 Intel Corporation. All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 *   Neither the name of the Intel Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "util/TokenBucket.h"
#include "util/UnitTest.h"

using namespace oasys;

// to make the test more predictable, we don't actually call the
// system call sleep() or usleep(), but instead spin until for the
// given amount of time has elapsed since the last call to safe_usleep
void safe_usleep(u_int32_t usecs) {
    static Time last;
    if (last.sec_ == 0) {
        last.get_time();
    }

    Time now;
    do {
        now.get_time();
    } while ((now - last).in_microseconds() < usecs);

    last = now;
}

DECLARE_TEST(Fast) {
    // bucket with a depth of 100 tokens and a replacement rate of
    // 10 tokens per ms (i.e. 10000 tokens per second)
    int rate = 10000;
    TokenBucket t("/test/tokenbucket", 100, rate);

    CHECK_EQUAL(t.tokens(), 100);
    CHECK_EQUAL(t.time_to_fill(), 0);
    
    // drain at a constant rate for a while
    for (int i = 0; i < 1000; ++ i) {
        CHECK(t.drain(10));
        safe_usleep(10000);
    }

    // let it fill up
    safe_usleep(1000000);
    t.update();
    CHECK_EQUAL(t.tokens(), 100);
    CHECK_EQUAL(t.time_to_fill(), 0);

    // make sure it doesn't over-fill
    safe_usleep(1000000);
    t.update();
    CHECK_EQUAL(t.tokens(), 100);
    CHECK_EQUAL(t.time_to_fill(), 0);
    
    // fully drain the bucket
    CHECK(t.drain(100));
    CHECK_EQUAL(t.tokens(), 0);

    // it should be able to catch up
    safe_usleep(10000);
    CHECK(t.drain(10));

    safe_usleep(1000);
    CHECK(t.drain(1));

    // fully empty the bucket
    safe_usleep(0);
    DO(t.empty());
    CHECK_EQUAL(t.tokens(), 0);

    // now spend a few seconds hammering on the bucket, making sure we
    // get the rate that we expect
    int nsecs    = 10;
    int interval = 10; // ms
    int total    = 0;
    for (int i = 0; i < (nsecs * (1000 / interval)); ++i) {
        safe_usleep(interval * 1000);
        while (t.drain(10)) {
            total += 10;
        }
        while (t.drain(1)) {
            total ++;
        }
    }

    double fudge = 0.01; // pct
    CHECK_GTU(total, (u_int) ((nsecs * rate) * (1.0 - fudge)));
    CHECK_LTU(total, (u_int) ((nsecs * rate) * (1.0 + fudge)));

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Slow) {
    // bucket with a depth of only one token and a replacement rate of
    // 1 token per second
    TokenBucket t("/test/tokenbucket", 1, 1);
    CHECK_EQUAL(t.tokens(), 1);
    CHECK(t.drain(1));
    CHECK_EQUAL((t.time_to_fill() + 500) / 1000, 1);
    CHECK(! t.drain(1));
    
    // fully empty the bucket
    safe_usleep(0);
    CHECK(t.drain(t.tokens()));
    CHECK_EQUAL(t.tokens(), 0);
    
    // now spend a few seconds hammering on the bucket, making sure we
    // get the rate that we should
    int nsecs    = 10;
    int interval = 10; // ms
    int total    = 0;
    for (int i = 0; i < (nsecs * (1000 / interval)); ++i) {
        safe_usleep(interval * 1000);
        if (t.drain(1)) {
            ++total;
        }
    }

    CHECK_EQUAL(total, nsecs);

    safe_usleep(1000000);
    CHECK(t.drain(1));

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(TimeToFill) {
    TokenBucket t("/test/tokenbucket", 10000, 1000);

    safe_usleep(0);

    CHECK_EQUAL(t.time_to_fill(), 0);

    CHECK(t.drain(5000));
    CHECK_EQUAL((t.time_to_fill() + 500) / 1000, 5);

    safe_usleep(1000000);
    CHECK_EQUAL((t.time_to_fill() + 500) / 1000, 4);

    safe_usleep(1000000);
    CHECK_EQUAL((t.time_to_fill() + 500) / 1000, 3);

    CHECK(t.drain(1000));
    CHECK_EQUAL((t.time_to_fill() + 500) / 1000, 4);

    DO(t.set_rate(100000));
    safe_usleep(0);
    DO(t.empty());

    u_int32_t ms = t.time_to_fill();
    CHECK_EQUAL(ms, 100);
    safe_usleep(101000);

    CHECK_EQUAL(t.time_to_fill(), 0);

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(Test) {
    ADD_TEST(Fast);
    ADD_TEST(Slow);
    ADD_TEST(TimeToFill);
}

DECLARE_TEST_FILE(Test, "token bucket test");