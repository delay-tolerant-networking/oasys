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


#ifndef OASYS_TIMER_H
#define OASYS_TIMER_H

#ifndef OASYS_CONFIG_STATE
#error "MUST INCLUDE oasys-config.h before including this file"
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <queue>
#include <signal.h>
#include <math.h>

#include "../debug/DebugUtils.h"
#include "../debug/Log.h"
#include "../util/Singleton.h"
#include "../util/Time.h"
#include "MsgQueue.h"
#include "OnOffNotifier.h"
#include "Thread.h"

/**
 * Typedef for a signal handler function. On some (but not all)
 * systems, there is a system-provided __sighandler_t typedef that is
 * equivalent, but in other cases __sighandler_t is a function pointer
 * (not a function).
 */
typedef RETSIGTYPE (sighandlerfn_t) (int);

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

#define DOUBLE_TO_TIMEVAL(d, tv)                                             \
    do {                                                                     \
        (tv).tv_sec  = static_cast<unsigned long>(floor(d));                 \
        (tv).tv_usec = static_cast<unsigned long>((d - floor(d)) * 1000000); \
    } while (0)

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
 * The main Timer system implementation that needs to be driven by a
 * thread, such as the TimerThread class defined below.
 */
class TimerSystem : public Singleton<TimerSystem>,
                    public Logger {
public:
    void schedule_at(struct timeval *when, Timer* timer);
    void schedule_in(int milliseconds, Timer* timer);
    void schedule_immediate(Timer* timer);
    bool cancel(Timer* timer);

    /**
     * Hook to use the timer thread to safely handle a signal.
     */
    void add_sighandler(int sig, sighandlerfn_t* handler);

    /**
     * Hook called from an the actual signal handler that notifies the
     * timer system thread to call the signal handler function.
     */
    static void post_signal(int sig);

    /**
     * Run any timers that have expired. Returns the interval in
     * milliseconds until the next timer that needs to fire.
     */
    int run_expired_timers();

    /**
     * Accessor for the notifier that indicates if another thread put
     * a timer on the queue.
     */
    OnOffNotifier* notifier() { return &notifier_; }

    /**
     * Return a count of the number of pending timers.
     */
    size_t num_pending_timers();

private:
    friend class Singleton<TimerSystem>;
    typedef std::priority_queue<Timer*, 
                                std::vector<Timer*>, 
                                TimerCompare> TimerQueue;
    

    //! KNOWN ISSUE: Signal handling has race conditions - but it's
    //! not worth the trouble to fix.
    sighandlerfn_t* handlers_[NSIG];	///< handlers for signals
    bool 	    signals_[NSIG];	///< which signals have fired
    bool	    sigfired_;		///< boolean to check if any fired

    SpinLock*  system_lock_;
    OnOffNotifier notifier_;
    TimerQueue timers_;
    u_int32_t   seqno_;       ///< seqno used to break ties
    size_t      num_cancelled_; ///< needed for accurate pending_timer count

    TimerSystem();
    virtual ~TimerSystem();
    
    void pop_timer(const struct timeval& now);
    void handle_signals();

};

/**
 * A simple thread class that drives the TimerSystem implementation.
 */
class TimerThread : public Thread {
public:
    static void init();

private:
    TimerThread() : Thread("TimerThread") {}
    void run();
    
    static TimerThread* instance_;
};

/**
 * A Timer class. Provides methods for scheduling timers. Derived
 * classes must override the pure virtual timeout() method.
 */
class Timer {
public:
    /// Enum type for cancel flags related to memory management
    typedef enum {
        NO_DELETE = 0,
        DELETE_ON_CANCEL = 1
    } cancel_flags_t;

    Timer(cancel_flags_t cancel_flags = DELETE_ON_CANCEL)
        : pending_(false),
          cancelled_(false),
          cancel_flags_(cancel_flags)
    {}
    
    virtual ~Timer() 
    {
        /*
         * The only time a timer should be deleted is after it fires,
         * so assert as such.
         */
        ASSERTF(pending_ == false, "can't delete a pending timer");
    }
    
    void schedule_at(struct timeval *when)
    {
        TimerSystem::instance()->schedule_at(when, this);
    }
    
    void schedule_at(const Time& when)
    {
        struct timeval tv;
        tv.tv_sec  = when.sec_;
        tv.tv_usec = when.usec_;
        TimerSystem::instance()->schedule_at(&tv, this);
    }
    
    void schedule_in(int milliseconds)
    {
        TimerSystem::instance()->schedule_in(milliseconds, this);
    }
    void schedule_immediate()
    {
        TimerSystem::instance()->schedule_immediate(this);
    }

    bool cancel()
    {
        return TimerSystem::instance()->cancel(this);
    }

    bool pending()
    {
        return pending_;
    }

    bool cancelled()
    {
        return cancelled_;
    }

    struct timeval when()
    {
        return when_;
    }
    
    virtual void timeout(const struct timeval& now) = 0;

protected:
    friend class TimerSystem;
    friend class TimerCompare;
    
    struct timeval when_;	  ///< When the timer should fire
    bool           pending_;	  ///< Is the timer currently pending
    bool           cancelled_;	  ///< Is this timer cancelled
    cancel_flags_t cancel_flags_; ///< Should we keep the timer around
                                  ///< or delete it when the cancelled
                                  ///< timer bubbles to the top
    u_int32_t      seqno_;        ///< seqno used to break ties
};

/**
 * The Timer comparator function used in the priority queue.
 */
bool
TimerCompare::operator()(Timer* a, Timer* b)
{
    if (TIMEVAL_GT(a->when_, b->when_)) return true;
    if (TIMEVAL_LT(a->when_, b->when_)) return false;
    return a->seqno_ > b->seqno_;
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

#endif /* OASYS_TIMER_H */

