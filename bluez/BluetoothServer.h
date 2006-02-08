/* $Id$ */
#ifndef _OASYS_BT_SERVER_H_
#define _OASYS_BT_SERVER_H_

#include <config.h> 
#ifdef OASYS_BLUETOOTH_ENABLED

#include "BluetoothSocket.h"
#include "../thread/Thread.h"

namespace oasys {

/**
 * \class BluetoothServer
 *
 * Wrapper class for a bluetooth server socket.
 */
class BluetoothServer : public BluetoothSocket {
public:
    BluetoothServer(int socktype, BluetoothSocket::proto_t proto,
             char* logbase = "/btserver");

    //@{
    /// System call wrapper
    int listen();
    int accept(int *fd, bdaddr_t *addr, u_int8_t *channel);
    //@}

    /**
     * @brief Try to accept a new connection, but don't block for more
     * than the timeout milliseconds.
     *
     * @return 0 on timeout, 1 on success, -1 on error
     */
    int timeout_accept(int *fd, bdaddr_t *addr, u_int8_t *channel,
                       int timeout_ms);
};

/**
 * \class BluetoothServerThread
 *
 * Simple class that implements a thread of control that loops,
 * blocking on accept(), and issuing the accepted() callback when new
 * connections arrive.
 */
class BluetoothServerThread : public BluetoothServer, public Thread {
public:
    BluetoothServerThread(int socktype, BluetoothSocket::proto_t proto,
                          const char* name,
                          char* logbase = "/btserver", int flags = 0,
                          int accept_timeout = -1)
        : BluetoothServer(socktype,proto,logbase), Thread(name,flags),
          accept_timeout_(accept_timeout)
    {
    }
    
    /**
     * Virtual callback hook that gets called when new connections
     * arrive.
     */
    virtual void accepted(int fd, bdaddr_t addr, u_int8_t channel) = 0;

    /**
     * Loop forever, issuing blocking calls to BluetoothServer::accept(),
     * then calling the accepted() function when new connections
     * arrive
     * 
     * Note that unlike in the Thread base class, this run() method is
     * public in case we don't want to actually create a new thread
     * for this guy, but instead just want to run the main loop.
     */
    void run();

    /**
     * @brief Bind to an address, open the channel for listening and
     * start the thread.
     *
     * Most uses of BluetoothServerThread will simply call these functions
     * in sequence, so this helper function is to merge such
     * redundancy.
     *
     * @return -1 on error, 0 otherwise.
     */
    int bind_listen_start(bdaddr_t local_addr, u_int8_t local_channel);
protected:
    /// If not -1, then call timeout_accept in the main loop. This
    /// gives a caller a (rough) way to stop the thread by calling
    /// set_should_stop() and then waiting for the accept call to
    /// timeout, at which point the bit will be checked.
    int accept_timeout_;
};

} // namespace oasys
                        
#endif /* OASYS_BLUETOOTH_ENABLED */
#endif /* _OASYS_BT_SERVER_H_ */
