/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2004 Intel Corporation. All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 *   Neither the name of the Intel Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <sys/stat.h>

#include "debug/Log.h"
#include "FileUtils.h"

namespace oasys {

bool
FileUtils::readable(const char* path, const char* log)
{
    struct stat st;
    int ret = stat(path, &st);

    if (ret == -1) {
        logf(log, LOG_DEBUG,
             "FileUtils::readable(%s): error running stat %s",
             path, strerror(errno));
        return false;
    }

    if (!S_ISREG(st.st_mode)) {
        logf(log, LOG_DEBUG,
             "FileUtils::readable(%s): not a regular file", path);
        return false;
    }

    if (st.st_mode & S_IRUSR == 0) {
        logf(log, LOG_DEBUG,
             "FileUtils::readable(%s): no readable permissions", path);
        return false;
    }
    
    return true;
}

size_t
FileUtils::size(const char* path, const char* log)
{
    struct stat st;
    int ret = stat(path, &st);

    if (ret == -1) {
        if (log) {
            logf(log, LOG_DEBUG,
                 "FileUtils::size(%s): error running stat %s",
                 path, strerror(errno));
        }
        return 0;
    }

    if (!S_ISREG(st.st_mode)) {
        if (log) {
            logf(log, LOG_DEBUG,
                 "FileUtils::size(%s): not a regular file", path);
        }
        return 0;
    }

    return st.st_size;
}

void
FileUtils::abspath(std::string* path)
{
    if ((*path)[0] != '/') {
        char cwd[PATH_MAX];
        ::getcwd(cwd, PATH_MAX);

        std::string temp = *path;
        *path = cwd;
        *path += '/' + temp;
    }
}


} // end namespace
