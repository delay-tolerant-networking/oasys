/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2004 Intel Corporation. All rights reserved. 
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

#include <stdio.h>
#include <unistd.h>
#include <thread/Timer.h>

#include "debug/Log.h"
#include "util/UnitTest.h"

using namespace oasys;

class PeriodicTimer : public Timer {
  public:
    PeriodicTimer(const char* name) {
        snprintf(log_, sizeof(log_), "/timer/%s", name);
        logf(log_, LOG_DEBUG, "PeriodicTimer 0x%x", (u_int)this);
    }
    
    void timeout(const struct timeval& now) {
        int late = TIMEVAL_DIFF_USEC(now, when());
        log_notice(log_, "timer at %ld.%ld (%d usec late)",
                   (long unsigned int)now.tv_sec, (long unsigned int)now.tv_usec,
                   late);
        
        reschedule();
    }
    
    virtual void reschedule() = 0;
    
  protected:
    char log_[64];
};

class TenSecondTimer : public PeriodicTimer {
  public:
    TenSecondTimer() : PeriodicTimer("10sec") { reschedule(); }
    void reschedule() { schedule_in(10000); }
};
               
class OneSecondTimer : public PeriodicTimer {
  public:
    OneSecondTimer() : PeriodicTimer("1sec") { reschedule(); }
    void reschedule() { schedule_in(1000); }
};
               
class HalfSecondTimer : public PeriodicTimer {
  public:
    HalfSecondTimer() : PeriodicTimer("500msec") { reschedule(); }
    void reschedule() { schedule_in(500); }
};

class TenImmediateTimer : public PeriodicTimer {
public:
    TenImmediateTimer() : PeriodicTimer("10immed")
    {
        count_ = 0;
        reschedule();
    }
    void reschedule() {
        if (count_ == 0) {
            count_ = 10;
            schedule_in(1000);
        } else {
            count_--;
            schedule_in(1);
        }
    }
protected:
    int count_;
};

DECLARE_TEST(TimerTest1) {
    TimerSystem::instance()->start();

    new OneSecondTimer();

    new TenSecondTimer();
    new OneSecondTimer();
    new HalfSecondTimer();

    usleep(500000);
    new OneSecondTimer();

    new HalfSecondTimer();
    usleep(100000);
    new HalfSecondTimer();
    usleep(100000);
    new HalfSecondTimer();
    usleep(100000);
    new HalfSecondTimer();

    new TenImmediateTimer();

    sleep(10);    

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(TimerTest) {
    ADD_TEST(TimerTest1);
}

DECLARE_TEST_FILE(TimerTest, "timer test");

#ifdef DECLARE_TEST_TCL
proc checkTimerTest1 {output} {
    while {[set line [gets $output]] != ""} {
        if [regexp {\(-?([0-9]+) usec late\)} $line match diff] {
            # check for alarms that are off by 20 msec.
            if [expr $diff > 20000] {
                puts "Timer off by more than 20000 usec"
                return -1
            }
        }
    }

    return 0
}
#endif

