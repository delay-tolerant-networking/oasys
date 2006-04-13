/* $Id$ */
#ifndef _OASYS_BT_SOCKET_H_
#define _OASYS_BT_SOCKET_H_

#include <config.h> 
#ifdef OASYS_BLUETOOTH_ENABLED

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/rfcomm.h>

#include "../io/IO.h"
#include "Bluetooth.h"
#include "../debug/Log.h"

namespace oasys {

#ifndef BDADDR_ANY
#define BDADDR_ANY   (&(bdaddr_t) {{0, 0, 0, 0, 0, 0}})
#endif

/**
 * BluetoothSocket is a base class that wraps around a Bluetooth socket. It 
 * is a base class for RFCOMMClient (possibly others to follow?).
 */ 
class BluetoothSocket : public Logger,
                        virtual public IOHandlerBase {
public:
    /**
      * from <bluetooth/bluetooth.h>:
      * #define BTPROTO_L2CAP   0
      * #define BTPROTO_HCI     1
      * #define BTPROTO_SCO     2
      * #define BTPROTO_RFCOMM  3
      * #define BTPROTO_BNEP    4
      * #define BTPROTO_CMTP    5
      * #define BTPROTO_HIDP    6
      * #define BTPROTO_AVDTP   7
      */
    enum proto_t {
        L2CAP=0,
        HCI,
        SCO,
        RFCOMM,
        BNEP,
        CMTP,
        HIDP,
        AVDTP
    };

    BluetoothSocket(int socktype, proto_t proto, const char* logbase);
    BluetoothSocket(int socktype, proto_t proto, int fd, bdaddr_t remote_addr,
                    u_int8_t channel, const char* logbase);
    virtual ~BluetoothSocket();

    /// Set the socket parameters
    void configure();

    //@{
    /// System call wrappers
    virtual int bind(bdaddr_t local_addr, u_int8_t channel);
    virtual int bind();
    virtual int connect(bdaddr_t remote_addr, u_int8_t channel);
    virtual int connect();
    virtual int close();
    virtual int shutdown(int how);

    virtual int send(const char* bp, size_t len, int flags);
    virtual int recv(char* bp, size_t len, int flags);

    /// Wrapper around poll() for this socket's fd
    virtual int poll_sockfd(int events, int* revents, int timeout_ms);

    //@}

    /// Socket State values
    enum state_t {
        INIT,           ///< initial state
        LISTENING,      ///< server socket, called listen()
        CONNECTING,     ///< client socket, called connect()
        ESTABLISHED,    ///< connected socket, data can flow
        RDCLOSED,       ///< shutdown(SHUT_RD) called, writing still enabled
        WRCLOSED,       ///< shutdown(SHUT_WR) called, reading still enabled
        CLOSED,         ///< shutdown called for both read and write
        FINI            ///< close() called on the socket
    };

    enum sockaddr_t {
        ZERO,
        LOCAL,
        REMOTE
    };

    /**
      * Return the current state.
      */
    state_t state() { return state_; }

    /// The socket file descriptor
    int fd();

    /// The local address that the socket is bound to
    void local_addr(bdaddr_t& addr);
    bdaddr_t local_addr();

    /// The channel that the socket is bound to
    u_int8_t channel();

    /// The remote address that the socket is bound to
    void remote_addr(bdaddr_t& addr);
    bdaddr_t remote_addr();

    /// Set the local address that the socket is bound to
    void set_local_addr(bdaddr_t& addr);

    /// Set the remote address that the socket is bound to
    void set_remote_addr(bdaddr_t& addr);

    /// Set the channel that the socket is bound to
    void set_channel(u_int8_t channel);

    // logfd can be set to false to disable the appending of the
    /// socket file descriptor
    void set_logfd(bool logfd) { logfd_ = logfd; }

    bool reuse_addr() { return reuse_addr_; }
    void reuse_addr(bool b) { reuse_addr_ = b; }
protected:
    void init_socket(); 
    void init_sa(int zero=(int)ZERO); 
    void set_state(state_t state);
    const char* statetoa(state_t state); 
    void set_proto(proto_t proto);
    const char* prototoa(proto_t proto); 
    void get_local();
    void get_remote(); 
    bdaddr_t* sa_baddr();
    u_int8_t sa_channel();

    static int abort_on_error_; 
    int fd_;
    int socktype_;
    state_t state_;
    int proto_;
    bool logfd_; 
    bdaddr_t local_addr_;
    bdaddr_t remote_addr_;
    u_int8_t channel_; 
    struct sockaddr* sa_;
    int slen_;
    struct sockaddr_rc* rc_;  /* BTPROTO_RFCOMM */
    bool reuse_addr_;
};

} // namespace oasys

#endif // OASYS_BLUETOOTH_ENABLED
#endif // _OASYS_BT_SOCKET_H_
