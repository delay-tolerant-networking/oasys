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
#include <unistd.h>
#include <sys/poll.h>

#include "thread/Lock.h"
#include "io/IO.h"

#include "OnOffNotifier.h"
#include "Lock.h"

namespace oasys {

//----------------------------------------------------------------------------
OnOffNotifier::OnOffNotifier(const char* logpath, bool quiet)
    : Logger("OnOffNotifier", (logpath == 0) ? "" : logpath), 
      waiter_(false),
      quiet_(quiet),
      active_(false)
{ 
    if (logpath == 0)
    { 
        logpathf("/notifier");
    }
    else
    {
        logpath_appendf("/notifier");
    }

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
}

//----------------------------------------------------------------------------
OnOffNotifier::~OnOffNotifier()
{
    if (!quiet_) 
    {
        log_debug("OnOffNotifier shutting down (closing fds %d %d)",
                  pipe_[0], 
                  pipe_[1]);
    }
}

//----------------------------------------------------------------------------
bool
OnOffNotifier::wait(Lock* lock, int timeout)
{ 
    notifier_lock_.lock("OnOffNotifier::wait");
    if (waiter_) 
    {
        PANIC("OnOffNotifier doesn't support multiple waiting threads");
    }
    if (!quiet_)
    {
        log_debug("wait() on %s notifier", active_ ? "active" : "inactive");
    }

    if (active_)
    {
        notifier_lock_.unlock();
        return true;
    }
    else
    {
        waiter_ = true;
        
        notifier_lock_.unlock();

        if (lock) {
            lock->unlock();
        }
        int ret = IO::poll_single(read_fd(), POLLIN, 0, timeout, 0, logpath_);
        if (lock) {
            lock->lock("OnOffNotifier::wait()");
        }

        notifier_lock_.lock("OnOffNotifier::wait");
        waiter_ = false;
        notifier_lock_.unlock();
        
        if (ret < 0 && ret != IOTIMEOUT) 
        {
            PANIC("fatal: error return from notifier poll: %s",
                  strerror(errno));
        }
        else if (ret == IOTIMEOUT) 
        {
            if (! quiet_) {
                log_debug("wait() timeout");
            }
            return false;
        }
        else
        {
            if (! quiet_) { 
                log_debug("wait() notified");
            }
        }
        return true;
    }
}

//----------------------------------------------------------------------------
void
OnOffNotifier::signal()
{
    ScopeLock l(&notifier_lock_, "OnOffNotifier::signal");
    if (active_)
    {
        return;
    }
    else
    {
        int cc = write(pipe_[1], "+", 1);
        ASSERT(cc == 1);
        active_ = true;
    }
}

//----------------------------------------------------------------------------
void
OnOffNotifier::clear()
{
    ScopeLock l(&notifier_lock_, "OnOffNotifier::clear");

    if (active_)
    {
        char buf[2];
        int cc = read(pipe_[0], &buf, 1);
        ASSERT(cc == 1);
        active_ = false;
    }
    else
    {
        return;
    }
}

} // namespace oasys
