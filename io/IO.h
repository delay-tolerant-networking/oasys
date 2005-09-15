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

#ifndef _OASYS_IO_H_
#define _OASYS_IO_H_

#include <fcntl.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <sys/socket.h>

#include "../debug/DebugUtils.h"
#include "../thread/Notifier.h"

namespace oasys {

//! Virtually inherited base class for holding the notifier with
//! interruptable I/O
class InterruptableIO {
public:
    InterruptableIO(Notifier* intr = 0) 
        : intr_(intr) {}
    ~InterruptableIO() { delete_z(intr_); }

    Notifier* get_notifier() { 
        return intr_; 
    }

    void interrupt_from_io() {
        ASSERT(intr_ != 0);
        intr_->notify();
    }
    
    void set_notifier(Notifier* intr) { 
        ASSERT(intr_ == 0);
        intr_ = intr; 
    }
    
private:
    Notifier* intr_;
};

/**
 * Return code values for the timeout enabled functions such as
 * timeout_read() and timeout_accept(). Note that the functions return
 * an int, not an enumerated type since they may return other
 * information, e.g. the number of bytes read.
 */
enum IOTimeoutReturn_t {
    IOEOF 	= 0,	///< eof
    IOERROR 	= -1,	///< error
    IOTIMEOUT 	= -2,   ///< timeout
    IOINTR      = -3,   ///< interrupted by notifier
};


struct IO {
    //! @return Text for the io error.
    static const char* ioerr2str(int err);

    //@{
    /// System call wrappers (for logging)
    static int open(const char* path, int flags,
                    const char* log = 0);
    
    static int open(const char* path, int flags, mode_t mode,
                    const char* log = 0);
    
    static int close(int fd,
                     const char* log = 0,
                     const char* filename = "");
    
    static int unlink(const char* path, 
                      const char* log = 0);    

    static int lseek(int fd, off_t offset, int whence,
                     const char* log = 0);
    
    static int truncate(int fd, off_t length,
                        const char* log = 0);

    static int mkstemp(char* templ, const char* log = 0);
    //@}

    //! @{ XXX/bowei - more documentation
    static int read(int fd, char* bp, size_t len,
                    Notifier* intr = 0, const char* log = 0);    

    static int readv(int fd, const struct iovec* iov, int iovcnt,
                     Notifier* intr = 0, const char* log = 0);

    static int readall(int fd, char* bp, size_t len,
                       Notifier* intr = 0, const char* log = 0);

    static int readvall(int fd, const struct iovec* iov, int iovcnt,
                 Notifier* intr = 0, const char* log = 0);

    static int timeout_read(int fd, char* bp, size_t len, int timeout_ms,
                            Notifier* intr = 0, const char* log  = 0);

    static int timeout_readv(int fd, const struct iovec* iov, int iovcnt,
                             int timeout_ms, Notifier* intr = 0, 
                             const char* log = 0);

    static int timeout_readall(int fd, char* bp, size_t len, int timeout_ms,
                               Notifier* intr = 0, const char* log = 0);

    static int timeout_readvall(int fd, const struct iovec* iov, int iovcnt,
                                int timeout_ms, Notifier* intr = 0, 
                                const char* log = 0);

    static int recv(int fd, char* bp, size_t len, int flags,
                    Notifier* intr = 0,  const char* log = 0);

    static int recvfrom(int fd, char* bp, size_t len,
                        int flags, struct sockaddr* from, socklen_t* fromlen,
                        Notifier* intr = 0, const char* log = 0);

    static int recvmsg(int fd, struct msghdr* msg, int flags,
                       Notifier* intr = 0, const char* log = 0);
    
    static int write(int fd, const char* bp, size_t len,
                     Notifier* intr = 0, const char* log = 0);

    static int writev(int fd, const struct iovec* iov, int iovcnt,
                      Notifier* intr = 0, const char* log = 0);

    static int writeall(int fd, const char* bp, size_t len,
                        Notifier* intr = 0, const char* log = 0);

