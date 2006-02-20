/* $Id$ */

#include <config.h>
#ifdef OASYS_BLUETOOTH_ENABLED

#include <stdlib.h>
#include <errno.h>

extern int errno;

#include "BluetoothClient.h"

namespace oasys {

BluetoothClient::BluetoothClient(int socktype, BluetoothSocket::proto_t proto,
                                 const char* logbase, Notifier* intr)
    : IOHandlerBase(intr),
      BluetoothSocket(socktype, proto, logbase)
{
}

BluetoothClient::BluetoothClient(int socktype, BluetoothSocket::proto_t proto,
                                 int fd, bdaddr_t remote_addr,
                                 u_int8_t remote_channel,
                                 const char* logbase, Notifier* intr)
    : IOHandlerBase(intr),
      BluetoothSocket(socktype, proto, fd, remote_addr,
                      remote_channel, logbase)
{
}

BluetoothClient::~BluetoothClient()
{
}

int
BluetoothClient::read(char* bp, size_t len)
{
    return IO::read(fd_, bp, len, get_notifier(), logpath_);
}

int
BluetoothClient::readv(const struct iovec* iov, int iovcnt)
{
    return IO::readv(fd_, iov, iovcnt, get_notifier(), logpath_);
}

int
BluetoothClient::write(const char* bp, size_t len)
{
    return IO::write(fd_, bp, len, get_notifier(), logpath_);
}

int
BluetoothClient::writev(const struct iovec* iov, int iovcnt)
{
    return IO::writev(fd_, iov, iovcnt, get_notifier(), logpath_);
}

int
BluetoothClient::readall(char* bp, size_t len)
{
    return IO::readall(fd_, bp, len, get_notifier(), logpath_);
}

int
BluetoothClient::writeall(const char* bp, size_t len)
{
    return IO::writeall(fd_, bp, len, get_notifier(), logpath_);
}

int
BluetoothClient::readvall(const struct iovec* iov, int iovcnt)
{
    return IO::readvall(fd_, iov, iovcnt, get_notifier(), logpath_);
}

int
BluetoothClient::writevall(const struct iovec* iov, int iovcnt)
{
    return IO::writevall(fd_, iov, iovcnt, get_notifier(), logpath_);
}

int
BluetoothClient::timeout_read(char* bp, size_t len, int timeout_ms)
{
    return IO::timeout_read(fd_, bp, len, timeout_ms,
                            get_notifier(), logpath_);
}

int
BluetoothClient::timeout_readv(const struct iovec* iov,
                               int iovcnt,
                               int timeout_ms)
{
    return IO::timeout_readv(fd_, iov, iovcnt, timeout_ms, get_notifier(),
                             logpath_);
}

int
BluetoothClient::timeout_readall(char* bp, size_t len, int timeout_ms)
{
    return IO::timeout_readall(fd_, bp, len, timeout_ms, get_notifier(),
                               logpath_);
}

int
BluetoothClient::timeout_readvall(const struct iovec* iov, int iovcnt,
                                  int timeout_ms)
{
    return IO::timeout_readvall(fd_, iov, iovcnt, timeout_ms, get_notifier(),
                                logpath_);
}

int
BluetoothClient::get_nonblocking(bool *nonblockingp)
{
    return IO::get_nonblocking(fd_, nonblockingp);
}

int
BluetoothClient::set_nonblocking(bool nonblocking)
{
    return IO::set_nonblocking(fd_, nonblocking);
}

} // namespace oasys
#endif /* OASYS_BLUETOOTH_ENABLED */