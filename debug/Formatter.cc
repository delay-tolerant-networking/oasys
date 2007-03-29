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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdarg.h>
#include <memory.h>
#include "Formatter.h"
#include "../compat/fpclassify.h"

namespace oasys {

int
Formatter::debug_dump()
{
    memset(DebugDumpBuf::buf_, 0, DebugDumpBuf::size_);
    
    return format(DebugDumpBuf::buf_, DebugDumpBuf::size_);
}

} // namespace oasys

extern "C"
size_t formatter_format(void* p, char* str, size_t strsz)
{
    if (! p) {
        strncpy(str, "(null)", strsz);
        return 6;
    }

    // at the entry to vsnprintf, the code decremented strsz by one to
    // leave space for the trailing null, but in case there are nested
    // calls to format, we need the real amount of space here, so bump
    // it up by one again since we're careful not to clobber
    const ::oasys::Formatter* fmtobj = (const ::oasys::Formatter*)p;
    ::oasys::Formatter::assert_valid(fmtobj);
    return fmtobj->format(str, strsz + 1);
}
