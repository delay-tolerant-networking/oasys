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
// XXX/demmer add copyright

#include <errno.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/fcntl.h>

#include "IO.h"
#include "debug/Log.h"

int
IO::open(const char* path, int flags, const char* log)
{
    int fd = ::open(path, flags);
    if (log) {
        logf(log, LOG_DEBUG, "open %s (flags 0x%x): fd %d", path, flags, fd);
    }
    return fd;
}

int
IO::open(const char* path, int flags, mode_t mode, const char* log)
{
    int fd = ::open(path, flags, mode);
    if (log) {
        logf(log, LOG_DEBUG, "open %s (flags 0x%x mode 0x%x): fd %d",
             path, flags, mode, fd);
    }
    return fd;
}
    
int
IO::close(int fd, const char* log, const char* filename)
{
    int ret = ::close(fd);
    if (log) {
        logf(log, LOG_DEBUG, "close %s fd %d: %d", filename, fd, ret);
    }
    return ret;
}

int
IO::read(int fd, char* bp, size_t len, const char* log)
{
    int cc = ::read(fd, (void*)bp, len);
    if (log) logf(log, LOG_DEBUG, "read %d/%d", cc, len);
    return cc;
}

int
IO::readv(int fd, const struct iovec* iov, int iovcnt, const char* log)
{
    size_t total = 0;
    for (int i = 0; i < iovcnt; ++i) {
        total += iov[i].iov_len;
    }

    int cc = ::readv(fd, iov, iovcnt);
    if (log) logf(log, LOG_DEBUG, "readv %d/%d", cc, total);
    
    return cc;
}
    
int
IO::write(int fd, const char* bp, size_t len, const char* log)
{
    int cc = ::write(fd, (void*)bp, len);
    if (log) logf(log, LOG_DEBUG, "write %d/%d", cc, len);
    return cc;
}

int
IO::writev(int fd, const struct iovec* iov, int iovcnt, const char* log)
{
    size_t total = 0;
    for (int i = 0; i < iovcnt; ++i) {
        total += iov[i].iov_len;
    }

    int cc = ::writev(fd, iov, iovcnt);
    if (log) logf(log, LOG_DEBUG, "writev %d/%d", cc, total);
    
    return cc;
}

int
IO::unlink(const char* path, const char* log)
{
    int ret = ::unlink(path);
    if (log) {
        logf(log, LOG_DEBUG, "unlink %s: %d", path, ret);
    }
    return ret;
}

int
IO::lseek(int fd, off_t offset, int whence, const char* log)
{
    int cc = ::lseek(fd, offset, whence);
    if (log) {
        logf(log, LOG_DEBUG, "lseek %u %s -> %d",
             (size_t)offset,
             (whence == SEEK_SET) ? "SEEK_SET" :
             (whence == SEEK_CUR) ? "SEEK_CUR" :
             (whence == SEEK_END) ? "SEEK_END" :
             "SEEK_INVALID",
             cc);
    }
    return cc;
}

int
IO::send(int fd, const char* bp, size_t len, int flags,
         const char* log)
{
    int cc = ::send(fd, (void*)bp, len, flags);
    if (log) logf(log, LOG_DEBUG, "send %d/%d", cc, len);
    return cc;
}

int
IO::sendto(int fd, char* bp, size_t len, int flags,
           const struct sockaddr* to, socklen_t tolen,
           const char* log)
{
    int cc = ::sendto(fd, (void*)bp, len, flags, to, tolen);
    if (log) logf(log, LOG_DEBUG, "sendto %d/%d", cc, len);
    return cc;
}

int
IO::sendmsg(int fd, const struct msghdr* msg, int flags,
            const char* log)
{
    int cc = ::sendmsg(fd, msg, flags);
    if (log) logf(log, LOG_DEBUG, "sendmsg: %d", cc);
    return cc;
}

int
IO::recv(int fd, char* bp, size_t len, int flags,
            const char* log)
{
    int cc = ::recv(fd, (void*)(bp), len, flags);
    if (log) logf(log, LOG_DEBUG, "recv %d/%d", cc, len);
    return cc;
}

