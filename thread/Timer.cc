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

namespace oasys {

TimerSystem* TimerSystem::instance_;

TimerSystem::TimerSystem()
    : Thread(Thread::INTERRUPTABLE),
      Logger("/timer"),
      system_lock_(new SpinLock()),
      signal_("/timer/signal"),
      timers_()
{

    memset(handlers_, 0, sizeof(handlers_));
    memset(signals_, 0, sizeof(signals_));
    sigfired_ = false;
}

void
TimerSystem::init()
{
    instance_ = new TimerSystem();
    instance_->start();
}

void
TimerSystem::schedule_at(struct timeval *when, Timer* timer)
{
    ScopeLock l(system_lock_);
    
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
        
    } else {
        timer->pending_ = 1;
        timer->cancelled_ = 0;
        timers_.push(timer);
    }

    // notify the timer thread (which is likely blocked in poll) that
    // we've stuck something new on the queue
    signal_.notify();
}

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

void
TimerSystem::schedule_immediate(Timer* timer)
{
    return schedule_at(0, timer);
}

bool
TimerSystem::cancel(Timer* timer)
{
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

/*
 * Pop the timer from the head of the heap and call its handler.
 */
void
TimerSystem::pop_timer(struct timeval* now)
{
    ASSERT(system_lock_->is_locked_by_me());
    
    Timer* next_timer = timers_.top();
    timers_.pop();

    // clear the pending bit since it could get rescheduled 
    ASSERT(next_timer->pending_);
    next_timer->pending_ = 0;
    
    // call the handler if it wasn't cancelled, otherwise delete the
    // timer object if asked
    if (! next_timer->cancelled_) {
        log_debug("popping timer at %u.%u",
                  (u_int)now->tv_sec, (u_int)now->tv_usec);
        next_timer->timeout(now);
    } else {
        log_debug("popping cancelled timer at %u.%u",
                  (u_int)now->tv_sec, (u_int)now->tv_usec);
        next_timer->cancelled_ = 0;
        if (next_timer->delete_on_cancel_) {
            log_debug("deleting cancelled timer at %u.%u",
                      (u_int)now->tv_sec, (u_int)now->tv_usec);
            delete next_timer;
        }
    }
}

/**
 * Hook called from an the actual signal handler that notifies the
 * timer sysetem thread to call the signal handler function.
 *
 * By setting the bit to indicate that the signal fired and calling
 * the thread's interrupt routine, we make sure to wake up the timer
 * thread if it's blocked in poll, causing it to actually execute the
 * signal handler.
 */
void
TimerSystem::post_signal(int sig)
{
    TimerSystem* _this = TimerSystem::instance();

    _this->sigfired_ = true;
    _this->signals_[sig] = true;
    _this->interrupt();
}

/**
 * Hook to use the timer thread to safely handle a signal.
 */
void
TimerSystem::add_sighandler(int sig, __sighandler_t handler)
{
    log_debug("adding signal handler 0x%x for signal %d",
              (u_int)handler, sig);
    handlers_[sig] = handler;
    signal(sig, post_signal);
}

/**
 * The main system thread function that blocks in poll() waiting for
 * either the first timer to fire or for another thread to schedule a
 * timer.
 */
void
TimerSystem::run()
{
    int timeout;
    struct timeval now;
    Timer* next_timer;
    
    system_lock_->lock();
        
    while (1) {
        timeout = -1; // default is to block forever

        if (sigfired_) {
            log_debug("sigfired_ set, calling registered handlers");
            for (int i = 0; i < NSIG; ++i) {
                if (signals_[i]) {
                    handlers_[i](i);
                    signals_[i] = 0;
                }
            }
            sigfired_ = 0;
        }

        if (! timers_.empty()) {
            ::gettimeofday(&now, 0);
            next_timer = timers_.top();
            
            timeout = TIMEVAL_DIFF_MSEC(next_timer->when_, now);

            // handle any immediate timers immediately, then re-loop
            if ((next_timer->when_.tv_sec == 0 &&
                 next_timer->when_.tv_usec == 0) ||
                (timeout == 0))
            {
                pop_timer(&now);
                continue;
            }

            
            if (timeout < 0) {
                log_warn("timer in the past, calling immediately");
                pop_timer(&now);
                continue;
            }

            ASSERT(timeout > 0);
        }

        // unlock the lock before calling poll
        system_lock_->unlock();
        int cc = IO::poll(signal_.read_fd(), POLLIN, NULL, timeout, logpath_);

        // and re-take the lock on return
        system_lock_->lock();

        if (cc == -1 && errno == EINTR) {
            log_debug("poll interrupted, looping");
        }
        
        if (cc == 0) {
            log_debug("poll returned due to timeout");
            
        } else if (cc == 1) {
            log_debug("poll returned due to signal");
            signal_.drain_pipe();
        } else {
            if (errno == EINTR) continue;
            log_err("unexpected return of %d from poll errno %d", cc, errno);
            continue; // XXX/demmer ??
        }
        
        // now check for any ready timers
	::gettimeofday(&now, NULL);
        while (! timers_.empty()) {
            next_timer = timers_.top();
	    if (TIMEVAL_LT(now, next_timer->when_)) {
                break;
	    }
            
            pop_timer(&now);
        }
    }
    // don't ever return
}

} // namespace oasys
