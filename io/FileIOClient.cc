// XXX/demmer add copyright
#include "FileIOClient.h"
#include "IO.h"

FileIOClient::FileIOClient()
    : FdIOClient(-1)
{
}

FileIOClient::~FileIOClient()
{
    if (fd_ != -1)
        close();
}

int
FileIOClient::open(const char* path, int flags)
{
    path_.assign(path);
    fd_ = IO::open(path, flags, logpath_);
    return fd_;
}

int
FileIOClient::open(const char* path, int flags, mode_t mode)
{
    path_.assign(path);
    fd_ = IO::open(path, flags, mode, logpath_);
    return fd_;
}

int
FileIOClient::close()
{
    int ret = IO::close(fd_, logpath_, path_.c_str());
    fd_ = -1;
    return ret;
}

int
FileIOClient::reopen(int flags)
{
    ASSERT(path_.length() != 0);
    fd_ = IO::open(path_.c_str(), flags, logpath_);
    return fd_;
}

int
FileIOClient::unlink()
{
    if (path_.length() == 0)
        return 0;
    
    int ret = 0;
    ret = IO::unlink(path_.c_str(), logpath_);
    path_.assign("");
    return ret;
}

int
FileIOClient::lseek(off_t offset, int whence)
{
    return IO::lseek(fd_, offset, whence, logpath_);
}
