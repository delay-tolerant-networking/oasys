#ifndef _UDP_CLIENT_H_
#define _UDP_CLIENT_H_

#include "IPClient.h"

/**
 * Wrapper class for a udp client socket.
 */
class UDPClient : public IPClient {
public:
    UDPClient(const char* logbase = "/udpclient");
};

#endif /* _UDP_CLIENT_H_ */
