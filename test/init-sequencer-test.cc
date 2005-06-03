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

bool g_done[6] = {false, false, false, false, false,
                  false};

OASYS_DECLARE_INIT_MODULE_0(Step1) { g_done[0] = true; return 0; }
OASYS_DECLARE_INIT_MODULE_0(Step2) { g_done[1] = true; return 0; }
OASYS_DECLARE_INIT_MODULE(Step3, 2, "Step1", "Step2") { g_done[2] = true; return 0; }
OASYS_DECLARE_INIT_MODULE_0(Step4) { g_done[3] = true; return 0; }
OASYS_DECLARE_INIT_MODULE(Step5, 2, "Step3", "Step4") { g_done[4] = true; return 0; }
OASYS_DECLARE_INIT_CONFIG(StepConfig);
OASYS_DECLARE_INIT_MODULE(StepConfigDep, 1, "StepConfig") { g_done[5] = true; return 0; }

DECLARE_TEST(InitSeqTest1) {
    Singleton<InitSequencer> seq;
    seq->start("Step3");    

    CHECK(g_done[0] && g_done[1] && g_done[2]);

    return 0;
}
DECLARE_TEST(InitSeqTest2) {
    Singleton<InitSequencer> seq;
    seq->reset();
    seq->start("Step5");    

    CHECK(g_done[0] && g_done[1] && g_done[2] && g_done[3] && g_done[4]);

    return 0;
}

DECLARE_TEST(InitSeqTest3) {
    Singleton<InitSequencer> seq;
    seq->reset();

    OASYS_INIT_CONFIG_DONE(StepConfig);
    seq->start("StepConfigDep");

    CHECK(g_done[0] && g_done[1] && g_done[2] && g_done[3] && g_done[4] && 
          g_done[5]);

    return 0;    
}

DECLARE_TESTER(InitSeqTest) {
    ADD_TEST(InitSeqTest1);
    ADD_TEST(InitSeqTest2);
    ADD_TEST(InitSeqTest3);
}

DECLARE_TEST_FILE(InitSeqTest, "init sequencer test");
