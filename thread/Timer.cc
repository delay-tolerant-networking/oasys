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

/*
 *    Modifications made to this file by the patch file oasys_mfs-33289-1.patch
 *    are Copyright 2015 United States Government as represented by NASA
 *       Marshall Space Flight Center. All Rights Reserved.
 *
 *    Released under the NASA Open Source Software Agreement version 1.3;
 *    You may obtain a copy of the Agreement at:
 * 
 *        http://ti.arc.nasa.gov/opensource/nosa/
 * 
 *    The subject software is provided "AS IS" WITHOUT ANY WARRANTY of any kind,
 *    either expressed, implied or statutory and this agreement does not,
 *    in any manner, constitute an endorsement by government agency of any
 *    results, designs or products resulting from use of the subject software.
 *    See the Agreement for the specific language governing permissions and
 *    limitations.
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
      cancel_lock_(new SpinLock()),
      notifier_(logpath_),
      timer_q1_(),
      timer_q2_(),
      seqno_(0),
      num_cancelled_(0),
      old_num_cancelled_(0)
{
    memset(handlers_, 0, sizeof(handlers_));
    memset(signals_, 0, sizeof(signals_));
    sigfired_ = false;

    timers_ = &timer_q1_;
    old_timers_ = &timer_q2_;

    should_stop_ = false;
}

//----------------------------------------------------------------------
TimerSystem::~TimerSystem()
{
    //printf("Time system destructor run\n"); 

    while (! timers_->empty()) {
        Timer* t = timers_->top();
        t->pending_ = false; // to avoid assertion
        timers_->pop();
        delete t;
    }

    while (! old_timers_->empty()) {
        Timer* t = old_timers_->top();
        t->pending_ = false; // to avoid assertion
        old_timers_->pop();
        delete t;
    }
    
    delete cancel_lock_;
    delete system_lock_;

    instance_ = NULL;
}

//----------------------------------------------------------------------
void
TimerSystem::shutdown()
{
    ScopeLock cl(system_lock_, "TimerSystem::shutdown");
    should_stop_ = true;
    notifier_.signal();
}

//----------------------------------------------------------------------
void
TimerSystem::schedule_at(struct timeval *when, Timer* timer)
{
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

    if (!should_stop_) {    
        ScopeLock l(system_lock_, "TimerSystem::schedule_at");

        timers_->push(timer);
    } else {
        delete timer;  // assumes process is exiting
    }

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
    if (!should_stop_) {
        ScopeLock cl(cancel_lock_, "TimerSystem::cancel");

        //XXX/dz in case timer pop on this timer is in progress
        timer->cancelled_ = true;

        // There's no good way to get a timer out of a heap, so we let it
        // stay in there and mark it as cancelled so when it bubbles to
        // the top, we don't bother with it. This makes rescheduling a
        // single timer instance tricky...
        if (timer->pending_) {
            num_cancelled_++;
            return true;
        }
    }

    return false;
}

//----------------------------------------------------------------------
size_t
TimerSystem::num_pending_timers()
{
    return timers_->size() + old_timers_->size() - 
           num_cancelled_ - old_num_cancelled_;
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
TimerSystem::process_popped_timer(const struct timeval& now, Timer* next_timer)
{
    if (! next_timer->cancelled_) {
        int late = TIMEVAL_DIFF_MSEC(now, next_timer->when());
        if (late > 2000) {
            log_warn("timer thread running slow -- timer is %d msecs late", late);
        }
        
        log_debug("popping timer %p at %u.%u", next_timer,
                  (u_int)now.tv_sec, (u_int)now.tv_usec);

        if (!should_stop_) {
            next_timer->timeout(now);
        }
    } else {
        log_debug("popping cancelled timer %p at %u.%u", next_timer,
                  (u_int)now.tv_sec, (u_int)now.tv_usec);
        //dz debug  next_timer->cancelled_ = 0;
        //ASSERT(num_cancelled_ > 0);
        if (num_cancelled_ > 0) {
            num_cancelled_--;
        } else {
            log_warn("TimerSystem::process_popped_timer - num_cancelled_ timers is zero while popping cancelled timer");
        }
        
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
                // XXX/dz caught a segfault trying to process sig 0??
                if (handlers_[i]) {
                    handlers_[i](i);
                }
                signals_[i] = 0;
            }
        }
    }
}

//----------------------------------------------------------------------
int
TimerSystem::run_expired_timers()
{
   int time_to_next_expiration = -1;


    struct timeval now;
    Timer* next_timer = NULL;
    int diff_ms;

    // minimizing the time that the lock is held
    // so that new timers do not get delayed excessively        
    //ScopeLock l(system_lock_, "TimerSystem::run_expired_timers");
    system_lock_->lock("TimerSystem::run_expired_timers - signals");
    
    handle_signals();

    while ( !should_stop_ && -1 == time_to_next_expiration && !timers_->empty() ) 
    {
        // do not allow cancel while working with this next timer
        cancel_lock_->lock("TimerSystem::run_expired_timers get top");

        next_timer = timers_->top();

        if (next_timer->cancelled()) {
            // if the next timer is cancelled, pop it immediately,
            // regardless of whether it's time has come or not
            // and fall through to "process" it
        } else {
            if (::gettimeofday(&now, 0) != 0) {
                PANIC("gettimeofday");
            }

            if (TIMEVAL_LT(now, next_timer->when_)) {

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
                    // clear the next_timer pointer so it does not get processed
                    next_timer = NULL;
                    time_to_next_expiration = diff_ms;
                }
            }
        }

        // verify the top timer is still the same one
        if (NULL == next_timer) {
            cancel_lock_->unlock();
        } else {
            timers_->pop();

            // clear the pending bit since it could get rescheduled 
            ASSERT(next_timer->pending_);
            next_timer->pending_ = 0;

            // release the locks while processing the timer
            cancel_lock_->unlock();
            system_lock_->unlock();

            process_popped_timer(now, next_timer);

            // reaquire the lock for the next pass through the loop
            system_lock_->lock("TimerSystem::run_expired_timers - after pop");
        }
    }

    system_lock_->unlock();

    return time_to_next_expiration;
}

//----------------------------------------------------------------------
void
TimerSystem::reinsert_timer(Timer* timer)
{
    ScopeLock l(system_lock_, "TimerSystem::reinsert_timer");

    timers_->push(timer);

    notifier_.signal();
}

//----------------------------------------------------------------------
void
TimerSystem::process_old_cancelled_timer(Timer* timer, 
                                         const struct timeval& now)
{
    (void) now; // in case debug is disabled
    log_debug("popping cancelled timer %p at %u.%u", timer,
              (u_int)now.tv_sec, (u_int)now.tv_usec);
    timer->pending_ = false;
    old_num_cancelled_--;
        
    if (timer->cancel_flags_ == Timer::DELETE_ON_CANCEL) {
        log_debug("deleting cancelled timer %p at %u.%u", timer,
                  (u_int)now.tv_sec, (u_int)now.tv_usec);
        delete timer;
    }
    
}

//----------------------------------------------------------------------
void
TimerSystem::check_cancelled_timers()
{
    Timer* next_timer;
    //bool found_good_timer = false;
    struct timeval now;    

    if (old_timers_->empty()) {
        // have canceled timers accumulated enough to warrant cleaning up?

        cancel_lock_->lock("TimerSystem::check_cancelled_timers - calcs");
        size_t active_timers = timers_->size() - num_cancelled_;
        size_t cancelled_timers = num_cancelled_;
        cancel_lock_->unlock();

        if ((cancelled_timers >= active_timers) || (cancelled_timers >= 1000)) {
            system_lock_->lock("TimerSystem::check_cancelled_timers - swap timer queues");
            cancel_lock_->lock("TimerSystem::check_cancelled_timers - swap timer queues");

            // swap the timer queues
            if (timers_ == &timer_q1_) {
                timers_ = &timer_q2_;
                old_timers_ = &timer_q1_;
            } else {
                timers_ = &timer_q1_;
                old_timers_ = &timer_q2_;
            }
            old_num_cancelled_ = num_cancelled_;
            num_cancelled_ = 0;

            // minimizing the time that the lock is held
            // so that new timers do not get delayed excessively        
            cancel_lock_->unlock();
            system_lock_->unlock();
        }
    }

    // process all of the old timers at our leisure since 
    // we are not locking out the TimerSystem
    while (! old_timers_->empty()) 
    {
        next_timer = old_timers_->top();
        old_timers_->pop();

        if (! next_timer->cancelled_) {
            reinsert_timer(next_timer);
        } else {
            if (::gettimeofday(&now, 0) != 0) {
                PANIC("gettimeofday");
            }

            process_old_cancelled_timer(next_timer, now);
        }
    }

    // it is possible that some timers were cancelled while
    // they were in old_timers_ queue so just zero out the
    // old_num_cancelled_ count and it should all come out
    // in the wash eventually
    old_num_cancelled_ = 0;
}

//----------------------------------------------------------------------
void
TimerSystem::cancel_all_timers()
{
    Timer* next_timer;

    int num_cancels = 0;
    int num_deleted = 0;

    system_lock_->lock("TimerSystem::cancel_all_timers");

    while (! timer_q1_.empty()) 
    {
        next_timer = timer_q1_.top();
        timer_q1_.pop();

        next_timer->cancelled_ = true;
        next_timer->pending_ = false;

        ++num_cancels;
        
        if (next_timer->cancel_flags_ == Timer::DELETE_ON_CANCEL) {
            delete next_timer;
            ++num_deleted;
        }
    }

    while (! timer_q2_.empty()) 
    {
        next_timer = timer_q2_.top();
        timer_q2_.pop();

        next_timer->cancelled_ = true;
        next_timer->pending_ = false;
        
        ++num_cancels;
        
        if (next_timer->cancel_flags_ == Timer::DELETE_ON_CANCEL) {
            delete next_timer;
            ++num_deleted;
        }
    }

    old_num_cancelled_ = 0;
    system_lock_->unlock();

    if (!destroy_singletons_on_exit_  && !getenv("OASYS_CLEANUP_SINGLETONS")) {
        delete this;
    }
}

//----------------------------------------------------------------------
void
TimerThread::run()
{
    TimerSystem* sys = TimerSystem::instance();
    while (!should_stop()) 
    {
        int timeout = sys->run_expired_timers();
        if (sys->notifier()->wait(NULL, timeout) ) {
            // if notifier was active then we need to clear it
            sys->notifier()->clear();
        }
    }

    sys->cancel_all_timers();
    
    instance_ = NULL;
    //NOTREACHED;
}

//----------------------------------------------------------------------
void
TimerThread::shutdown()
{
    TimerSystem* sys = TimerSystem::instance();
    sys->shutdown();
    set_should_stop();
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

//----------------------------------------------------------------------
void
TimerReaperThread::run()
{
    int ctr = 0;
    TimerSystem* sys = TimerSystem::instance();
    while (!should_stop()) 
    {
        usleep(250000);

        // roughly every 60 seconds - make configurable?
        if (++ctr >= 60*4) {
            if (!should_stop()) {
                sys->check_cancelled_timers();
            }
            ctr = 0;
        }
    }

    instance_ = NULL;
    //NOTREACHED;
}

//----------------------------------------------------------------------
void
TimerReaperThread::init()
{
    ASSERT(instance_ == NULL);
    instance_ = new TimerReaperThread();
    instance_->start();
}

TimerReaperThread* TimerReaperThread::instance_ = NULL;

} // namespace oasys
