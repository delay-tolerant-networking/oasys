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

#ifndef tier_timer_h
#define tier_timer_h

#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <queue>
#include <signal.h>

#include "../config.h"
#include "../debug/Debug.h"
#include "../debug/Log.h"
#include "MsgQueue.h"
#include "Notifier.h"
#include "Thread.h"

typedef RETSIGTYPE (*__sighandler_t) (int);

namespace oasys {

/**
 * Miscellaneous timeval macros.
 */
#define TIMEVAL_DIFF(t1, t2, t3) \
    do { \
       ((t3).tv_sec  = (t1).tv_sec - (t2).tv_sec); \
       ((t3).tv_usec = (t1).tv_usec - (t2).tv_usec); \
       if ((t3).tv_usec < 0) { (t3).tv_sec--; (t3).tv_usec += 1000000; } \
    } while (0)

#define TIMEVAL_DIFF_DOUBLE(t1, t2) \
    ((double)(((t1).tv_sec  - (t2).tv_sec)) + \
     (double)((((t1).tv_usec - (t2).tv_usec)) * 1000000.0))

#define TIMEVAL_DIFF_MSEC(t1, t2) \
    ((unsigned long int)(((t1).tv_sec  - (t2).tv_sec)  * 1000) + \
     (((t1).tv_usec - (t2).tv_usec) / 1000))

#define TIMEVAL_DIFF_USEC(t1, t2) \
    ((unsigned long int)(((t1).tv_sec  - (t2).tv_sec)  * 1000000) + \
     (((t1).tv_usec - (t2).tv_usec)))

#define TIMEVAL_GT(t1, t2) \
    (((t1).tv_sec  >  (t2).tv_sec) ||  \
     (((t1).tv_sec == (t2).tv_sec) && ((t1).tv_usec > (t2).tv_usec)))

#define TIMEVAL_LT(t1, t2) \
    (((t1).tv_sec  <  (t2).tv_sec) ||  \
     (((t1).tv_sec == (t2).tv_sec) && ((t1).tv_usec < (t2).tv_usec)))

class SpinLock;
class Timer;

/**
 * The Timer comparison class.
 */
class TimerCompare {
public:
    inline bool operator ()(Timer* a, Timer* b);
};    

/**
 * The main Timer system implementation.
 */
class TimerSystem : public Thread, public Logger {
public:
    static TimerSystem* instance() {
        ASSERT(instance_ != NULL);
        return instance_;
    }

    static void init();

    void schedule_at(struct timeval *when, Timer* timer);
    void schedule_in(int milliseconds, Timer* timer);
    void schedule_immediate(Timer* timer);
    bool cancel(Timer* timer);

    /**
     * Hook to use the timer thread to safely handle a signal.
     */
    void add_sighandler(int sig, __sighandler_t handler);

    /**
     * Hook called from an the actual signal handler that notifies the
     * timer sysetem thread to call the signal handler function.
     */
    static void post_signal(int sig);
                             
    void run();

private:
    TimerSystem();
    
    void pop_timer(struct timeval *now);
    
    static TimerSystem* instance_;

    SpinLock* system_lock_;
    Notifier signal_;
    std::priority_queue<Timer*, std::vector<Timer*>, TimerCompare> timers_;

    __sighandler_t handlers_[NSIG];	///< handlers for signals
    bool 	   signals_[NSIG];	///< which signals have fired
    bool	   sigfired_;		///< boolean to check if any fired
};

/**
 * A Timer class. Provides methods for scheduling timers. Derived
 * classes must override the pure virtual timeout() method.
 */
class Timer {
public:
    Timer() : pending_(false), cancelled_(false), cancel_flags_(NO_DELETE) {}
    
    virtual ~Timer() {}
    
    void schedule_at(struct timeval *when)
    {
        TimerSystem::instance()->schedule_at(when, this);
    }
    
    void schedule_in(int milliseconds)
    {
        TimerSystem::instance()->schedule_in(milliseconds, this);
    }

    void schedule_immediate()
    {
        TimerSystem::instance()->schedule_immediate(this);
    }

    typedef enum {
        NO_DELETE = 0,
        DELETE_ON_CANCEL = 1
    } cancel_flags_t;
    
    bool cancel(cancel_flags_t cancel_flags)
    {
        cancel_flags_ = cancel_flags;
        return TimerSystem::instance()->cancel(this);
    }

    int pending()
    {
        return pending_;
    }

    struct timeval when()
    {
        return when_;
    }
    
    virtual void timeout(struct timeval* now) = 0;

private:
    friend class TimerSystem;
    friend class TimerCompare;
    
    struct timeval when_;
    bool pending_;
protected:
    bool cancelled_;
    cancel_flags_t cancel_flags_;
};

/**
 * The Timer comparator function used in the priority queue.
 */
bool
TimerCompare::operator()(Timer* a, Timer* b)
{
    return TIMEVAL_GT(a->when_, b->when_);
}

/**
 * For use with the QueuingTimer, this struct defines a TimerEvent,
 * i.e. a particular firing of a Timer that captures the timer and the
 * time when it fired.
 */
struct TimerEvent {
    TimerEvent(const Timer* timer, struct timeval* time)
        : timer_(timer), time_(*time)
    {
    }
    
    const Timer* timer_;
    struct timeval time_;
};

/**
 * The queue type used in the QueueingTimer.
 */
typedef MsgQueue<TimerEvent> TimerEventQueue;
    
/**
 * A Timer class that's useful in cases when a separate thread (i.e.
 * not the main TimerSystem thread) needs to process the timer event.
 * Note that multiple QueuingTimer instances can safely share the same
 * event queue.
 */
class QueuingTimer : public Timer {
public:
    QueuingTimer(TimerEventQueue* queue) : queue_(queue) {}
    
    virtual void timeout(struct timeval* now)
    {
        queue_->push(TimerEvent(this, now));
    }
    
protected:
    TimerEventQueue* queue_;
};

} // namespace oasys

#endif /* tier_timer_h */

