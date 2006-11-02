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


#include <errno.h>
#include <unistd.h>
#include <sys/poll.h>
#include "Notifier.h"
#include "SpinLock.h"
#include "io/IO.h"

namespace oasys {

Notifier::Notifier(const char* logpath, bool quiet)
    : Logger("Notifier", logpath), 
      count_(0),
      quiet_(quiet),
      busy_notifiers_(0)
{
    logpath_appendf("/notifier");
    
    if (pipe(pipe_) != 0) {
        PANIC("can't create pipe for notifier");
    }

    if (!quiet_) {
        log_debug("created pipe, fds: %d %d", pipe_[0], pipe_[1]);
    }
    
    for (int n = 0; n < 2; ++n) {
        if (IO::set_nonblocking(pipe_[n], true, quiet ? 0 : logpath_) != 0) 
	{
            PANIC("error setting fd %d to nonblocking: %s",
                     pipe_[n], strerror(errno));
        }
    }

    waiter_ = false;
}

Notifier::~Notifier()
{
    int err;
    if (!quiet_) {
        log_debug("Notifier shutting down (closing fds %d %d)",
                  pipe_[0], pipe_[1]);
    }

    err = close(pipe_[0]);
    if (err != 0) {
        log_err("error closing pipe %d: %s", pipe_[0], strerror(errno));
    }

    err = close(pipe_[1]);
    if (err != 0) {
        log_err("error closing pipe %d: %s", pipe_[1], strerror(errno));
    }
    
    // Allow graceful deletion by a wait thread in a wait/notify scenario:
    // Upon notification by some "finished" signal, a wait thread
    // may decide to delete this object. We want to avoid having that happen
    // while notification of the "finished" message is still in progress.
    while(atomic_cmpxchg32(&busy_notifiers_, 0, 1) != 0)
    {
        usleep(100000);
    }
}

void
Notifier::drain_pipe(size_t bytes)
{
    int    ret;
    char   buf[256];
    size_t bytes_drained = 0;

    while (true)
    {
        if (!quiet_) {
            log_debug("drain_pipe: attempting to drain %zu bytes", bytes);
        }

        ret = IO::read(read_fd(), buf,
                       (bytes == 0) ? sizeof(buf) :
                       std::min(sizeof(buf), bytes - bytes_drained));
        if (ret <= 0) {
            if (ret == IOAGAIN) {
                PANIC("drain_pipe: trying to drain with not enough notify "
                      "calls, count = %u and trying to drain %zu bytes", 
                      count_, bytes);
                break;
            } else {
                log_crit("drain_pipe: unexpected error return from read: %s",
                         strerror(errno));
                exit(1);
            }
        }
        
        bytes_drained += ret;
        if (!quiet_) {
            log_debug("drain_pipe: drained %zu/%zu byte(s) from pipe", 
                      bytes_drained, bytes);
        }
        count_ -= ret;
        
        if (bytes == 0 || bytes_drained == bytes) {
            break;
        }
        
        // More bytes were requested from the pipe than there are
        // bytes in the pipe. This means that the bytes requested is
        // bogus. This probably is the result of a race condition.
        if (ret < static_cast<int>(sizeof(buf))) {
            log_warn("drain_pipe: only possible to drain %zu bytes out of %zu! "
                     "race condition?", bytes_drained, bytes);
            break;
        }
    }

    if (!quiet_) {
        log_debug("drain pipe count = %d", count_);
    }
}

bool
Notifier::wait(SpinLock* lock, int timeout, bool drain_the_pipe)
{
    if (waiter_) {
        PANIC("Notifier doesn't support multiple waiting threads");
    }
    waiter_ = true;

    if (!quiet_) {
        log_debug("attempting to wait on %p, count = %d", 
                  this, count_);
    }
    
    if (lock)
        lock->unlock();

    int ret = IO::poll_single(read_fd(), POLLIN, 0, timeout, 0, logpath_);
    if (ret < 0 && ret != IOTIMEOUT) {
        PANIC("fatal: error return from notifier poll: %s",
              strerror(errno));
    }
    
    if (lock) {
        lock->lock("Notifier::wait");
    }

    waiter_ = false;
    
    if (ret == IOTIMEOUT) {
        if (!quiet_) {
            log_debug("notifier wait timeout");
        }
        return false; // timeout
    } else {
        if (drain_the_pipe)
        {
            drain_pipe(1);
        }
        if (!quiet_) {
            log_debug("notifier wait successfully notified");
        }
        return true;
    }
}

void
Notifier::notify(SpinLock* lock)
{
	atomic_incr(&busy_notifiers_);
    char b = 0;
    int num_retries = 0;

    bool need_to_relock = false;
    
  retry:
    if (!quiet_) {
        log_debug("notifier notify");
    }

    // see the comment below
    if (need_to_relock && (lock != NULL)) {
        lock->lock("Notifier::notify");
    }

    int ret = ::write(write_fd(), &b, 1);
    
    if (ret == -1) {
        if (errno == EAGAIN) {
            // If the pipe is full, that probably means the consumer
            // is just slow, but keep trying for up to 30 seconds
            // because otherwise we will break the semantics of the
            // notifier.
            //
            // We need to release the lock before sleeping to give
            // another thread a chance to come in and drain, however
            // it is important that we re-take the lock before writing
            // to maintain the atomicity required by MsgQueue
            if (num_retries == 0) {
                log_warn("pipe appears to be full -- retrying write until success"); 
            }

            if (++num_retries == 600) {
                // bail after 1 minute of spinning
                PANIC("slow reader on pipe: can't notify within 1 minute!");
            }
            
            // give it some time
            if (lock) {
                lock->unlock();
                need_to_relock = true;
            }
            
            usleep(100000);
            goto retry;
        } else {
            log_err("unexpected error writing to pipe fd %d: %s",
                    write_fd(), strerror(errno));
        }
    } else if (ret == 0) {
        log_err("unexpected eof writing to pipe");
    } else {
        ASSERT(ret == 1);

        // XXX/demmer potential bug here -- once the pipe has been
        // written to, this thread might context-switch out and the
        // other thread (that owns the notifier) could be woken up. at
        // which point the notifier object itself might be deleted.
        //
        // solutions: either be careful to only write at the end of
        // the fn, or (better) use a lock.
        ++count_;
        if (!quiet_) {
            log_debug("notify count = %d", count_);
        }
    }
    atomic_decr(&busy_notifiers_);
}

} // namespace oasys
