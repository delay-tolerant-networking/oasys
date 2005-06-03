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

#include <errno.h>

#include "Thread.h"
#include "debug/Debug.h"
#include "debug/Log.h"
#include "util/InitSequencer.h"
#include "memory/Memory.h"

namespace oasys {

bool Thread::signals_inited_ = false;
sigset_t Thread::interrupt_sigset_;
bool Thread::start_barrier_enabled_ = false;
std::vector<Thread*> Thread::threads_in_barrier_;

OASYS_DECLARE_INIT_MODULE_0(oasys, ThreadStartCreateBarrier) {
    oasys::Thread::activate_start_barrier();
    return 0;
}

OASYS_DECLARE_INIT_MODULE_1(oasys, ThreadReleaseCreateBarrier, 
                            "oasys::ThreadStartCreateBarrier") 
{
    oasys::Thread::release_start_barrier();
    return 0;
}

void
Thread::interrupt_signal(int sig)
{
}

void*
Thread::thread_run(void* t)
{
    Thread* thr = (Thread*)t;

    /*
     * There's a potential race between the starting of the new thread
     * and the storing of the thread id in the pthread_ member
     * variable, so we can't trust that it's been written by our
     * spawner. So we re-write it here to make sure that it's valid
     * for the new thread (specifically for set_interruptable's
     * assertion.
     */
    thr->pthread_ = Thread::current();
    
    thr->set_interruptable((thr->flags_ & INTERRUPTABLE));

    thr->flags_ &= ~STOPPED;
    thr->flags_ &= ~SHOULD_STOP;

    try {
        thr->run();
    } catch (...) {
        PANIC("unexpected exception caught from Thread::run");
    }
    
    thr->flags_ |= STOPPED;
    
    if (thr->flags_ & DELETE_ON_EXIT) {
        delete thr;
    }

    pthread_exit(0);

    NOTREACHED;
}

Thread::Thread(int flags)
    : flags_(flags)
{
  pthread_=0;
}

Thread::~Thread()
{
}

void
Thread::activate_start_barrier()
{
    start_barrier_enabled_ = true;

    log_debug("/thread", "activating thread creation barrier");
}

void
Thread::release_start_barrier()
{
    start_barrier_enabled_ = false;

    log_debug("/thread",
              "releasing thread creation barrier -- %u queued threads",
              (u_int)threads_in_barrier_.size());
    
    for (size_t i = 0; i < threads_in_barrier_.size(); ++i) {
        Thread* thr = threads_in_barrier_[i];
        thr->start();
    }

    threads_in_barrier_.clear();
}

void
Thread::start()
{
    // if this is the first thread, set up signals
    if (!signals_inited_) {
        sigemptyset(&interrupt_sigset_);
        sigaddset(&interrupt_sigset_, SIGURG);
        signal(SIGURG, interrupt_signal);
        siginterrupt(SIGURG, 1);
        signals_inited_ = true;
    }

    // check if the creation barrier is enabled
    if (start_barrier_enabled_) {
        log_debug("/thread", "delaying start of thread %p due to barrier",
                  this);
        threads_in_barrier_.push_back(this);
        return;
    }

    log_debug("/thread", "starting thread %p", this);
    
    int ntries = 0;
    while (pthread_create(&pthread_, 0, Thread::thread_run, this) != 0) {
        if (++ntries == 10000) {
            PANIC("maximum thread creation attempts");
#ifdef OASYS_DEBUG_MEMORY_ENABLED
            DbgMemInfo::debug_dump();
#endif
        }
        
        log_err("/thread", "error in pthread_create: %s, retrying",
                strerror(errno));
    }

    // since in most cases we want detached threads, users must
    // explicitly request for them to be joinable
    if (! (flags_ & CREATE_JOINABLE)) {
	pthread_detach(pthread_);
    }
}

void
Thread::join()
{
    if (! (flags_ & CREATE_JOINABLE)) {
        PANIC("tried to join a thread that isn't joinable -- "
              "need CREATE_JOINABLE flag");
    }
    
    void* ignored;
    if (pthread_join(pthread_, &ignored) != 0) {
        PANIC("error in pthread_join");
    }
}

void
Thread::kill(int sig)
{
    if (pthread_kill(pthread_, sig) != 0) {
        PANIC("error in pthread_kill");
    }
}

void
Thread::interrupt()
{
    kill(SIGURG);
}

void
Thread::set_interruptable(bool interruptable)
{
    ASSERT(Thread::current() == pthread_);
    
    int block = (interruptable ? SIG_UNBLOCK : SIG_BLOCK);
    if (pthread_sigmask(block, &interrupt_sigset_, NULL) != 0) {
        PANIC("error in pthread_sigmask");
    }
}

void
Thread::check_for_SMP()
{
    // XXX/bowei - check /proc/cpuinfo for SMP
}

} // namespace oasys
