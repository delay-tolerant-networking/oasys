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
