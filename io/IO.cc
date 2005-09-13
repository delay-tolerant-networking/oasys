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
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/fcntl.h>

#include "IO.h"

#include "debug/Log.h"
#include "thread/Notifier.h"

namespace oasys {

/////////////////////////////////////////////////////////////////////////////
//! Small helper class which is a copy-on-write iovec and also handle
//! adjustment for consumption of the bytes in the iovec.
class COWIoVec {
public:
    COWIoVec(const struct iovec* iov, int iovcnt) 
	: iov_(const_cast<struct iovec*>(iov)),
	  iovcnt_(iovcnt),
	  bytes_left_(0),
	  copied_(false),
	  dynamic_iov_(0)
    {
	for (int i=0; i<iovcnt_; ++i) {
	    bytes_left_ += iov_[i].iov_len;
	}
    }

    ~COWIoVec() { 
	if (dynamic_iov_ != 0) {
	    free(iov_); 
	    dynamic_iov_ = 0;
	} 
    }
    
    //! @return number of bytes left in the iovecs
    void consume(size_t cc) {
	ASSERT(bytes_left_ >= cc);

	// common case, all the bytes are gone on the first run
	if (!copied_ && cc == bytes_left_) {
	    iov_        = 0;
	    bytes_left_ = 0;
	    return;
	}
	
	if (!copied_) {
	    copy();
        }
	
	// consume the iovecs
	bytes_left_ -= cc;
	while (cc > 0) {
	    ASSERT(iovcnt_ > 0);

	    if (iov_[0].iov_len <= cc) {
		cc -= iov_[0].iov_len;
		--iovcnt_;
		++iov_;
	    } else {
		iov_[0].iov_base = reinterpret_cast<char*>
                                   (iov_[0].iov_base) + cc;
		iov_[0].iov_len  -= cc;
                cc = 0;
                break;
            }
	}
        
        // For safety
        if (bytes_left_ == 0) {
            iov_ = 0;
        }
    }

    void copy() {
	ASSERT(!copied_);
	
	copied_ = true;
	if (iovcnt_ <= 16) {
	    memcpy(static_iov_, iov_, 
		   iovcnt_ * sizeof(struct iovec));
	    iov_ = static_iov_;
	} else {
	    dynamic_iov_ = static_cast<struct iovec*>
                           (malloc(iovcnt_ * sizeof(struct iovec)));
	    memcpy(dynamic_iov_, iov_, iovcnt_* sizeof(struct iovec));
	    iov_ = dynamic_iov_;
	}
    }
    
    const struct iovec* iov()        { return iov_; }
    int                 iovcnt()     { return iovcnt_; }
    size_t              bytes_left() { return bytes_left_; }

private:
    struct iovec* iov_;
    int           iovcnt_;
    size_t        bytes_left_;
    