int
IO::recvfrom(int fd, char* bp, size_t len, int flags,
             struct sockaddr* from, socklen_t* fromlen,
             const char* log)
{
    int cc = ::recvfrom(fd, (void*)bp, len, flags, from, fromlen);
    if (log) logf(log, LOG_DEBUG, "recvfrom %d/%d", cc, len);
    return cc;
}

int
IO::recvmsg(int fd, struct msghdr* msg, int flags,
            const char* log)
{
    int cc = ::recvmsg(fd, msg, flags);
    if (log) logf(log, LOG_DEBUG, "recvmsg: %d", cc);
    return cc;
}

int
IO::poll(int fd, int events, int* revents, int timeout_ms, const char* log)
{
    int cc;
    struct pollfd pollfd;
    
    pollfd.fd = fd;
    pollfd.events = events;
    pollfd.revents = 0;

    cc = ::poll(&pollfd, 1, timeout_ms);

    if (log)
        logf(log, LOG_DEBUG,
             "poll: events 0x%x timeout %d revents 0x%x cc %d",
             events, timeout_ms, pollfd.revents, cc);
                  
    if (cc < 0) {
        if (log && errno != EINTR)
            logf(log, LOG_ERR, "error in poll: %s", strerror(errno));
        return -1;
    }

    
    if (revents)
        *revents = pollfd.revents;
    
    return cc; // 0 or 1
}

int
IO::rwall(rw_func_t rw, int fd, char* bp, size_t len, const char* log)
{
    int cc, done = 0;
    do {
        cc = (*rw)(fd, bp, len);
        if (cc < 0) {
            if (errno == EAGAIN) continue;
            return cc;
        }
        
        if (cc == 0)
            return done;
        
        done += cc;
        bp += cc;
        len -= cc;
        
    } while (len > 0);

    return done;
}

int
IO::readall(int fd, char* bp, size_t len, const char* log)
{
    int cc = rwall(::read, fd, bp, len, log);
    if (log) logf(log, LOG_DEBUG, "readall %d/%d", cc, len);
    return cc;
}

int
IO::writeall(int fd, const char* bp, size_t len, const char* log)
{
    int cc = rwall((rw_func_t)::write, fd, (char*)bp, len, log);
    if (log) logf(log, LOG_DEBUG, "writeall %d/%d", cc, len);
    return cc;
}

int
IO::rwvall(rw_vfunc_t rw, int fd, const struct iovec* const_iov, int iovcnt,
           const char* log_func, const char* log)
{
    struct iovec* iov;
    struct iovec  static_iov[16];
    struct iovec* dynamic_iov = NULL;

    if (iovcnt <= 16) {
        iov = static_iov;
    } else {
        // maybe this shouldn't be logged at level warning, but for
        // now, keep it as such since it probably won't ever be an
        // issue and if it is, we can always demote the level later
        logf(log, LOG_WARN, "%s required to malloc since iovcnt is %d",
             log_func, iovcnt);
        dynamic_iov = (struct iovec*)malloc(sizeof(struct iovec) * iovcnt);
        iov = dynamic_iov;
    }
    
    memcpy(iov, const_iov, sizeof(struct iovec) * iovcnt);
    
    int cc, total = 0, done = 0;
    for (int i = 0; i < iovcnt; ++i) {
        total += iov[i].iov_len;
    }

    if (log) logf(log, LOG_DEBUG, "%s cnt %d, total %d",
                  log_func, iovcnt, total);
    
    while (1)
    {
        cc = (*rw)(fd, iov, iovcnt);
        if (cc < 0) {
            done = cc;
            goto done;
        }

        if (cc == 0) {
            goto done;
        }
        
        done += cc;
        total -= cc;

        if (total == 0) break; // all done
        
        /**
         * Advance iov past any completed chunks, then adjust iov_base
         * and iov_cnt for the partially completed one.
         */
        while (cc >= (int)iov[0].iov_len)
        {
            if (log) logf(log, LOG_DEBUG, "%s skipping all %d of %p", log_func,
                          iov[0].iov_len, iov[0].iov_base);
            cc -= iov[0].iov_len;
            iov++;
            iovcnt--;
        }
            
        if (log) logf(log, LOG_DEBUG, "%s skipping %d of %p -> %p", log_func,
                      cc, iov[0].iov_base, (((char*)iov[0].iov_base) + cc));
        
        iov[0].iov_base = (((char*)iov[0].iov_base) + cc);
        iov[0].iov_len  -= cc;
    }

 done:
    if (dynamic_iov != NULL)
        free(dynamic_iov);
    
    return done;
}

