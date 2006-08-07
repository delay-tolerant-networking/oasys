#include <config.h>
#ifdef OASYS_BLUETOOTH_ENABLED

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h> 

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/rfcomm.h>

#include "../io/IO.h"
#include "Bluetooth.h"
#include "BluetoothSocket.h"
#include "../debug/Log.h"

namespace oasys {

int BluetoothSocket::abort_on_error_ = 0;

BluetoothSocket::BluetoothSocket(int socktype,
                                 proto_t proto,
                                 const char* logbase)
    : Logger("BluetoothSocket", logbase)
{
    state_ = INIT;
    memset(&local_addr_,0,sizeof(bdaddr_t));
    channel_ = 0;
    memset(&remote_addr_,0,sizeof(bdaddr_t));
    fd_ = -1;
    socktype_ = socktype;
    proto_ = (proto_t) proto;
    logfd_ = true; 
    sa_ = NULL;
    slen_ = -1;
    reuse_addr_ = false;
    silent_connect_ = false;
}

BluetoothSocket::BluetoothSocket(int socktype, proto_t proto, int sock,
                   bdaddr_t remote_addr, u_int8_t remote_channel,
                   const char* logbase)
    : Logger("BluetoothSocket", logbase)
{
    sa_ = NULL;
    fd_ = sock;
    proto_ = (proto_t) proto;
    logpathf("%s/%s/%d",logbase,prototoa((proto_t)proto_),sock);
    socktype_ = socktype;
    state_ = ESTABLISHED;
    bacpy(&local_addr_,BDADDR_ANY);
    channel_ = remote_channel;
    set_remote_addr(remote_addr);

    silent_connect_ = false;
    configure();
}

BluetoothSocket::~BluetoothSocket()
{
    if (sa_ != NULL) delete sa_; 
    sa_ = NULL;
    close();
}

void
BluetoothSocket::init_socket()
{
    // should only be called at real init time or after a call to close()
    ASSERT(state_ == INIT || state_ == FINI);
    ASSERT(fd_ == -1);
    state_ = INIT;

    fd_ = socket(PF_BLUETOOTH, socktype_, (int) proto_);
    if (fd_ == -1) {
        logf(LOG_ERR, "error creating socket: %s", strerror(errno));
        if(errno==EBADFD) close();
        return;
    }

    if (logfd_)
        Logger::logpath_appendf("/%s/%d",
                                prototoa((proto_t)proto_),
                                fd_);

    logf(LOG_DEBUG, "created socket %d of protocol %s", fd_, 
         prototoa((proto_t)proto_));

    configure(); 
}

int
BluetoothSocket::bind(bdaddr_t local_addr, u_int8_t local_channel)
{
    if (fd_ == -1) init_socket();

    set_local_addr(local_addr);
    channel_ = local_channel;

    char buff[18];
    if (!silent_connect_)
    logf(LOG_DEBUG, "binding to %s(%d)", Bluetooth::batostr(&local_addr,buff),
         local_channel);

    init_sa((int)LOCAL);

    if(::bind(fd_,sa_,slen_) != 0) {
        log_level_t level = LOG_ERR;
        if(errno == EADDRINUSE) level = LOG_DEBUG;
        if (!silent_connect_)
        logf(level, "failed to bind to %s(%d): %s",
             Bluetooth::batostr(&local_addr_,buff), channel_,
             strerror(errno));
        if(errno==EBADFD) close();
        return -1;
    }

    return 0;
}

int
BluetoothSocket::bind()
{
    return bind(local_addr_,channel_);
}

int
BluetoothSocket::connect(bdaddr_t remote_addr, u_int8_t remote_channel)
{
    // In Bluetooth, piconets are formed by one Master
    // connecting to up to seven Slaves, simultaneously
    // The device performing connect() is Master
    if (fd_ == -1) init_socket();

    set_remote_addr(remote_addr);
    channel_ = remote_channel;

    char buff[18];
    log_debug("connecting to %s(%d)",Bluetooth::batostr(&remote_addr_,buff),
              channel_);

    init_sa((int)REMOTE);
    rc_->rc_channel=channel_;
    bacpy(&rc_->rc_bdaddr,&remote_addr);

    set_state(CONNECTING);

    if (::connect(fd_,sa_,slen_) < 0) {
        if (errno == EISCONN && !silent_connect_)
            log_debug("already connected to %s-%u",
                      Bluetooth::batostr(&remote_addr_,buff), channel_);
        else if (errno == EINPROGRESS && !silent_connect_) {
            log_debug("delayed connect to %s-%u",
                      Bluetooth::batostr(&remote_addr_,buff), channel_);
        } else if(errno==EBADFD) {
            if (!silent_connect_) log_err("EBADFD");
            close();
        } else {
            if (!silent_connect_)
                log_debug("error connecting to %s(%d): %s",
                          Bluetooth::batostr(&remote_addr_,buff), channel_,
                          strerror(errno));
        }
        return -1;
    }

    set_state(ESTABLISHED);

    return 0;
}

int
BluetoothSocket::async_connect_result()
{
    ASSERT(state_ == CONNECTING);

    int result;
    socklen_t len = sizeof(result);
    logf(LOG_DEBUG, "getting connect result");
    if (::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &result, &len) != 0) {
        logf(LOG_ERR, "error getting connect result: %s", strerror(errno));
        return errno;
    }