    bool          copied_;
    struct iovec  static_iov_[16];
    struct iovec* dynamic_iov_;
};

//////////////////////////////////////////////////////////////////////////////
const char* 
IO::ioerr2str(int err)
{
    switch (err) {
    case IOEOF:     return "eof";
    case IOERROR:   return "error";
    case IOTIMEOUT: return "timeout";
    case IOINTR:    return "intr";
    }
    
    NOTREACHED;
}

//////////////////////////////////////////////////////////////////////////////
int
IO::open(const char* path, int flags, const char* log)
{
    int fd = ::open(path, flags);
    if (log) {
        logf(log, LOG_DEBUG, "open %s (flags 0x%x): fd %d", path, flags, fd);
    }
    return fd;
}

//////////////////////////////////////////////////////////////////////////////
int
IO::open(const char* path, int flags, mode_t mode, const char* log)
{
    int fd = ::open(path, flags, mode);
    if (log) {
        logf(log, LOG_DEBUG, "open %s (flags 0x%x mode 0x%x): fd %d",
             path, flags, (u_int) mode, fd);
    }
    return fd;
}
    
//////////////////////////////////////////////////////////////////////////////
int
IO::close(int fd, const char* log, const char* filename)
{
    int ret = ::close(fd);
    if (log) {
        logf(log, LOG_DEBUG, "close %s fd %d: %d", filename, fd, ret);
    }
    return ret;
}

//////////////////////////////////////////////////////////////////////////////
int
IO::unlink(const char* path, const char* log)
{
    int ret = ::unlink(path);
    if (log) {
        logf(log, LOG_DEBUG, "unlink %s: %d", path, ret);
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////////
int
IO::lseek(int fd, off_t offset, int whence, const char* log)
{
    int cc = ::lseek(fd, offset, whence);
    if (log) {
        logf(log, LOG_DEBUG, "lseek %u %s -> %d",
             (u_int)offset,
             (whence == SEEK_SET) ? "SEEK_SET" :
             (whence == SEEK_CUR) ? "SEEK_CUR" :
             (whence == SEEK_END) ? "SEEK_END" :
             "SEEK_INVALID",
             cc);
    }

    return cc;
}

//////////////////////////////////////////////////////////////////////////////
int
IO::truncate(int fd, off_t length, const char* log)
{
    int ret = ftruncate(fd, length);
    if (log) {
        logf(log, LOG_DEBUG, "truncate %u: %d", (u_int)length, ret);
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////////
int
IO::mkstemp(char* templ, const char* log)
{
    int ret = ::mkstemp(templ);
    if (log) {
        logf(log, LOG_DEBUG, "mkstemp %s: %d", templ, ret);
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////////
int
IO::read(int fd, char* bp, size_t len, 
         Notifier* intr, const char* log)
{
    struct iovec iov;
    iov.iov_base = bp;
    iov.iov_len  = len;
    return rwdata(READV, fd, &iov, 1, 0, -1, 0, 0, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::readv(int fd, const struct iovec* iov, int iovcnt, 
          Notifier* intr, const char* log)
{
    return rwdata(READV, fd, iov, iovcnt, 0, -1, 0, 0, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::readall(int fd, char* bp, size_t len, 
            Notifier* intr, const char* log)
{
    struct iovec iov;
    iov.iov_base = bp;
    iov.iov_len  = len;

    return rwvall(READV, fd, &iov, 1, -1, 0, intr, "readall", log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::readvall(int fd, const struct iovec* iov, int iovcnt,
             Notifier* intr, const char* log)
{
    return rwvall(READV, fd, iov, iovcnt, -1, 0, intr, "readvall", log);
}


//////////////////////////////////////////////////////////////////////////////
int
IO::timeout_read(int fd, char* bp, size_t len, int timeout_ms,
                 Notifier* intr, const char* log)
{
    struct iovec iov;
    iov.iov_base = bp;
    iov.iov_len  = len;

    struct timeval start;
    gettimeofday(&start, 0);

    return rwdata(READV, fd, &iov, 1, 0, timeout_ms, 0, 
                  &start, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::timeout_readv(int fd, const struct iovec* iov, int iovcnt, int timeout_ms,
                  Notifier* intr, const char* log)
{
    struct timeval start;
    gettimeofday(&start, 0);

    return rwdata(READV, fd, iov, iovcnt, 0, timeout_ms, 0, 
                  &start, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::timeout_readall(int fd, char* bp, size_t len, int timeout_ms,
                    Notifier* intr, const char* log)
{
    struct iovec iov;
    iov.iov_base = bp;
    iov.iov_len  = len;

    struct timeval start;
    gettimeofday(&start, 0);    

    return rwvall(READV, fd, &iov, 1, timeout_ms, &start, 
                  intr, "timeout_readall", log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::timeout_readvall(int fd, const struct iovec* iov, int iovcnt, 
                     int timeout_ms, Notifier* intr, const char* log)
{
    struct timeval start;
    gettimeofday(&start, 0);        

    return rwvall(READV, fd, iov, iovcnt, timeout_ms, &start, intr, 
                  "timeout_readvall", log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::recv(int fd, char* bp, size_t len, int flags,
         Notifier* intr, const char* log)
{
    struct iovec iov;
    iov.iov_base = bp;
    iov.iov_len  = len;
    return rwdata(RECV, fd, &iov, 1, 0, -1, 0, 0, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::recvfrom(int fd, char* bp, size_t len, int flags,
             struct sockaddr* from, socklen_t* fromlen,
             Notifier* intr, const char* log)
{
    struct iovec iov;
    iov.iov_base = bp;
    iov.iov_len  = len;

    RwDataExtraArgs args;
    args.recvfrom.from    = from;
    args.recvfrom.fromlen = fromlen;
    return rwdata(RECVFROM, fd, &iov, 1, flags, -1, &args, 0, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::recvmsg(int fd, struct msghdr* msg, int flags,
            Notifier* intr, const char* log)
{
    RwDataExtraArgs args;
    args.msg_hdr = msg;
    return rwdata(RECVMSG, fd, 0, 0, flags, -1, &args, 0, intr, log);
}


//////////////////////////////////////////////////////////////////////////////
int
IO::write(int fd, const char* bp, size_t len, 
          Notifier* intr, const char* log)
{
    struct iovec iov; 
    iov.iov_base = const_cast<void*>(static_cast<const void*>(bp));
    iov.iov_len  = len;
    return rwdata(WRITEV, fd, &iov, 1, 0, -1, 0, 0, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::writev(int fd, const struct iovec* iov, int iovcnt, 
           Notifier* intr, const char* log)
{
    return rwdata(WRITEV, fd, iov, iovcnt, 0, -1, 0, 0, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::writeall(int fd, const char* bp, size_t len, 
             Notifier* intr, const char* log)
{
    struct iovec iov;
    iov.iov_base = const_cast<char*>(bp);
    iov.iov_len  = len;

    return rwvall(WRITEV, fd, &iov, 1, -1, 0, intr, "writeall", log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::writevall(int fd, const struct iovec* iov, int iovcnt,
              Notifier* intr, const char* log)
{
    return rwvall(WRITEV, fd, iov, iovcnt, -1, 0, intr, "writevall", log);
}

//////////////////////////////////////////////////////////////////////////////
int 
IO::timeout_write(int fd, char* bp, size_t len, int timeout_ms,
                  Notifier* intr, const char* log)
{
    struct iovec iov; 
    iov.iov_base = const_cast<void*>(static_cast<const void*>(bp));
    iov.iov_len  = len;
    return rwdata(WRITEV, fd, &iov, 1, 0, timeout_ms, 0, 0, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int 
IO::timeout_writev(int fd, const struct iovec* iov, int iovcnt, int timeout_ms,
                   Notifier* intr, const char* log)
{
    return rwdata(WRITEV, fd, iov, iovcnt, 0, timeout_ms, 0, 0, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int 
IO::timeout_writeall(int fd, const char* bp, size_t len, int timeout_ms,
                     Notifier* intr, const char* log)
{
    struct iovec iov;
    iov.iov_base = const_cast<char*>(bp);
    iov.iov_len  = len;

    struct timeval start;
    gettimeofday(&start, 0);    

    return rwvall(WRITEV, fd, &iov, 1, timeout_ms, &start, intr, 
                  "timeout_writeall", log);
}

//////////////////////////////////////////////////////////////////////////////
int 
IO::timeout_writevall(int fd, const struct iovec* iov, int iovcnt,
                      int timeout_ms, Notifier* intr, const char* log)
{
    struct timeval start;
    gettimeofday(&start, 0);    

    return rwvall(WRITEV, fd, iov, iovcnt, timeout_ms, &start, intr, 
                  "timeout_writevall", log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::send(int fd, const char* bp, size_t len, int flags,
         Notifier* intr, const char* log)
{    
    struct iovec iov;
    iov.iov_base = const_cast<void*>(static_cast<const void*>(bp));
    iov.iov_len  = len;
    return rwdata(SEND, fd, &iov, 1, 0, -1, 0, 0, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::sendto(int fd, char* bp, size_t len, int flags,
           const struct sockaddr* to, socklen_t tolen,
           Notifier* intr, const char* log)
{
    struct iovec iov;
    iov.iov_base = bp;
    iov.iov_len  = len;

    RwDataExtraArgs args;
    args.sendto.to    = to;
    args.sendto.tolen = tolen;

    return rwdata(SENDTO, fd, &iov, 1, flags, -1, &args, 0, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::sendmsg(int fd, const struct msghdr* msg, int flags,
            Notifier* intr, const char* log)
{
    RwDataExtraArgs args;
    args.msg_hdr = msg;

    return rwdata(SENDMSG, fd, 0, 0, flags, -1, &args, 0, intr, log);
}

//////////////////////////////////////////////////////////////////////////////
int
IO::poll(int fd, short events, short* revents, int timeout_ms, 
         Notifier* intr, const char* log)
{   
    struct timeval start;
    if (timeout_ms > 0) {
        gettimeofday(&start, 0);
    }

    int err = poll_with_notifier(intr, fd, events, revents, timeout_ms, 
                                 &start, log);
    if (log) {
        logf(log, LOG_DEBUG,
             "poll: events 0x%x timeout %d revents 0x%x cc %d",
             events, timeout_ms, ((revents == 0) ? 0 : *revents), err);
    }

    if (err == 0) {
        return 1;
    } else if (err == IOTIMEOUT) {
        return IOTIMEOUT;
    } else {
        ASSERT(err < 0);
        return err;
    }
}

//////////////////////////////////////////////////////////////////////////////
int
IO::get_nonblocking(int fd, bool *nonblockingp, const char* log)
{
    int flags = 0;
    ASSERT(nonblockingp);
    
    if ((flags = fcntl(fd, F_GETFL)) < 0) {
        if (log) log_debug(log, "get_nonblocking: fcntl GETFL err %s",
                           strerror(errno));
        return -1;
    }

    *nonblockingp = (flags & O_NONBLOCK);
    if (log) log_debug(log, "get_nonblocking: %s mode",
                       *nonblockingp ? "nonblocking" : "blocking");
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
int
IO::set_nonblocking(int fd, bool nonblocking, const char* log)
{
    int flags = 0;
    bool already = 0;
    
    if ((flags = fcntl(fd, F_GETFL)) < 0) {
        if (log) log_debug(log, "set_nonblocking: fcntl GETFL err %s",
                           strerror(errno));
        return -1;
    }
    
    if (nonblocking) {
        if (flags & O_NONBLOCK) {
            already = 1; // already nonblocking
            goto done;
        }
        flags = flags | O_NONBLOCK;
    } else {
        if (!(flags & O_NONBLOCK)) {
            already = 1; // already blocking
            goto done;
        }
            
        flags = flags & ~O_NONBLOCK;
    }
    
    if (fcntl(fd, F_SETFL, flags) < 0) {
        if (log) log_debug(log, "set_nonblocking: fcntl SETFL err %s",
                           strerror(errno));
        return -1;
    }

  done:
    if (log) log_debug(log, "set_nonblocking: %s mode %s",
                       nonblocking ? "nonblocking" : "blocking",
                       already     ? "already set" : "set");
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
int
IO::poll_with_notifier(
    Notifier*             intr,
    int                   fd,
    short                 events,
    short*                revents,
    int                   timeout,
    const struct timeval* start_time,
    const char*           log
    )
{    
    ASSERT(! (timeout > 0 && start_time == 0));
    ASSERT(timeout >= -1);

    struct pollfd poll_set[2];
    int poll_set_size = 1;

    poll_set[0].fd     = fd;
    poll_set[0].events = events;

    if (intr != 0) {
        poll_set[1].fd     = intr->read_fd();
        poll_set[1].events = POLLIN | POLLPRI | POLLERR;
        ++poll_set_size;
    }

  retry:
    int cc = ::poll(poll_set, poll_set_size, timeout);
    if (cc < 0 && errno == EINTR) {
        if (timeout > 0) {
            timeout = adjust_timeout(timeout, start_time);
        }
        goto retry;
    }

    if (cc < 0) {
        return IOERROR;
    } else if (cc == 0) {
        if (log) {
            logf(log, LOG_DEBUG, "poll_with_notifier timed out");
        }
        return IOTIMEOUT;
    } else {

#ifdef __APPLE__
        // there's some strange bug in the poll emulation
        if (cc > 2) {
            if (log) {
                logf(log, LOG_WARN,
                     "poll_with_notifier: returned bogus high value %d, "
                     "capping to 2", cc);
            }
            cc = 2;
        }
#endif

        if (log) {
            logf(log, LOG_DEBUG, 
                 "poll_with_notifier: %d fds, status {%hx,%hx}",
                 cc, poll_set[0].revents, poll_set[1].revents);
        }
        
        // Always prioritize getting data before interrupt via notifier
        if (poll_set[0].revents & events) {
            if (revents != 0) {
                *revents = poll_set[0].revents;
            }

            return 0;
        } 

        if (intr != 0 & poll_set[1].revents & POLLERR) {
            if (log) {
                logf(log, LOG_DEBUG,
                     "poll_with_notifier: error in notifier fd!");
            }

            return IOERROR; // XXX/bowei - technically this is not an
                            // error with the IO, but there should be
                            // some kind of signal here that things
                            // are not right
        } else if (intr != 0 && poll_set[1].revents & (POLLIN | POLLPRI)) {
            if (log) {
                logf(log, LOG_DEBUG,
                     "poll_with_notifier: interrupted");
            }
            intr->drain_pipe();
            return IOINTR;
        }

        if (log) {
            logf(log, LOG_DEBUG, 
                 "poll_with_notifier: something wrong with revents "
                 "- not part of watched events");
        }        
        return IOERROR;
    }
}

//////////////////////////////////////////////////////////////////////////////
int 
IO::rwdata(
    RwDataOp              op,
    int                   fd,
    const struct iovec*   iov,
    int                   iovcnt,
    int                   flags,
    int                   timeout,
    RwDataExtraArgs*      args,
    const struct timeval* start_time,
    Notifier*             intr, 
    const char*           log
    )
{
    ASSERT(! ((op == READV || op == WRITEV) && 
              (iov == 0 || flags != 0 || args != 0)));
    ASSERT(! ((op == RECV  || op == SEND) && 
              (iovcnt != 1 | args != 0)));
    ASSERT(! ((op == RECVFROM || op == SENDTO)  && 
              (iovcnt != 1 || args == 0)));
    ASSERT(! ((op == RECVMSG || op == SENDMSG) && 
              (iov != 0 && args == 0)));
    ASSERT(timeout >= -1);
    ASSERT(! (timeout > -1 && start_time == 0));

    short events;
    switch (op) {
    case READV: case RECV: case RECVFROM: case RECVMSG:
        events = POLLIN | POLLPRI | POLLERR; break;
    case WRITEV: case SEND: case SENDTO: case SENDMSG:
        events = POLLOUT | POLLERR; break;
    }
   
    int cc;
    while (true) {
        if (intr || timeout > -1) {
            cc = poll_with_notifier(intr, fd, events, 0, timeout, start_time, log);
            if (cc == IOERROR || cc == IOTIMEOUT || cc == IOINTR) {
                return cc;
            } 
        }

        switch (op) {
        case READV:
            cc = ::readv(fd, iov, iovcnt);
            if (log) logf(log, LOG_DEBUG, "::readv() fd %d cc %d", fd, cc);
            break;
        case RECV:
            cc = ::recv(fd, iov[0].iov_base, iov[0].iov_len, flags);
            if (log) logf(log, LOG_DEBUG, "::recv() fd %d %p/%u cc %d", 
                          fd, iov[0].iov_base, iov[0].iov_len, cc);
            break;
        case RECVFROM:
            cc = ::recvfrom(fd, iov[0].iov_base, iov[0].iov_len, flags,
                            args->recvfrom.from, 
                            args->recvfrom.fromlen);
            if (log) logf(log, LOG_DEBUG, "::recvfrom() fd %d %p/%u cc %d", 
                          fd, iov[0].iov_base, iov[0].iov_len, cc);
            break;
        case RECVMSG:
            cc = ::sendmsg(fd, args->msg_hdr, flags);
            if (log) logf(log, LOG_DEBUG, "::recvmsg() fd %d %p cc %d", 
                          fd, args->msg_hdr, cc);
            break;
        case WRITEV:
            cc = ::writev(fd, iov, iovcnt);
            if (log) logf(log, LOG_DEBUG, "::writev() fd %d cc %d", fd, cc);
            break;
        case SEND:
            cc = ::send(fd, iov[0].iov_base, iov[0].iov_len, flags);
            if (log) logf(log, LOG_DEBUG, "::send() fd %d %p/%u cc %d", 
                          fd, iov[0].iov_base, iov[0].iov_len, cc);
            break;
        case SENDTO:
            cc = ::sendto(fd, iov[0].iov_base, iov[0].iov_len, flags,
                          args->sendto.to, args->sendto.tolen);
            if (log) logf(log, LOG_DEBUG, "::sendto() fd %d %p/%u cc %d", 
                          fd, iov[0].iov_base, iov[0].iov_len, cc);
            break;
        case SENDMSG:
            cc = ::sendmsg(fd, args->msg_hdr, flags);
            if (log) logf(log, LOG_DEBUG, "::sendmsg() fd %d %p cc %d", 
                          fd, args->msg_hdr, cc);
            break;
        }
        
        if (cc < 0 && (errno == EAGAIN || errno == EINTR) ) {
            if (timeout > 0) {
                timeout = adjust_timeout(timeout, start_time);
            }
            continue;
        }

        if (cc < 0) {
            return IOERROR;
        } else if (cc == 0) {
            return IOEOF;
        } else {
            return cc;
        }
    } // while (true)

    NOTREACHED;
}

//////////////////////////////////////////////////////////////////////////////
int
IO::rwvall(
    RwDataOp              op,
    int                   fd,
    const struct iovec*   iov, 
    int                   iovcnt,
    int                   timeout,             
    const struct timeval* start,
    Notifier*             intr,
    const char*           fcn_name,
    const char*           log
    )
{
    ASSERT(op == READV || op == WRITEV);
    ASSERT(! (timeout != -1 && start == 0));

    COWIoVec cow_iov(iov, iovcnt);
    int total_bytes = cow_iov.bytes_left();

    while (cow_iov.bytes_left() > 0) {
	int cc = rwdata(op, fd, cow_iov.iov(), cow_iov.iovcnt(), 
		        0, timeout, 0, start, intr, log);
	if (cc < 0) {
	    if (log && cc != IOTIMEOUT && cc != IOINTR) {
		logf(log, LOG_DEBUG, "%s %s %s", 
		     fcn_name, ioerr2str(cc), strerror(errno));
	    }
	    return cc;
	} else if (cc == 0) {
            if (log) {
                logf(log, LOG_DEBUG, "%s eof", fcn_name);
            }
	    return IOEOF;
	} else {
	    cow_iov.consume(cc);
            if (log) {
                logf(log, LOG_DEBUG, "%s %d bytes %u left %d total",
                     fcn_name, cc, cow_iov.bytes_left(), total_bytes);
            }
            
            if (timeout > 0) {
                timeout = adjust_timeout(timeout, start);
            }
	}
    }

    return total_bytes;
}

//////////////////////////////////////////////////////////////////////////////
int
IO::adjust_timeout(int timeout, const struct timeval* start)
{
    struct timeval now;
    int err = gettimeofday(&now, 0);
    ASSERT(err == 0);
    
    now.tv_sec  -= start->tv_sec;
    now.tv_usec -= start->tv_usec;
    timeout -= now.tv_sec * 1000 + now.tv_usec / 1000;
    if (timeout < 0) {
        timeout = 0;
    }

    return timeout;
}

} // namespace oasys
