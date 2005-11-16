/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2005 Intel Corporation. All rights reserved. 
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

#include <dlfcn.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "FatalSignals.h"
#include "StackTrace.h"
#include "thread/Thread.h"

namespace oasys {

const char* FatalSignals::appname_  = "(unknown app)";
const char* FatalSignals::core_dir_ = NULL;
bool        FatalSignals::in_abort_handler_ = false;

void
FatalSignals::init(const char* appname)
{
    appname_ = appname;
    signal(SIGSEGV, FatalSignals::handler);
    signal(SIGBUS,  FatalSignals::handler);
    signal(SIGILL,  FatalSignals::handler);
    signal(SIGFPE,  FatalSignals::handler);
    signal(SIGABRT, FatalSignals::handler);
    signal(SIGQUIT, FatalSignals::handler);
}

void
FatalSignals::cancel()
{
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS,  SIG_DFL);
    signal(SIGILL,  SIG_DFL);
    signal(SIGFPE,  SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}

void
FatalSignals::handler(int sig)
{
    char* signame = "";
    switch(sig) {
#define FATAL(_s) case _s: signame = #_s; break;

    FATAL(SIGSEGV);
    FATAL(SIGBUS);
    FATAL(SIGILL);
    FATAL(SIGFPE);
    FATAL(SIGABRT);
    FATAL(SIGQUIT);

    default:
        char buf[1024];
        snprintf(buf, sizeof(buf), "ERROR: UNEXPECTED FATAL SIGNAL %d\n", sig);
        exit(1);
    };
    
    fprintf(stderr, "ERROR: %s (pid %d) got fatal %s - will dump core\n",
            appname_, (int)getpid(), signame);

    // make sure we're in the right directory
    if (!in_abort_handler_ && core_dir_ != NULL) {
        fprintf(stderr, "fatal handler chdir'ing to core dir '%s'\n",
                core_dir_);
        chdir(core_dir_);   
    }

    StackTrace::print_current_trace(true);
    fflush(stderr);

    // trap-generated signals are automatically redelivered by the OS,
    // so we restore the default handler below.
    //
    // SIGABRT and SIGQUIT, however, need to be redelivered. but, we
    // really would like to have stack traces from all threads, so we
    // first set a flag indicating that we've started the process,
    // then try to deliver the same signal to all the other threads to
    // try to get more stack traces
    if (sig == SIGABRT || sig == SIGQUIT) {
        if (! in_abort_handler_) {
            in_abort_handler_ = true;

            Thread::IDArray& ids = Thread::all_thread_ids_;
            for (size_t i = 0; i < ids.size(); ++i) {
                if (ids[i] != 0 && ids[i] != Thread::current()) {
                    fprintf(stderr,
                            "fatal handler sending signal to thread %u\n",
                            ids[i]);
                    pthread_kill(ids[i], sig);
                    sleep(1);
                }
            }

            fprintf(stderr, "fatal handler dumping core\n");
            signal(sig, SIG_DFL);
            kill(getpid(), sig);
        }
    } else {
        signal(sig, SIG_DFL);
    }
}

void
FatalSignals::die()
{
    Breaker::break_here();
    StackTrace::print_current_trace(false);
    if (core_dir_ != NULL) {
        fprintf(stderr, "fatal handler chdir'ing to core dir '%s'\n",
                core_dir_);
        chdir(core_dir_);
    }

    cancel();
    ::abort();
}


} // namespace oasys