    if (result == 0) {
        state_ = ESTABLISHED;
    }

    return result;
}

int
BluetoothSocket::connect()
{
    return connect(remote_addr_,channel_);
}

void
BluetoothSocket::configure()
{
    ASSERT(fd_ != -1);

    if (params_.reuseaddr_) {
        int y = 1;
        logf(LOG_DEBUG, "setting SO_REUSEADDR");
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &y, sizeof y) != 0) {
            logf(LOG_WARN, "error setting SO_REUSEADDR: %s",
                 strerror(errno));
        }
    }

    if (params_.recv_bufsize_ > 0) {
        logf(LOG_DEBUG, "setting SO_RCVBUF to %d",
             params_.recv_bufsize_);
        if (::setsockopt(fd_, SOL_SOCKET, SO_RCVBUF,
                         &params_.recv_bufsize_,
                         sizeof(params_.recv_bufsize_)) < 0)
        {
            logf(LOG_WARN, "error setting SO_RCVBUF to %d: %s",
                 params_.recv_bufsize_, strerror(errno)); 
        }
    }

    if (params_.send_bufsize_ > 0) {
        logf(LOG_DEBUG, "setting SO_SNDBUF to %d",
             params_.send_bufsize_);
        if (::setsockopt(fd_, SOL_SOCKET, SO_SNDBUF,
                         &params_.send_bufsize_,
                         sizeof(params_.send_bufsize_)) < 0)
        {
            logf(LOG_WARN, "error setting SO_SNDBUF to %d: %s",
                 params_.send_bufsize_, strerror(errno)); 
        }
    }

    init_sa((int)ZERO);
}

int
BluetoothSocket::close()
{
    logf(LOG_DEBUG, "closing socket in state %s", statetoa(state_));

    if (fd_ == -1) {
        ASSERT(state_ == INIT || state_ == FINI);
        return 0;
    }

    if (::close(fd_) != 0) {
        logf(LOG_ERR, "error closing socket in state %s: %s",
                statetoa(state_), strerror(errno));
        return -1;
    }

    set_state(FINI);
    fd_ = -1;
    return 0;
}

int
BluetoothSocket::shutdown(int how)
{
    const char* howstr;

    switch (how) {
        case SHUT_RD:   howstr = "R";  break;
        case SHUT_WR:   howstr = "W";  break;
        case SHUT_RDWR: howstr = "RW"; break;

        default:
                        logf(LOG_ERR, "shutdown invalid mode %d", how);
                        return -1;
    }

    logf(LOG_DEBUG, "shutdown(%s) state %s", howstr, statetoa(state_));

    if (state_ == INIT || state_ == FINI) {
        ASSERT(fd_ == -1);
        return 0;
    }

    if (::shutdown(fd_, how) != 0) {
        logf(LOG_ERR, "error in shutdown(%s) state %s: %s",
             howstr, statetoa(state_), strerror(errno));
    } 
    if (state_ == ESTABLISHED) {
        if (how == SHUT_RD)     { set_state(RDCLOSED); }
        if (how == SHUT_WR)     { set_state(WRCLOSED); }
        if (how == SHUT_RDWR)   { set_state(CLOSED); } 

    } else if (state_ == RDCLOSED && how == SHUT_WR) {
        set_state(CLOSED);

    } else if (state_ == WRCLOSED && how == SHUT_RD) {
        set_state(CLOSED);

    } else {
        logf(LOG_ERR, "invalid state %s for shutdown(%s)",
             statetoa(state_), howstr);
        return -1;
    }

    return 0; 
}

