// XXX/demmer add copyright
#ifndef _FILE_IOCLIENT_H_
#define _FILE_IOCLIENT_H_

#include "FdIOClient.h"
#include <fcntl.h>
#include <string.h>

/**
 * IOClient derivative for real files (i.e. not sockets). Unlike the
 * base class FdIOClient, FileIOClient contains the path as a member
 * variable and exposes file specific system calls, i.e. open(),
 * lseek(), etc.
 */
class FileIOClient : public FdIOClient {
public:
    /// Basic constructor, leaves both path and fd unset
    FileIOClient();
    virtual ~FileIOClient();

    ///@{
    /// System call wrappers
    int open(const char* path, int flags);
    int open(const char* path, int flags, mode_t mode);
    int close();
    int unlink();
    int lseek(off_t offset, int whence);
    ///@}

    /// Set the path associated with this file handle
    void set_path(const char* path) {
        path_.assign(path);
    }

    /// Reopen a previously opened path
    int reopen(int flags);

    /// Check if the file descriptor is open
    bool is_open() { return fd_ != -1; }

    /// Path accessor
    const char* path() { return path_.c_str(); }

protected:
    /// Path to the file
    std::string path_;
};

#endif /* _FILE_IOCLIENT_H_ */
