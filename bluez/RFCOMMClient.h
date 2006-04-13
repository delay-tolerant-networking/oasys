#ifndef _OASYS_RFCOMM_CLIENT_H_
#define _OASYS_RFCOMM_CLIENT_H_

#include "config.h"

#ifdef OASYS_BLUETOOTH_ENABLED

#include "BluetoothClient.h"

namespace oasys {

class RFCOMMClient : public BluetoothClient {
public:
    RFCOMMClient(const char* logbase = "/rfcommclient")
        : BluetoothClient(SOCK_STREAM,BluetoothSocket::RFCOMM,logbase)
    {
    }
    RFCOMMClient(int fd, bdaddr_t remote_addr, u_int8_t remote_channel,
                 const char* logbase = "/rfcommclient")
        : BluetoothClient(SOCK_STREAM,BluetoothSocket::RFCOMM,fd,remote_addr,
                   remote_channel,logbase)

    {
    }
    /**
     * Since RFCOMM channels only range from [1 .. 30], scan the whole
     * space for an available channel on the remote Bluetooth host
     */
    int rc_connect(bdaddr_t remote_addr);
    int rc_connect();
};

}  // namespace oasys

#endif /* OASYS_BLUETOOTH_ENABLED */
#endif /* _OASYS_RFCOMM_CLIENT_H_ */