const char*
BluetoothSocket::statetoa(state_t state)
{
    switch (state) {
        case INIT:          return "INIT";
        case LISTENING:     return "LISTENING";
        case CONNECTING:    return "CONNECTING";
        case ESTABLISHED:   return "ESTABLISHED";
        case RDCLOSED:      return "RDCLOSED";
        case WRCLOSED:      return "WRCLOSED";
        case CLOSED:        return "CLOSED";
        case FINI:          return "FINI";
    }
    ASSERT(0);
    return NULL;
}

void
BluetoothSocket::set_state(state_t state)
{
    logf(LOG_DEBUG, "state %s -> %s", statetoa(state_), 
         statetoa(state));
    state_ = state;
}

const char* 
BluetoothSocket::prototoa(proto_t proto)
{
    switch (proto) {
        case L2CAP:    return "L2CAP";
        case HCI:      return "HCI";
        case SCO:      return "SCO";
        case RFCOMM:   return "RFCOMM";
        case BNEP:     return "BNEP";
        case CMTP:     return "CMTP";
        case HIDP:     return "HIDP";
        case AVDTP:    return "AVDTP";
    }
    ASSERT(0);
    return NULL;
}

void 
BluetoothSocket::set_proto(proto_t proto)
{
    logf(LOG_DEBUG, "protocol %s -> %s", prototoa((proto_t)proto_), 
         prototoa((proto_t)proto));
    proto_ = proto;
}

int
BluetoothSocket::send(const char* bp, size_t len, int flags)
{
    return IO::send(fd_, bp, len, flags, get_notifier(), logpath_);
}

int
BluetoothSocket::recv(char* bp, size_t len, int flags)
{
    return IO::recv(fd_, bp, len, flags, get_notifier(), logpath_);
}

int
BluetoothSocket::fd()
{
    return fd_;
}

bdaddr_t
BluetoothSocket::local_addr()
{
    if (!bacmp(BDADDR_ANY,&local_addr_)) get_local();
    return local_addr_;
}

void
BluetoothSocket::local_addr(bdaddr_t& addr)
{
    if (!bacmp(&addr,&local_addr_)) get_local();
    bacpy(&addr,&local_addr_);
}

u_int8_t
BluetoothSocket::channel()
{
    if (channel_ == 0) get_local();
    return channel_;
}

bdaddr_t
BluetoothSocket::remote_addr()
{
    if (!bacmp(BDADDR_ANY,&remote_addr_)) get_remote();
    return remote_addr_;
}

void
BluetoothSocket::remote_addr(bdaddr_t& addr)
{
    if (!bacmp(&addr,&remote_addr_)) get_remote();
    bacpy(&addr,&remote_addr_);
}

void
BluetoothSocket::set_local_addr(bdaddr_t& addr)
{
    bacpy(&local_addr_,&addr);
}

void
BluetoothSocket::set_channel(u_int8_t channel)
{
    // from net/bluetooth/rfcomm/core.c
    ASSERT(channel >= 1 && channel <= 30);
    channel_ = channel;
}

void
BluetoothSocket::set_remote_addr(bdaddr_t& addr)
{
    bacpy(&remote_addr_,&addr);
}

