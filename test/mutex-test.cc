#include <cstdio>
#include <sys/time.h>

#include "debug/Log.h"
#include <thread/Mutex.h>
#include <thread/Thread.h>

using namespace oasys;


int invariant[1000];

class Thread1 : public Thread {
public:
    Thread1(Mutex* m) : mutex_(m) {}
    
protected:
    virtual void run() {
        struct timeval start, now;

        while(true)
        {
            ScopeLock lock(mutex_);

            fprintf(stderr, "Thread1: grabbed lock, sleeping");

            gettimeofday(&start, NULL);
            
            for(int i=0; i<1000; ++i)
            {
                invariant[i]++;
            }

            do {
                gettimeofday(&now, NULL);

                // reminds us of the Win3.1 days
                Thread::yield();
            } while(now.tv_sec - 1 < start.tv_sec);
        }
    }

    Mutex* mutex_;
};


class Thread2 : public Thread {
public:
    Thread2(Mutex* m) : mutex_(m) {}
    
protected:
    virtual void run() {
        struct timeval start, now;

        while(true)
        {
            ScopeLock lock(mutex_);

            fprintf(stderr, "Thread2: grabbed lock, sleeping\n");
            
            // recursive locking test
            {
                ScopeLock lock2(mutex_);
                gettimeofday(&start, NULL);
            }

            do {
                // assert invariant
                int val = invariant[0];
                for(int i=0; i<1000; i++)
                {
                    ASSERT(invariant[i] == val);
                }

                // reminds us of the Win3.1 days
                Thread::yield();

                gettimeofday(&now, NULL);
            } while(now.tv_sec - 0.3333 < start.tv_sec);
        }
    }

    Mutex* mutex_;
};


int
main()
{
    for(int i=0; i<1000; ++i)
    {
        invariant[i] = 0;
    }
    
    Log::init(LOG_DEBUG);

    Mutex mutex;
    
    Thread* t1 = new Thread1(&mutex);
    Thread* t2 = new Thread2(&mutex);
    t1->start();
    t2->start();
    
    while(true) {
        Thread::yield();
    }
}
