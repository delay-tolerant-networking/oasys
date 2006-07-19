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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/poll.h>

#include "Timer.h"
#include "io/IO.h"
#include "../util/InitSequencer.h"

namespace oasys {

template <> TimerSystem* Singleton<TimerSystem>::instance_ = 0;

//----------------------------------------------------------------------
TimerSystem::TimerSystem()
    : Logger("TimerSystem", "/timer"),
      system_lock_(new SpinLock()),
      notifier_(logpath_),
      timers_()
{
    memset(handlers_, 0, sizeof(handlers_));
    memset(signals_, 0, sizeof(signals_));
    sigfired_ = false;
}

//----------------------------------------------------------------------
void
TimerSystem::schedule_at(struct timeval *when, Timer* timer)
{
    ScopeLock l(system_lock_, "TimerSystem::schedule_at");
    
    struct timeval now;
    
    if (when == 0) {
        // special case a NULL timeval as an immediate timer
        log_debug("scheduling timer %p immediately", timer);
        
        timer->when_.tv_sec = 0;
        timer->when_.tv_usec = 0;
    } else {
        ::gettimeofday(&now, 0);
        log_debug("scheduling timer %p in %ld ms at %u:%u",
                  timer, TIMEVAL_DIFF_MSEC(*when, now),
                  (u_int)when->tv_sec, (u_int)when->tv_usec);
        
        timer->when_ = *when;
    }
    
    if (timer->pending_) {
        // XXX/demmer this could scan through the heap, find the right
        // timer and re-sort the heap, but it seems better to just
        // expose a new "reschedule" api call to make it explicit that
        // it's a different operation.
        PANIC("rescheduling timers not implemented");
    }
    
    timer->pending_ = 1;
    timer->cancelled_ = 0;
    timers_.push(timer);

    notifier_.notify();
}

//----------------------------------------------------------------------
void
TimerSystem::schedule_in(int milliseconds, Timer* timer)
{
    struct timeval when;
    ::gettimeofday(&when, 0);
    when.tv_sec += milliseconds / 1000;
    when.tv_usec += (milliseconds % 1000) * 1000;
    while (when.tv_usec > 1000000) {
        when.tv_sec += 1;
        when.tv_usec -= 1000000;
    }
    
    return schedule_at(&when, timer);
}

//----------------------------------------------------------------------
void
TimerSystem::schedule_immediate(Timer* timer)
{
    return schedule_at(0, timer);
}

//----------------------------------------------------------------------
bool
TimerSystem::cancel(Timer* timer)
{
    ScopeLock l(system_lock_, "TimerSystem::cancel");

    // There's no good way to get a timer out of a heap, so we let it
    // stay in there and mark it as cancelled so when it bubbles to
    // the top, we don't bother with it. This makes rescheduling a
    // single timer instance tricky...
    if (timer->pending_) {
        timer->cancelled_ = true;
        return true;
    }
    
    return false;
}

//----------------------------------------------------------------------
void
TimerSystem::post_signal(int sig)
{
    TimerSystem* _this = TimerSystem::instance();

    _this->sigfired_ = true;
    _this->signals_[sig] = true;
    
    _this->notifier_.notify();
}

//----------------------------------------------------------------------
void
TimerSystem::add_sighandler(int sig, __sighandler_t handler)
{
    log_debug("adding signal handler 0x%x for signal %d",
              (u_int)handler, sig);
    handlers_[sig] = handler;
    signal(sig, post_signal);
}

//----------------------------------------------------------------------
void
TimerSystem::pop_timer(const struct timeval& now)
{
    ASSERT(system_lock_->is_locked_by_me());
    
    Timer* next_timer = timers_.top();
    timers_.pop();

    // clear the pending bit since it could get rescheduled 
    ASSERT(next_timer->pending_);
    next_timer->pending_ = 0;

    int late = TIMEVAL_DIFF_MSEC(now, next_timer->when());
    if (late > 2000) {
        log_warn("timer thread running slow -- timer is %d msecs late", late);
    }
        
    if (! next_timer->cancelled_) {
        log_debug("popping timer %p at %u.%u", next_timer,
                  (u_int)now.tv_sec, (u_int)now.tv_usec);
        next_timer->timeout(now);
    } else {
        log_debug("popping cancelled timer %p at %u.%u", next_timer,
                  (u_int)now.tv_sec, (u_int)now.tv_usec);
        next_timer->cancelled_ = 0;
        
        if (next_timer->cancel_flags_ == Timer::DELETE_ON_CANCEL) {
            log_debug("deleting cancelled timer %p at %u.%u", next_timer,
                      (u_int)now.tv_sec, (u_int)now.tv_usec);
            delete next_timer;
        }
    }
}

//----------------------------------------------------------------------
void
TimerSystem::handle_signals()
{        
    // KNOWN ISSUE: if a second signal is received before the first is
    // handled it is ignored, i.e. sending signal gives at-least-once
    // semantics, not exactly-once semantics
    if (sigfired_) {
        sigfired_ = 0;
        
        log_debug("sigfired_ set, calling registered handlers");
        for (int i = 0; i < NSIG; ++i) {
            if (signals_[i]) {
                handlers_[i](i);
                signals_[i] = 0;
            }
        }
    }
}

//----------------------------------------------------------------------
int
TimerSystem::run_expired_timers()
{
    ScopeLock l(system_lock_, "TimerSystem::run_expired_timers");
    
    handle_signals();
    
    struct timeval now;    
    while (! timers_.empty()) 
    {
        if (::gettimeofday(&now, 0) != 0) {
            PANIC("gettimeofday");
        }

        Timer* next_timer = timers_.top();
        if (TIMEVAL_LT(now, next_timer->when_)) {
            int diff_ms = TIMEVAL_DIFF_MSEC(next_timer->when_, now);
            ASSERT(diff_ms >= 0);
            
            // there's a chance that we're within a millisecond of the
            // time to pop, but still not at the right time. in this
            // case case we don't return 0, but fall through to pop
            // the timer
            if (diff_ms != 0) {
                log_debug("new timeout %d", diff_ms);
                return diff_ms;
            } else {
                log_debug("sub-millisecond difference found, falling through");
            }
        }
        pop_timer(now);
    }

    return -1;
}

//----------------------------------------------------------------------
void
TimerThread::run()
{
    TimerSystem* sys = TimerSystem::instance();
    while (true) 
    {
        int timeout = sys->run_expired_timers();
        sys->notifier()->wait(NULL, timeout);
    }

    NOTREACHED;
}

//----------------------------------------------------------------------
void
TimerThread::init()
{
    ASSERT(instance_ == NULL);
    instance_ = new TimerThread();
    instance_->start();
}

TimerThread* TimerThread::instance_ = NULL;

} // namespace oasys
