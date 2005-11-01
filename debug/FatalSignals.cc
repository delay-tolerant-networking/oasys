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

#include "FatalSignals.h"
#include "StackTrace.h"

namespace oasys {

const char* FatalSignals::appname_ = "(unknown app)";

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
    
    signal(sig, SIG_DFL);
    fprintf(stderr, "ERROR: %s (pid %d) got fatal %s - will dump core\n",
            appname_, (int)getpid(), signame);
    StackTrace::print_current_trace(true);
    fflush(stderr);

    // trap-generated signals are automatically redelivered by the OS.
    // SIGABRT and SIGQUIT, however, need to be redelivered
    if (sig == SIGABRT || sig == SIGQUIT) {
        kill(getpid(), sig);
    }
}

} // namespace oasys