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
};

} // namespace oasys

#endif /* OASYS_BLUETOOTH_ENABLED */
#endif /* _OASYS_RFCOMM_SERVER_H_ */
