/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2006 Intel Corporation. All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 *  Neither the name of the Intel Corporation nor the names of its
 *  contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
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

#include "Singleton.h"
#include "debug/Log.h"
#include <string.h>

namespace oasys {

#define MAX_SINGLETONS 64

SingletonBase** SingletonBase::all_singletons_ = 0;
int             SingletonBase::num_singletons_ = 0;
SingletonBase::Fini SingletonBase::fini_;

//----------------------------------------------------------------------
SingletonBase::SingletonBase()
{
    if (all_singletons_ == 0) {
        all_singletons_ = (SingletonBase**)malloc(MAX_SINGLETONS * sizeof(SingletonBase*));
        memset(all_singletons_, 0, (MAX_SINGLETONS * sizeof(SingletonBase*)));
    }

    all_singletons_[num_singletons_++] = this;
}


//----------------------------------------------------------------------
SingletonBase::~SingletonBase()
{
}

//----------------------------------------------------------------------
SingletonBase::Fini::~Fini()
{
    for (int i = SingletonBase::num_singletons_ - 1; i >= 0; --i)
    {
        log_debug("/debug",
                  "deleting singleton %d (%p)",
                  i, SingletonBase::all_singletons_[i]);
        
        delete SingletonBase::all_singletons_[i];
    }

    oasys::Log::shutdown();
}

} // namespace oasys
