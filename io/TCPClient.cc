// XXX/demmer add copyright

#include <stdlib.h>
#include <sys/poll.h>

#include "TCPClient.h"
#include "NetUtils.h"
#include "debug/Debug.h"
#include "debug/Log.h"

TCPClient::TCPClient(const char* logbase)
    : IPClient(logbase, SOCK_STREAM)
{
}

TCPClient::TCPClient(int fd, in_addr_t remote_addr, u_int16_t remote_port,
                     const char* logbase)
    : IPClient(fd, remote_addr, remote_port, logbase)
{
}

int
TCPClient::timeout_connect(in_addr_t remote_addr, u_int16_t remote_port,
                           int timeout_ms, int* errp)
{
    int ret, err;
    socklen_t len = sizeof(err);

    if (fd_ == -1) init_socket();

    if (IO::set_nonblocking(fd_, true) < 0) {
        log_err("error setting fd %d to nonblocking: %s",
                fd_, strerror(errno));
        if (errp) *errp = errno;
        return IOERROR;
    }
    
    ret = IPSocket::connect(remote_addr, remote_port);
    
    if (ret == 0)
    {
        log_debug("timeout_connect: succeeded immediately");
        if (errp) *errp = 0;
    }
    else if (ret < 0 && errno != EINPROGRESS)
    {
        log_err("timeout_connect: error from connect: %s",
                strerror(errno));
        if (errp) *errp = errno;
        ret = IOERROR;
    }
    else
    {
        ASSERT(errno == EINPROGRESS);
        log_debug("EINPROGRESS from connect(), calling poll()");
        ret = IO::poll(fd_, POLLOUT, NULL, timeout_ms, logpath_);
        
        if (ret < 0) {
            log_err("error in poll(): %s", strerror(errno));
            if (errp) *errp = errno;
            ret = IOERROR;
        }
        else if (ret == 0)
        {
            log_debug("timeout_connect: poll timeout");
            ret = IOTIMEOUT;
        }
        else
        {
            ASSERT(ret == 1);
            // call getsockopt() to see if connect succeeded
            ret = getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len);
            ASSERT(ret == 0);
            if (err == 0) {
                log_debug("return from poll, connect succeeded");
                ret = 0;
            } else {
                log_debug("return from poll, connect failed");
                ret = IOERROR;
            }
        }
    }

    // always make sure to set the fd back to blocking
    if (IO::set_nonblocking(fd_, false) < 0) {
        log_err("error setting fd %d back to blocking: %s",
                fd_, strerror(errno));
        if (errp) *errp = errno;
        return IOERROR;
    }

    return ret;
}
