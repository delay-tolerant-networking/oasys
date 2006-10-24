/*
 *    Copyright 2004-2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */


#ifndef _OASYS_FILE_IOCLIENT_H_
#define _OASYS_FILE_IOCLIENT_H_

#include "FdIOClient.h"
#include <fcntl.h>
#include <string.h>

namespace oasys {

/**
 * IOClient derivative for real files -- not sockets. Unlike the base
 * class FdIOClient, FileIOClient contains the path as a member
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
    int open(const char* path, int flags, int* errnop = 0);
    int open(const char* path, int flags, mode_t mode, int* errnop = 0);
    int close();
    int unlink();
    int lseek(off_t offset, int whence);
    int truncate(off_t length);
    int mkstemp(char* temp);
    int stat(struct stat* buf);
    int lstat(struct stat* buf);
    ///@}

    /// Copy len bytes of file contents from the current offset to
    /// another open file. Return the amount copied or -1 if error
    int copy_contents(size_t len, FileIOClient* dest);

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

    /// Path accessor
    size_t path_len() { return path_.length(); }

protected:
    /// Path to the file
    std::string path_;
};

} // namespace oasys

#endif /* _OASYS_FILE_IOCLIENT_H_ */
