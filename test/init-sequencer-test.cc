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

#include "util/UnitTest.h"
#include "util/InitSequencer.h"
#include "util/Singleton.h"

using namespace oasys;

class Step1 : public InitStep, public Logger {
public:
    Step1() : InitStep("step1"), 
              Logger("/test/step1") {}

    int run_component() {
        log_info("running");

        return 0;
    }
       
};

class Step2 : public InitStep, public Logger {
public:
    Step2() : InitStep("step2", 1, "step1"), 
              Logger("/test/step2") {}

    int run_component() {
        log_info("running");
        return 0;
    }
       
};

class Step3 : public InitStep, public Logger {
public:
    Step3() : InitStep("step3", 2, "step1", "step2"), 
              Logger("/test/step3") {}

    int run_component() {
        log_info("running");
        return 0;
    }
       
};

class Step4 : public InitStep, public Logger {
public:
    Step4() : InitStep("step4"), 
              Logger("/test/step4") {}

    int run_component() {
        log_info("running");
        return 0;
    }
       
};

class Step5 : public InitStep, public Logger {
public:
    Step5() : InitStep("step5", 2, "step4", "step2"), 
              Logger("/test/step5") {}

    int run_component() {
        log_info("running");
        return 0;
    }
       
};

Step1 s1;
Step2 s2;
Step3 s3;
Step4 s4;
Step5 s5;

DECLARE_TEST(InitSeqTest1) {
    Singleton<InitSequencer>::instance()->start("step3");    
    return 0;
}
DECLARE_TEST(InitSeqTest2) {
    Singleton<InitSequencer>::instance()->start("step5");    
    return 0;
}

DECLARE_TESTER(InitSeqTest) {
    ADD_TEST(InitSeqTest1);
    ADD_TEST(InitSeqTest2);
}

DECLARE_TEST_FILE(InitSeqTest, "init sequencer test");
