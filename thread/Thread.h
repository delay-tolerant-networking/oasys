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

#ifndef _OASYS_THREAD_H_
#define _OASYS_THREAD_H_

#include "config.h"

#include <sys/types.h>
#include <pthread.h>
#include <vector>

#ifdef HAVE_SCHED_YIELD
#include <sched.h>
#endif

#include <signal.h>
#include "../debug/DebugUtils.h"
#include "SafeArray.h"

namespace oasys {

/**
 * Class to wrap a thread of execution using pthreads. Similar to the
 * Java API.
 */
class Thread {
public:
    /**
     * Bit values for thread flags.
     */
    enum thread_flags_t {
        CREATE_JOINABLE	= 1 << 0,	///< inverse of PTHREAD_CREATE_DETACHED
        DELETE_ON_EXIT  = 1 << 1,	///< delete thread when run() completes
        INTERRUPTABLE   = 1 << 2,	///< thread can be interrupted
        SHOULD_STOP   	= 1 << 3,	///< bit to signal the thread to stop
        STOPPED   	= 1 << 4,	///< bit indicating the thread has stopped
    };

    /**
     * Enum to define signals used internally in the thread system
     * (currently just the interrupt signal).
     */
    enum thread_signal_t {
        INTERRUPT_SIG = SIGURG
    };

    /**
     * Constructor / Destructor
     */
    Thread(const char* name, int flags = 0);
    virtual ~Thread();
    
    /**
     * Starts a new thread and calls the virtual run() method with the
     * new thread.
     */
    void start();

    /**
     * Join with this thread, blocking the caller until we quit.
     */
    void join();

    /**
     * Activate the thread creation barrier. No new threads will be
     * created until release_start_barrier() is called.
     *
     * Note this should only be called in a single-threaded context,
     * i.e. during initialization.
     */
    static void activate_start_barrier();
    
    /**
     * Release the thread creation barrier and actually start up any
     * pending threads.
     */
    static void release_start_barrier();
    
    /**
     * @return Status of start barrier
     */
    static bool start_barrier_enabled() { 
        return start_barrier_enabled_;
    }

    /**
     * Yield the current process. Needed for capriccio to yield the
     * processor during long portions of code with no I/O. (Probably
     * used for testing only).
     */
    static void yield();

    /**
     * Potentially yield the current thread to implement a busy
     * waiting function.
     *
     * The implementation is dependant on the configuration. On an
     * SMP, don't do anything. Otherwise, with capriccio threads, this
     * should never happen, so issue an assertion. Under pthreads,
     * however, actually call thread_yield() to give up the processor.
     */
    static void spin_yield();

    /**
     * Return a pointer to the currently running thread.
     *
     * XXX/demmer this could keep a map to return a Thread* if it's
     * actually useful
     */
    static pthread_t current();

    /**
     * Send a signal to the thread.
     */
    void kill(int sig);

    /**
     * Send an interrupt signal to the thread to unblock it if it's
     * stuck in a system call. Implemented with SIGUSR1.
     */
    void interrupt();

    /**
     * Set the interruptable state of the thread (default off). If
     * interruptable, a thread can receive SIGUSR1 signals to
     * interrupt any system calls.
     *
     * Note: This must be called by the thread itself. To set the
     * interruptable state before a thread is created, use the
     * INTERRUPTABLE flag.
     */
    void set_interruptable(bool interruptable);
    
    /**
     * Set a bit to stop the thread
     *
     * This simply sets a bit in the thread's flags that can be
     * examined by the run method to indicate that the thread should
     * be stopped.
     */
    void set_should_stop() { flags_ |= SHOULD_STOP; }

    /**
     * Return whether or not the thread should stop.
     */
    bool should_stop() { return ((flags_ & SHOULD_STOP) != 0); }
    
    /**
     * Return true if the thread has stopped.
     */
    bool is_stopped() { return ((flags_ & STOPPED) != 0); }

    /**
     * Set the given thread flag.
     */
    void set_flag(thread_flags_t flag) { flags_ |= flag; }

    /**
     * Clear the given thread flag.
     */
    void clear_flag(thread_flags_t flag) { flags_ &= ~flag; }
    
    /**
     * Return a pointer to the Thread object's id. Note that this may
     * or may not be the currently executing thread.
     */
    pthread_t thread_id()
    {
        return pthread_;
    }
    
    /**
     * Probe the CPU for the number of CPU's to make the
     * implementation behavior more efficient depending on whether the
     * system is running a SMP or single CPU
     */
    static void check_for_SMP();
    
    /**
     * Array of all live pthread ids. Used for debugging, see
     * FatalSignals.cc.
     */
    typedef SafeArray<256, pthread_t, NULL> IDArray;
    static IDArray all_thread_ids_;
    
protected:
    /**
     * Derived classes should implement this function which will get
     * called in the new Thread context.
     */
    virtual void run() = 0;

    static void* pre_thread_run(void* t);
    void thread_run(const char* thread_name, pthread_t thread_id);
    static void interrupt_signal(int sig);

    pthread_t pthread_;
    int       flags_;

    static bool     signals_inited_;
    static sigset_t interrupt_sigset_;
    
    static bool                 start_barrier_enabled_;
    static std::vector<Thread*> threads_in_barrier_;

    const char* name_;

};

inline pthread_t
Thread::current()
{
    return pthread_self();
}

inline void
Thread::yield()
{
#ifdef HAVE_PTHREAD_YIELD
    pthread_yield();
#elif  HAVE_SCHED_YIELD
    sched_yield();
#else
#error MUST EITHER HAVE PTHREAD_YIELD OR SCHED_YIELD
#endif
}

inline void
Thread::spin_yield()
{
    // XXX/bowei: 
    // 1-p call yield()
    // o.w. spin
    Thread::yield();
}

} // namespace oasys

#endif /* _OASYS_THREAD_H_ */
