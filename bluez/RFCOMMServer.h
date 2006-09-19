#ifndef _OASYS_RFCOMM_SERVER_H_
#define _OASYS_RFCOMM_SERVER_H_

#include "config.h"

#ifdef OASYS_BLUETOOTH_ENABLED

#include "BluetoothServer.h"
#include "../thread/Thread.h"

namespace oasys {

class RFCOMMServer : public BluetoothServer {
public:
    RFCOMMServer(char* logbase = "/rfcommserver")
    : BluetoothServer(SOCK_STREAM,BluetoothSocket::RFCOMM,logbase)
    {
    }
};

class RFCOMMServerThread : public BluetoothServerThread {
public:
    RFCOMMServerThread(char * logbase = "/rfcommserver", int flags = 0)
    : BluetoothServerThread(SOCK_STREAM,
                     BluetoothSocket::RFCOMM,
                     "oasys::RFCOMMServerThread",
                     logbase,
                     flags)
    {
    }

    void set_accept_timeout(int ms) {
        accept_timeout_ = ms;
    }


    int rc_bind() {
        int res = -1;
        for (channel_ = 1; channel_ <= 30; channel_++) {
            if ((res = bind()) != 0) {

                // something is borked
                if (errno != EADDRINUSE) {
                    log_err("error binding to %s:%d: %s",
                            bd2str(local_addr_), channel_, strerror(errno));
                    if (errno == EBADFD) close();
                    return res;
                }
            } else {

                // bind succeeded
                return res;
            }

        }

        log_err("Scanned all local RFCOMM channels but unable to bind to %s",
                bd2str(local_addr_));
        return -1;
    }
};

} // namespace oasys

#endif /* OASYS_BLUETOOTH_ENABLED */
#endif /* _OASYS_RFCOMM_SERVER_H_ */
