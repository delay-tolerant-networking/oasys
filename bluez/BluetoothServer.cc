
#include <config.h>
#ifdef OASYS_BLUETOOTH_ENABLED

#include "Bluetooth.h"
#include "BluetoothServer.h"
#include "debug/Log.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace oasys {

BluetoothServer::BluetoothServer(int socktype,
                                 BluetoothSocket::proto_t proto,
                                 char* logbase)
    : BluetoothSocket(socktype, proto, logbase)
{
}

int
BluetoothServer::listen()
{
    // In Bluetooth, piconets are formed by one Master
    // connecting to up to seven Slaves.  The device
    // performing connect() is the Master ... thus the
    // device performing listen()/accept() is the Slave.
    logf(LOG_DEBUG, "listening");
    ASSERT(fd_ != -1);

    if (::listen(fd_, SOMAXCONN) == -1) {
        logf(LOG_ERR, "error in listen(): %s",strerror(errno));
        return -1;
    }
    
    set_state(LISTENING);
    return 0;
}
    
int
BluetoothServer::accept(int *fd, bdaddr_t *addr, u_int8_t *channel)
{
    ASSERT(state_ == LISTENING);
    
    init_sa(BluetoothSocket::ZERO); // zero out sockaddr
    int ret = ::accept(fd_,sa_,(socklen_t*)&slen_);
    if (ret == -1) {
        if (errno != EINTR)
            logf(LOG_ERR, "error in accept(): %s [%s:%d]",
                 strerror(errno),
                 __FILE__,__LINE__);
        return ret;
    }
    
    *fd = ret;
    bacpy(addr,sa_baddr());
    *channel = sa_channel();

    monitor(IO::ACCEPT, 0); // XXX/bowei

    return 0;
}

int
BluetoothServer::timeout_accept(int *fd, bdaddr_t *addr,
                                u_int8_t *channel, int timeout_ms)
{
    int ret = poll_sockfd(POLLIN, NULL, timeout_ms);

    if (ret != 1) return ret;
    ASSERT(ret == 1);

    ret = accept(fd, addr, channel);

    if (ret < 0) {
        return IOERROR;
    }

    monitor(IO::ACCEPT, 0); // XXX/bowei

    return 0;
}

void
BluetoothServerThread::run()
{
    int fd;
    bdaddr_t addr;
    u_int8_t channel;

    while (1) {
        // check if someone has told us to quit by setting the
        // should_stop flag. if so, we're all done.
        if (should_stop())
            break;

        // check the accept_timeout parameter to see if we should
        // block or poll when calling accept
        int ret;
        if (accept_timeout_ == -1) {
            ret = accept(&fd, &addr, &channel);
        } else {
            ret = timeout_accept(&fd, &addr, &channel, accept_timeout_);
            if (ret == IOTIMEOUT)
                continue;
        }

        if (ret != 0) {
            if (errno == EINTR || ret == IOINTR) 
                continue;

            logf(LOG_ERR, "error in accept() [%d]: %d %s [%s:%d]",
                 ret, errno, strerror(errno),
                 __FILE__,__LINE__);
            close();

            ASSERT(errno != 0);

            break;
        }
        
        char buff[18];
        logf(LOG_DEBUG, "accepted connection fd %d from %s(%d)",
             fd, Bluetooth::batostr(&addr,buff), channel);

        accepted(fd, addr, channel);
    }
}

int
BluetoothServerThread::bind_listen_start(bdaddr_t local_addr,
                                  u_int8_t local_channel)
{
    if(bind(local_addr, local_channel) != 0)
        return -1;

    if(listen() != 0)
        return -1;

    start();

    return 0;
}

} // namespace oasys
#endif /* OASYS_BLUETOOTH_ENABLED */
