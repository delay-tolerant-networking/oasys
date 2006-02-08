/* $Id$ */
#ifndef _OASYS_BT_CLIENT_H_
#define _OASYS_BT_CLIENT_H_

#include <config.h> 
#ifdef OASYS_BLUETOOTH_ENABLED

#include "BluetoothSocket.h"
#include "../io/IOClient.h"

namespace oasys {

/**
 * Base class that unifies the BluetoothSocket and IOClient interfaces.
 */
class BluetoothClient : public BluetoothSocket, public IOClient {
public:
    BluetoothClient(int socktype, BluetoothSocket::proto_t proto,
                    const char* logbase, Notifier* intr=0);
    BluetoothClient(int socktype, BluetoothSocket::proto_t proto, int fd,
                    bdaddr_t remote_addr, u_int8_t remote_channel,
                    const char* logbase,Notifier* intr=0);
    virtual ~BluetoothClient();
    
    // Virtual from IOClient
    virtual int read(char* bp, size_t len);
    virtual int write(const char* bp, size_t len);
    virtual int readv(const struct iovec* iov, int iovcnt);
    virtual int writev(const struct iovec* iov, int iovcnt);
    
    virtual int readall(char* bp, size_t len);
    virtual int writeall(const char* bp, size_t len);
    virtual int readvall(const struct iovec* iov, int iovcnt);
    virtual int writevall(const struct iovec* iov, int iovcnt);
    
    virtual int timeout_read(char* bp, size_t len, int timeout_ms);
    virtual int timeout_readv(const struct iovec* iov, int iovcnt,
                              int timeout_ms);
    virtual int timeout_readall(char* bp, size_t len, int timeout_ms);
    virtual int timeout_readvall(const struct iovec* iov, int iovcnt,
                                 int timeout_ms);

    virtual int get_nonblocking(bool *nonblockingp);
    virtual int set_nonblocking(bool nonblocking);
};

} // namespace oasys

#endif /* OASYS_BLUETOOTH_ENABLED */
#endif /* _OASYS_BT_CLIENT_H_ */
