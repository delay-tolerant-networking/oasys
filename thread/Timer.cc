/*
 *    Copyright 2004-2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#  include <oasys-config.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <unistd.h>
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
      timers_(),
      seqno_(0),
      num_cancelled_(0)
{
    memset(handlers_, 0, sizeof(handlers_));
    memset(signals_, 0, sizeof(signals_));
    sigfired_ = false;
}

//----------------------------------------------------------------------
TimerSystem::~TimerSystem()
{
    while (! timers_.empty()) {
        Timer* t = timers_.top();
        t->pending_ = false; // to avoid assertion
        timers_.pop();
        delete t;
    }
    delete system_lock_;
    printf("Time system destructor run\n");
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

        ::gettimeofday(&timer->when_, 0);
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
    timer->seqno_ = seqno_++;
    
    timers_.push(timer);

    notifier_.signal();
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
        num_cancelled_++;
        timer->cancelled_ = true;
        return true;
    }
    
    return false;
}

//----------------------------------------------------------------------
size_t
TimerSystem::num_pending_timers()
{
    return timers_.size() - num_cancelled_;
}

//----------------------------------------------------------------------
void
TimerSystem::post_signal(int sig)
{
    TimerSystem* _this = TimerSystem::instance();

    _this->sigfired_ = true;
    _this->signals_[sig] = true;
    
    _this->notifier_.signal();
}

//----------------------------------------------------------------------
void
TimerSystem::add_sighandler(int sig, sighandlerfn_t* handler)
{
    log_debug("adding signal handler %p for signal %d", handler, sig);
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

    if (! next_timer->cancelled_) {
        int late = TIMEVAL_DIFF_MSEC(now, next_timer->when());
        if (late > 2000) {
            log_warn("timer thread running slow -- timer is %d msecs late", late);
        }
        
        log_debug("popping timer %p at %u.%u", next_timer,
                  (u_int)now.tv_sec, (u_int)now.tv_usec);
        next_timer->timeout(now);
    } else {
        log_debug("popping cancelled timer %p at %u.%u", next_timer,
                  (u_int)now.tv_sec, (u_int)now.tv_usec);
        next_timer->cancelled_ = 0;
        ASSERT(num_cancelled_ > 0);
        num_cancelled_--;
        
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

        // if the next timer is cancelled, pop it immediately,
        // regardless of whether it's time has come or not
        if (next_timer->cancelled()) {
            pop_timer(now);
            continue;
        }
        
        if (TIMEVAL_LT(now, next_timer->when_)) {
            int diff_ms;

            // XXX/demmer it's possible that the next timer is too far
            // in the future to be expressable in milliseconds, so we
            // just max it out
            if (next_timer->when_.tv_sec - now.tv_sec < (INT_MAX / 1000)) {
                diff_ms = TIMEVAL_DIFF_MSEC(next_timer->when_, now);
            } else {
                log_debug("diff millisecond overflow: "
                          "next timer due at %u.%u, now %u.%u",
                          (u_int)next_timer->when_.tv_sec,
                          (u_int)next_timer->when_.tv_usec,
                          (u_int)now.tv_sec,
                          (u_int)now.tv_usec);
                
                diff_ms = INT_MAX;
            }

            ASSERTF(diff_ms >= 0,
                    "next timer due at %u.%u, now %u.%u, diff %d",
                    (u_int)next_timer->when_.tv_sec,
                    (u_int)next_timer->when_.tv_usec,
                    (u_int)now.tv_sec,
                    (u_int)now.tv_usec,
                    diff_ms);
            
            // there's a chance that we're within a millisecond of the
            // time to pop, but still not at the right time. in this
            // case case we don't return 0, but fall through to pop
            // the timer after adjusting the "current time"
            if (diff_ms == 0) {
                log_debug("sub-millisecond difference found, falling through");
                now = next_timer->when_;
            } else {
                log_debug("next timer due at %u.%u, now %u.%u -- "
                          "new timeout %d",
                          (u_int)next_timer->when_.tv_sec,
                          (u_int)next_timer->when_.tv_usec,
                          (u_int)now.tv_sec,
                          (u_int)now.tv_usec,
                          diff_ms);
                return diff_ms;
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
