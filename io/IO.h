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

#ifndef _IO_H_
#define _IO_H_

#include <fcntl.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <sys/socket.h>

/**
 * Return code values for the timeout enabled functions such as
 * timeout_read() and timeout_accept(). Note that the functions return
 * an int, not an enumerated type since they may return other
 * information, e.g. the number of bytes read.
 */
enum IOTimeoutReturn_t {
    IOEOF 	= 0,	/* eof */
    IOERROR 	= -1,	/* error */
    IOTIMEOUT 	= -2	/* timeout */
};

/**
 * Static class (never instantiated) that provides simple wrappers for
 * system calls as well as more complicated primitives to deal with
 * short read/write operations and timeouts.
 */
class IO {
public:
    //@{
    /// System call wrappers (for logging)
    static int open(const char* path, int flags,
                    const char* log = NULL);

    static int open(const char* path, int flags, mode_t mode,
                    const char* log = NULL);

    static int close(int fd,
                     const char* log = NULL,
                     const char* filename = "");
    
    static int read(int fd, char* bp, size_t len,
                    const char* log = NULL);
    
    static int readv(int fd, const struct iovec* iov, int iovcnt,
                     const char* log = NULL);
    
    static int write(int fd, const char* bp, size_t len,
                     const char* log = NULL);
    
    static int writev(int fd, const struct iovec* iov, int iovcnt,
                      const char* log = NULL);

    static int unlink(const char* path, 
                      const char* log = NULL);
    
    static int lseek(int fd, off_t offset, int whence,
                     const char* log = NULL);
    
    static int send(int fd, const char* bp, size_t len, int flags,
                    const char* log = NULL);
    
    static int sendto(int fd, char* bp, size_t len, int flags,
                      const struct sockaddr* to, socklen_t tolen,
                      const char* log = NULL);
                      
    static int sendmsg(int fd, const struct msghdr* msg, int flags,
                       const char* log = NULL);
    
    static int recv(int fd, char* bp, size_t len, int flags,
                    const char* log = NULL);
    
    static int recvfrom(int fd, char* bp, size_t len, int flags,
                        struct sockaddr* from, socklen_t* fromlen,
                        const char* log = NULL);
    
    static int recvmsg(int fd, struct msghdr* msg, int flags,
                       const char* log = NULL);
    
    //@}
    
    /// Wrapper around poll() for a single fd
    /// @return -1 for error, 0 or 1 to indicate readiness
    static int poll(int fd, int events, int* revents, int timeout_ms,
                    const char* log = NULL);
    
    //@{
    /// Fill in the entire supplied buffer, potentially
    /// requiring multiple calls to read().
    static int readall(int fd, char* bp, size_t len,
                       const char* log = NULL);

    static int readvall(int fd, const struct iovec* iov, int iovcnt,
                        const char* log = NULL);
    //@}
    
    //@{
    /// Write out the entire supplied buffer, potentially
    /// requiring multiple calls to write().
    static int writeall(int fd, const char* bp, size_t len,
                        const char* log = NULL);

    static int writevall(int fd, const struct iovec* iov, int iovcnt,
                         const char* log = NULL);
    //@}

    //@{
    /**
     * @brief Try to read or recv the specified number of bytes, but
     * don't block for more than timeout milliseconds.
     *
     * @return the number of bytes read or the appropriate
     * IOTimeoutReturn_t code
     */
    static int timeout_read(int fd, char* bp, size_t len, int timeout_ms,
                            const char* log = NULL);
    
    static int timeout_readv(int fd, const struct iovec* iov, int iovcnt,
                             int timeout_ms, const char* log = NULL);
    static int timeout_readall(int fd, char* bp, size_t len,
                               int timeout_ms, const char* log = NULL);
    static int timeout_readvall(int fd, const struct iovec* iov, int iovcnt,
                                int timeout_ms, const char* log = NULL);

    //@}
    
    /// Set the file descriptor's nonblocking status
    static int set_nonblocking(int fd, bool nonblocking);
    
private:
    IO();  // don't ever instantiate

    typedef ssize_t(*rw_func_t)(int, void*, size_t);
    typedef ssize_t(*rw_vfunc_t)(int, const struct iovec*, int);

    static int rwall(rw_func_t rw, int fd, char* bp, size_t len,
                     const char* log);
    
    static int rwvall(rw_vfunc_t rw, int fd,
                      const struct iovec* iov, int iovcnt,
                      const char* log_func, const char* log);
};

#endif /* _IO_H_ */

