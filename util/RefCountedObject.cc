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

#include "RefCountedObject.h"

namespace oasys {

//----------------------------------------------------------------------
RefCountedObject::RefCountedObject(const char* logpath)
    : refcount_(0),
      logger_("RefCountedObject", logpath)
{
}

//----------------------------------------------------------------------
RefCountedObject::~RefCountedObject()
{
}

//----------------------------------------------------------------------
void
RefCountedObject::add_ref(const char* what1, const char* what2) const
{
    atomic_incr(&refcount_);
    
    logger_.logf(LOG_DEBUG,
                 "refcount *%p %u -> %u add %s %s",
                 this, refcount_.value - 1, refcount_.value, what1, what2);
    
    ASSERT(refcount_.value > 0);
}

//----------------------------------------------------------------------
void
RefCountedObject::del_ref(const char* what1, const char* what2) const
{
    ASSERT(refcount_.value > 0);

    logger_.logf(LOG_DEBUG,
                 "refcount *%p %d -> %d del %s %s",
                 this, refcount_.value, refcount_.value - 1, what1, what2);
    
    // atomic_decr_test will only return true if the currently
    // executing thread is the one that sent the refcount to zero.
    // hence we are safe in knowing that there are no other references
    // on the object, and that only one thread will call
    // no_more_refs()
    
    if (atomic_decr_test(&refcount_)) {
        ASSERT(refcount_.value == 0);
        no_more_refs();
    }
}

//----------------------------------------------------------------------
void
RefCountedObject::no_more_refs() const
{
    logger_.logf(LOG_DEBUG, "no_more_refs *%p... deleting object", this);
    delete this;
}

//----------------------------------------------------------------------
int
RefCountedObject::format(char* buf, size_t sz) const
{
    return snprintf(buf, sz, "RefCountedObject %p", this);
}

} // namespace oasys
