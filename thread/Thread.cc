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

#include "config.h"
#include "Thread.h"

#include "../debug/DebugUtils.h"
#include "../debug/Log.h"
#include "../util/CString.h"

#if GOOGLE_PROFILE_ENABLED
#include <google/profiler.h>
#endif

namespace oasys {

#ifdef __win32__
__declspec(thread) Thread* Thread::current_thread_ = 0;
#else
bool                 Thread::signals_inited_ = false;
sigset_t             Thread::interrupt_sigset_;
#endif

bool                 Thread::start_barrier_enabled_ = false;
std::vector<Thread*> Thread::threads_in_barrier_;
Thread::IDArray      Thread::all_thread_ids_;

//----------------------------------------------------------------------------
void
Thread::activate_start_barrier()
{
    start_barrier_enabled_ = true;
    log_debug("/thread", "activating thread creation barrier");
}

//----------------------------------------------------------------------------
void
Thread::release_start_barrier()
{
    start_barrier_enabled_ = false;

    log_debug("/thread",
              "releasing thread creation barrier -- %zu queued threads",
              threads_in_barrier_.size());
    
    for (size_t i = 0; i < threads_in_barrier_.size(); ++i) 
    {
        Thread* thr = threads_in_barrier_[i];
        thr->start();
    }

    threads_in_barrier_.clear();
}

//----------------------------------------------------------------------------
bool
Thread::id_equal(ThreadId_t a, ThreadId_t b)
{
#ifdef __win32__
    // XXX/bowei - check if comparing handles for equality is ok
    return a == b;
#else
    return pthread_equal(a, b);
#endif
}

//----------------------------------------------------------------------------
Thread::Thread(const char* name, int flags)
    : flags_(flags)
{
    if ((flags & CREATE_JOINABLE) && (flags & DELETE_ON_EXIT)) {
        flags &= ~DELETE_ON_EXIT;
    }

#ifdef __win32__
    if (flags & CREATE_JOINABLE) {
        join_event_ = CreateEvent(0, TRUE, FALSE, 0);
        ASSERT(join_event_ != 0);
    } else {
        join_event_ = 0;
    }
#endif //__win32__
    cstring_copy(name_, 64, name);
    thread_id_ = 0;
}

//----------------------------------------------------------------------------
Thread::~Thread()
{
#ifdef __win32__
    if (join_event_ != 0) {
        CloseHandle(join_event_);
    }
#endif //__win32__    
}

//----------------------------------------------------------------------------
void
Thread::start()
{
#ifndef __win32__

    // if this is the first thread, set up signals
    if (!signals_inited_) 
    {
        sigemptyset(&interrupt_sigset_);
        sigaddset(&interrupt_sigset_, INTERRUPT_SIG);
        signal(INTERRUPT_SIG, interrupt_signal);
        siginterrupt(INTERRUPT_SIG, 1);
        signals_inited_ = true;
    }

#endif

    // check if the creation barrier is enabled
    if (start_barrier_enabled_) 
    {
        log_debug("/thread", "delaying start of thread %p due to barrier",
                  this);
        threads_in_barrier_.push_back(this);
        return;
    }

    log_debug("/thread", "starting thread %p", this);

#ifdef __win32__

    DWORD  thread_dword_id;
    HANDLE h_thread = CreateThread(0, // Security attributes
                                   0, // default stack size
                                   Thread::pre_thread_run, // thread function
                                   this,
                                   // start suspended to fix some state in
                                   // the Thread* class before it starts running
                                   CREATE_SUSPENDED,       
                                   &thread_dword_id);
    
    ASSERT(h_thread != 0);
    thread_id_ = thread_dword_id;

    // Fix the handle on the thread
    ResumeThread(h_thread);
#else 

    // XXX/bowei - Why do we retry so many times? Shouldn't we just
    // give up?
    int ntries = 0;
    while (pthread_create(&thread_id_, 0, Thread::pre_thread_run, this) != 0) 
    {
        if (++ntries == 10000) {
            PANIC("maximum thread creation attempts");
#ifdef OASYS_DEBUG_MEMORY_ENABLED
            DbgMemInfo::debug_dump();
#endif
        }
        
        log_err("/thread", "error in thread_id_create: %s, retrying",
                strerror(errno));
    }

    // since in most cases we want detached threads, users must
    // explicitly request for them to be joinable
    if (! (flags_ & CREATE_JOINABLE)) 
    {
	pthread_detach(thread_id_);
    }

#endif // __win32__
}

//----------------------------------------------------------------------------
void
Thread::join()
{
    if (! (flags_ & CREATE_JOINABLE)) 
    {
        PANIC("tried to join a thread that isn't joinable -- "
              "need CREATE_JOINABLE flag");
    }
    
#ifdef __win32__
    ASSERT(join_event_ != 0);
    DWORD ret = WaitForSingleObject(join_event_, INFINITE);
    (void)ret; // XXX/bowei - get rid of me
    ASSERT(ret != WAIT_FAILED);
#else
    void* ignored;
    int err;
    if ((err = pthread_join(thread_id_, &ignored)) != 0) 
    {
        PANIC("error in pthread_join: %s", strerror(err));
    }
#endif
}

//----------------------------------------------------------------------------
void
Thread::kill(int sig)
{
#ifdef __win32__
    (void)sig;
    NOTIMPLEMENTED;
#else
    if (pthread_kill(thread_id_, sig) != 0) {
        PANIC("error in pthread_kill: %s", strerror(errno));
    }
#endif
}

//----------------------------------------------------------------------------
void
Thread::interrupt()
{
#ifdef __win32__
    NOTIMPLEMENTED;
#else
    log_debug("/thread", "interrupting thread %p", this);
    kill(INTERRUPT_SIG);
#endif
}

//----------------------------------------------------------------------------
void
Thread::set_interruptable(bool interruptable)
{
#ifdef __win32__
    (void)interruptable;
    NOTIMPLEMENTED;
#else
    ASSERT(Thread::current() == thread_id_);
    
    int block = (interruptable ? SIG_UNBLOCK : SIG_BLOCK);
    if (pthread_sigmask(block, &interrupt_sigset_, NULL) != 0) {
        PANIC("error in thread_id_sigmask");
    }
#endif
}


//----------------------------------------------------------------------------
#ifdef __win32__
DWORD WINAPI 
#else
void*
#endif
Thread::pre_thread_run(void* t)
{
    Thread* thr = static_cast<Thread*>(t);

#ifdef __win32__
    current_thread_ = thr;
#endif

    ThreadId_t thread_id = Thread::current();
    thr->thread_run(thread_id);

    return 0;
}

//----------------------------------------------------------------------------
void
Thread::interrupt_signal(int sig)
{
    (void)sig;
}

//----------------------------------------------------------------------------
void
Thread::thread_run(ThreadId_t thread_id)
{
#if GOOGLE_PROFILE_ENABLED
    ProfilerRegisterThread();
#endif
    
    all_thread_ids_.insert(thread_id);

#ifndef __win32__    
    /*
     * There's a potential race between the starting of the new thread
     * and the storing of the thread id in the thread_id_ member
     * variable, so we can't trust that it's been written by our
     * spawner. So we re-write it here to make sure that it's valid
     * for the new thread (specifically for set_interruptable's
     * assertion.
     */
    thread_id_ = thread_id;
    set_interruptable((flags_ & INTERRUPTABLE));
#endif

    flags_ |= STARTED;
    flags_ &= ~STOPPED;
    flags_ &= ~SHOULD_STOP;

    try 
    {
        run();
    } 
    catch (...) 
    {
        PANIC("unexpected exception caught from Thread::run");
    }
    
    flags_ |= STOPPED;

#ifdef __win32__
    if (join_event_) {
        SetEvent(join_event_);
    }
#endif //__win32__

    if (flags_ & DELETE_ON_EXIT) 
    {
        delete this;
    }

    all_thread_ids_.remove(thread_id_);
    
#ifdef __win32__

    // Make sure C++ cleanup is called, which does not occur if we
    // call ExitThread().
    return;

#else

    pthread_exit(0);
    NOTREACHED;

#endif
}

} // namespace oasys
