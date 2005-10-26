/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2005 Intel Corporation. All rights reserved. 
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

#include "StackTrace.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace oasys {

void
StackTrace::print_current_trace(bool in_sighandler)
{
    void *stack[MAX_STACK_DEPTH];
    memset(stack, 0, sizeof(stack));
    size_t count = get_trace(stack, MAX_STACK_DEPTH, in_sighandler ? 3 : 0);
    print_trace(stack + 2, count - 2); // skip this fn
}

void
StackTrace::print_trace(void *stack[], size_t count)
{
    char buf[1024];
    void* addr;
    
    strncpy(buf, "STACK TRACE: ", sizeof(buf));
    write(2, buf, strlen(buf));

    for (size_t i = 0; i < count; ++i) {
        addr = stack[i];
        Dl_info info;
        if (dladdr(addr, &info)) {
            int dll_offset = (int)((char*)addr - (char*)info.dli_fbase);
            sprintf(buf, "0x%08x:%s@0x%08x+0x%08x ",
                    (u_int)addr, info.dli_fname,
                    (u_int)info.dli_fbase, dll_offset);
        } else {
            sprintf(buf, "%p ", addr);
        }
        
        write(2, buf, strlen(buf));
    }
    write(2, "\n", 1);
}

#if defined(__i386__)
#include "StackTrace-x86.cc"
#else

#include <execinfo.h>

// Generic stack trace function uses the glibc builtin
size_t
StackTrace::get_trace(void* stack[], size_t size, u_int sighandler_frame)
{
    return backtrace(stack, size);
}

#endif

} // namespace oasys