void
BluetoothSocket::get_local()
{
    if (fd_ < 0)
        return;

    init_sa((int)ZERO);
    socklen_t slen = sizeof(struct sockaddr);
    if(::getsockname(fd_, sa_, &slen) == 0) {
        switch (proto_) { 
            case L2CAP:
            case HCI:
            case SCO:
                break; // not implemented
            case RFCOMM:
                rc_ = (struct sockaddr_rc *) sa_;
                bacpy(&local_addr_,&rc_->rc_bdaddr);
                channel_ = rc_->rc_channel;
                break;
            // not implemented
            case BNEP:
            case CMTP:
            case HIDP:
            case AVDTP:
            default:
                break;
        }
    }
}

bdaddr_t*
BluetoothSocket::sa_baddr()
{
    ASSERT(sa_ != NULL);
    switch (proto_) {
        case L2CAP:
        case HCI:
        case SCO:
            break; // not implemented
        case RFCOMM:
            rc_ = (struct sockaddr_rc*) sa_;
            return &rc_->rc_bdaddr;
        case BNEP:
        case CMTP:
        case HIDP:
        case AVDTP:
        default:
            break;
    }
    ASSERT(0);
    return NULL;
}

u_int8_t
BluetoothSocket::sa_channel()
{
    ASSERT(sa_ != NULL);
    switch (proto_) {
        case L2CAP:
        case HCI:
        case SCO:
            break; // not implemented
        case RFCOMM:
            rc_ = (struct sockaddr_rc*) sa_;
            return rc_->rc_channel;
        case BNEP:
        case CMTP:
        case HIDP:
        case AVDTP:
        default:
            break;
    }
    ASSERT(0);
    return 0;
}

void
BluetoothSocket::get_remote()
{
    if (fd_ < 0)
        return;

    init_sa((int)ZERO);
    socklen_t slen = sizeof(struct sockaddr);
    if(::getpeername(fd_, sa_, &slen) == 0) {
        switch (proto_) {
            case L2CAP:
            case HCI:
            case SCO:
                break; // not implemented
            case RFCOMM:
                rc_ = (struct sockaddr_rc *) sa_;
                bacpy(&remote_addr_,&rc_->rc_bdaddr);
                channel_ = rc_->rc_channel;
                break;
            case BNEP:
            case CMTP:
            case HIDP:
            case AVDTP:
            default:
                break;
        }
    }
}

void
BluetoothSocket::init_sa(int sa_type)
{
    if (sa_ == NULL) {
        sa_ = new sockaddr();
    }
    memset(sa_,0,sizeof(struct sockaddr));
    if( sa_type == (int) ZERO ) return;

    switch (proto_) {
        case L2CAP:
        case HCI:
        case SCO:
            break; // not implemented
        case RFCOMM:
            // from net/bluetooth/rfcomm/core.c
            ASSERT(channel_ >= 1 && channel_ <= 30);
            slen_ = sizeof(struct sockaddr_rc);
            rc_ = (struct sockaddr_rc*) sa_;
            rc_->rc_family = AF_BLUETOOTH;
            if( sa_type == (int) LOCAL ) {
                bacpy(&rc_->rc_bdaddr,&local_addr_);
                rc_->rc_channel = channel_;
            } else {
                bacpy(&rc_->rc_bdaddr,&remote_addr_);
                rc_->rc_channel = channel_;
            } 
            return;
        case BNEP:
        case CMTP:
        case HIDP:
        case AVDTP:
        default:
            break; // not implemented
    } 
    ASSERT(0);
} 

int
BluetoothSocket::poll_sockfd(int events, int* revents, int timeout_ms)
{
    short s_events = events;
    short s_revents;

    int cc = IO::poll_single(fd_, s_events, &s_revents, timeout_ms,
                             get_notifier(), logpath_);

    if (revents != 0) {
        *revents = s_revents;
    }

    return cc;
}

int RFCOMMChannel::rc_channel_ = -1;
SpinLock RFCOMMChannel::lock_ = SpinLock();

int
RFCOMMChannel::next() {
    ScopeLock l(&lock_,"RFCOMMChannel::next");
    ++rc_channel_; // starts off at zero;
    rc_channel_ %= 30; // never exceeds 29
    int next = rc_channel_ + 1;
    return next;
}

} // namespace oasys
#endif /* OASYS_BLUETOOTH_ENABLED */
