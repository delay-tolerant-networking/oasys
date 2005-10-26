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

#if defined(__i386__)

#include "FatalSignals.h"

#if defined(__linux__)
#include <asm/sigcontext.h>
struct sigframe
{
	char *pretcode;
	int sig;
	struct sigcontext sc;
	struct _fpstate fpstate;
};
#endif

// Generally, this function just follows the frame pointer and looks
// at the return address (i.e. the next one on the stack).
//
// Things get a little funky in the frame for the signal handler
// (identified by the parameter sighandler_frame), where we need to
// look into the place where the kernel stored the faulting address.
size_t
StackTrace::get_trace(void* stack[], size_t size, u_int sighandler_frame)
{
    void **fp;

    asm volatile("movl %%ebp,%0" : "=r" (fp));

    stack[0] = 0; // fake frame for this this fn, just use 0
    size_t frame = 1;
    while (frame < size) {
	if (*(fp + 1) == 0 || *fp == 0)
	    break;
        
	if (sighandler_frame != 0 && frame == sighandler_frame) {
#if defined(__linux__) 
	    struct sigframe* sf = (struct sigframe*)(fp+1);
	    struct sigcontext* scxt = &(sf->sc);
	    stack[frame] = (void*) scxt->eip;
#else
	    stack[frame] = *(fp + 1);
#endif 
	} else {
	    stack[frame] = *(fp + 1);
	}
        
	fp = (void **)(*fp);
        ++frame;
    }

    return frame;
}

#endif /* __i386__ */