    static int writevall(int fd, const struct iovec* iov, int iovcnt,
                         Notifier* intr = 0, const char* log = 0);

    static int timeout_write(int fd, char* bp, size_t len, int timeout_ms,
                             Notifier* intr = 0, const char* log  = 0);

    static int timeout_writev(int fd, const struct iovec* iov, int iovcnt, 
                              int timeout_ms, Notifier* intr = 0, 
                              const char* log = 0);

    static int timeout_writeall(int fd, const char* bp, size_t len, 
                                int timeout_ms,
                                Notifier* intr = 0, const char* log = 0);

    static int timeout_writevall(int fd, const struct iovec* iov, int iovcnt,
                                 int timeout_ms, Notifier* intr = 0, 
                                 const char* log = 0);
    
    static int send(int fd, const char* bp, size_t len, int flags,
                    Notifier* intr = 0, const char* log = 0);
    
    static int sendto(int fd, char* bp, size_t len, 
                      int flags, const struct sockaddr* to, socklen_t tolen,
                      Notifier* intr = 0, const char* log = 0);

    static int sendmsg(int fd, const struct msghdr* msg, int flags,
                       Notifier* intr = 0, const char* log = 0);
    //! @}

    //! @return IOTIMEOUT, IOINTR, 1 indicates readiness, otherwise
    //! it's an error.
    static int poll_single(int fd, short events, short* revents, 
                           int timeout_ms,
                           Notifier* intr = 0, const char* log = 0);

    //! @return IOTIMEOUT, IOINTR, 1 indicates readiness, otherwise
    //! it's an error
    static int poll_multiple(struct pollfd* fds, int nfds, int timeout_ms,
                             Notifier* intr = 0, const char* log = 0);

    //! @{ Read/Write in the entire supplied buffer, potentially !
    //! requiring multiple system calls
    //! @}

    //! @{ Get and Set the file descriptor's nonblocking status
    static int get_nonblocking(int fd, bool *nonblocking,
                               const char* log = NULL);
    static int set_nonblocking(int fd, bool nonblocking,
                               const char* log = NULL);
    //! @}

    //! Poll on an fd, interruptable by the notifier.
    static int poll_with_notifier(Notifier*             intr, 
                                  struct pollfd*        fds,
                                  size_t                nfds,
                                  int                   timeout,  
                                  const struct timeval* start_time,
                                  const char*           log);
    
    //! Op code used by rwdatas()
    enum RwDataOp {
        READV = 1,
        RECV,
        RECVFROM,
        RECVMSG,
        WRITEV,
        SEND,
        SENDTO,
        SENDMSG,
    };

    //! Union used to pass extra arguments to rwdata
    union RwDataExtraArgs {
        const struct msghdr* msg_hdr;

        struct {
            const struct sockaddr* to;
            socklen_t tolen;
        } sendto;
    
        struct {
            struct sockaddr* from;
            socklen_t* fromlen;
        } recvfrom;
    };

    //! This is the do all function which will (depending on the flags
    //! given dispatch to the correct read/write/send/rcv call
    static int rwdata(RwDataOp              op, 
                      int                   fd, 
                      const struct iovec*   iov, 
                      int                   iovcnt, 
                      int                   flags, 
                      int                   timeout,
                      RwDataExtraArgs*      args,
                      const struct timeval* start_time,
                      Notifier*             intr, 
                      const char*           log);
    
    //! Do all function for iovec reading/writing
    static int rwvall(RwDataOp              op, 
                      int                   fd, 
                      const struct iovec*   iov, 
                      int                   iovcnt,
                      int                   timeout,
                      const struct timeval* start,
                      Notifier*             intr, 
                      const char*           fcn_name, 
                      const char*           log);
    
    //! Adjust the timeout value given a particular start time
    static int adjust_timeout(int timeout, const struct timeval* start);
}; // class IO

} // namespace oasys

#endif /* _OASYS_IO_H_ */

