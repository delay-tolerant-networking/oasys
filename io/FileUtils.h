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


#ifndef _OASYS_FILE_UTILS_H_
#define _OASYS_FILE_UTILS_H_

#include <string>

namespace oasys {

/**
 * Abstraction class for some stateless file operations such as
 * probing to see if a file and/or directory exists, is readable, etc.
 *
 * For most operations, this is just a simpler interface to the stat()
 * system call.
 */
class FileUtils {
public:
    static bool readable(const char* path,
                         const char* log = 0);

    static size_t size(const char* path,
                       const char* log = 0);

    /// Make sure the given path is absolute, prepending the current
    /// directory if necessary.
    static void abspath(std::string* path);

    /// Deletes all of the files from a given directory
    /// @return 0 on success, errno otherwise
    static int rm_all_from_dir(const char* path);
};

}

#endif /* _OASYS_FILE_UTILS_H_ */
