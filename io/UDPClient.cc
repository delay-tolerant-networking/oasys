#include <stdlib.h>
#include <sys/poll.h>

#include "UDPClient.h"
#include "NetUtils.h"
#include "debug/Debug.h"
#include "debug/Log.h"

UDPClient::UDPClient(const char* logbase)
    : IPClient(logbase, SOCK_DGRAM)
{
}

UDPClient::UDPClient(int fd, in_addr_t remote_addr, u_int16_t remote_port,
                     const char* logbase)
    : IPClient(fd, remote_addr, remote_port, logbase)
{
}

int
UDPClient::internal_connect(in_addr_t remote_addr, u_int16_t remote_port)
{
    int ret;
    if (fd_ == -1) init_socket();
    
    remote_addr_ = remote_addr;
    remote_port_ = remote_port;

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = remote_addr;
    sa.sin_port = htons(remote_port);
    
    set_state(CONNECTING);
    
    log_debug("connecting to %s:%d", intoa(remote_addr), remote_port);

    if ((ret = ::connect(fd_, (struct sockaddr*)&sa, sizeof(sa))) < 0) {
        return -1;
    }

    set_state(ESTABLISHED);

    return 0;
}

int
UDPClient::connect(in_addr_t remote_addr, u_int16_t remote_port)
{
    int ret = internal_connect(remote_addr, remote_port);
    if (ret < 0) {
        log_err("error connecting to %s:%d: %s",
                intoa(remote_addr), remote_port, strerror(errno));
    }
    return ret;
}


