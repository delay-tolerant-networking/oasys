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


#ifndef _OASYS_FORMATTER_H_
#define _OASYS_FORMATTER_H_

#include <stdio.h>
#include <stdarg.h>

#include "DebugDumpBuf.h"
#include "DebugUtils.h"

namespace oasys {

/**
 * This class is intended to be used with a modified implementation of
 * snprintf/vsnprintf, defined in Formatter.cc.
 *
 * The modification implements a special control code combination of
 * '*%p' that triggers a call to Formatter::format(), called on the
 * object pointer passed in the vararg list.
 *
 * For example:
 *
 * @code
 * class FormatterTest : public Formatter {
 * public:
 *     virtual int format(char* buf, size_t sz) {
 *         return snprintf(buf, sz, "FormatterTest");
 *     }
 * };
 *
 * FormatterTest f;
 * char buf[256];
 * snprintf(buf, sizeof(buf), "pointer %p, format *%p\n");
 * // buf contains "pointer 0xabcd1234, format FormatterTest\n"
 * @endcode
 */
class Formatter {
public:
    /**
     * Virtual callback, called from this vsnprintf implementation
     * whenever it encounters a format string of the form "*%p".
     *
     * The output routine must not write more than sz bytes and is not
     * null terminated.
     *
     * @return The number of bytes written, or the number of bytes
     * that would have been written if the output is
     * truncated. 
     *
     * XXX/bowei -- this contract is fairly annoying to implement.
     */
    virtual int format(char* buf, size_t sz) const = 0;

    /**
     * Assertion check to make sure that obj is a valid formatter.
     * This basically just makes sure that in any multiple inheritance
     * situation where each base class has virtual functions,
     * Formatter _must_ be the first class in the inheritance chain.
     */
    static inline void assert_valid(const Formatter* obj);

    /**
     * Print out to a statically allocated buffer which can be called
     * from gdb. Note: must not be inlined in order for gdb to be able
     * to execute this function.
     */
    int debug_dump();

    virtual ~Formatter() {}

#ifndef NDEBUG
#define FORMAT_MAGIC 0xffeeeedd
    Formatter() : format_magic_(FORMAT_MAGIC) {}
    Formatter(const Formatter&) : format_magic_(FORMAT_MAGIC) {}
    unsigned int format_magic_;
#else
    Formatter() {}
    Formatter(const Formatter&) {}
#endif // NDEBUG

    
};

void __log_assert(bool x, const char* what, const char* file, int line);

inline void
Formatter::assert_valid(const Formatter* obj)
{
    (void)obj; // avoid unused variable warning
#ifndef NDEBUG
    __log_assert(obj->format_magic_ == FORMAT_MAGIC,
                 "Formatter object invalid -- maybe need a cast to Formatter*",
                 __FILE__, __LINE__);
#endif
}

} // namespace oasys

#endif /* _OASYS_FORMATTER_H_ */