int
IO::readvall(int fd, const struct iovec* iov, int iovcnt,
             const char* log)
{
    return rwvall(::readv, fd, iov, iovcnt, "readvall", log);

}

int
IO::writevall(int fd, const struct iovec* iov, int iovcnt,
              const char* log)
{
    return rwvall(::writev, fd, iov, iovcnt, "writevall", log);
}

/**
 * Implement a blocking read with a timeout. This is really done by
 * first calling poll() and then calling read if there was actually
 * something to do.
 */
int
IO::timeout_read(int fd, char* bp, size_t len, int timeout_ms,
                 const char* log)
{
    ASSERT(timeout_ms >= 0);
    
    int cc = poll(fd, POLLIN | POLLPRI, NULL, timeout_ms, log);
    if (cc < 0)
        return IOERROR;

    if (cc == 0) {
        if (log) logf(log, LOG_DEBUG, "poll timed out");
        return IOTIMEOUT;
    }
    
    ASSERT(cc == 1);
    
    cc = read(fd, bp, len);
    
    if (cc < 0) {
        if (log) logf(log, LOG_ERR, "timeout_read error: %s", strerror(errno));
        return IOERROR;
    }

    if (cc == 0) {
        return IOEOF;
    }

    return cc;
}

int
IO::timeout_readv(int fd, const struct iovec* iov, int iovcnt, int timeout_ms,
                  const char* log)
{
    ASSERT(timeout_ms >= 0);
    
    int cc = poll(fd, POLLIN | POLLPRI, NULL, timeout_ms, log);
    if (cc < 0)
        return IOERROR;

    if (cc == 0) {
        if (log) logf(log, LOG_DEBUG, "poll timed out");
        return IOTIMEOUT;
    }
    
    ASSERT(cc == 1);
    
    cc = ::readv(fd, iov, iovcnt);

    if (cc < 0) {
        if (log) logf(log, LOG_ERR, "timeout_readv error: %s",
                      strerror(errno));
        return IOERROR;
    }

    if (cc == 0) {
        return IOEOF;
    }

    return cc;
}

int
IO::timeout_readall(int fd, char* bp, size_t len, int timeout_ms,
                    const char* log)
{
    ASSERT(timeout_ms >= 0);
    int cc;
    int total = 0;
    while (len > 0) {
        cc = timeout_read(fd, bp, len, timeout_ms, log);
        if (cc <= 0)
            return cc;

        total += cc;

        if (cc == (int)len) {
            return total;
            
        } else {
            bp  += cc;
            len -= cc;
        }
    }

    NOTREACHED;
}

int
IO::timeout_readvall(int fd, const struct iovec* iov, int iovcnt,
                     int timeout_ms, const char* log)
{
    // XXX/demmer 
    NOTIMPLEMENTED;
}

int
IO::set_nonblocking(int fd, bool nonblocking)
{
    int flags = 0;
    
    if ((flags = fcntl(fd, F_GETFL)) < 0) {
        return -1;
    }
    
    if (nonblocking) {
        if (flags & O_NONBLOCK) return 1; // already nonblocking
        flags = flags | O_NONBLOCK;
    } else {
        if (!(flags & O_NONBLOCK)) return 1; // already blocking
        flags = flags & ~O_NONBLOCK;
    }
    
    if (fcntl(fd, F_SETFL, flags) < 0) {
        return -1;
    }
    
    return 0;
}
