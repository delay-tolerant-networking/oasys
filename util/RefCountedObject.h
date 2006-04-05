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
#ifndef _REFCOUNTEDOBJECT_H_
#define _REFCOUNTEDOBJECT_H_

#include "../debug/Formatter.h"
#include "../debug/Log.h"
#include "../thread/Atomic.h"

namespace oasys {

/**
 * Simple implementation of the add_ref / del_ref contract used by the
 * templated Ref class. This class maintains an integer count of the
 * live references on it, and calls the virtual function
 * no_more_refs() when the refcount goes to zero. The default
 * implementation of no_more_refs() simply calls 'delete this'.
 *
 * Also, this implementation declares add_ref and del_ref as const
 * methods, and declares refcount_ to be mutable, the reason for which
 * being that taking or removing a reference on an object doesn't
 * actually modify the object itself.
 *
 * Finally, the class also stores a Logger instance that's used for
 * debug logging of add_ref/del_ref calls. Note that we use a
 * contained logger (rather than inheritance) to avoid conflicts with
 * descendent classes that may themselves inherit from Logger.
 *
 * The RefCountedObject inherits from Formatter and defines a simple
 * implementation of Format that just includes the pointer value, but
 * other derived classes can (and should) override format to print
 * something more useful.
 * 
 */
class RefCountedObject : public Formatter {
public:
    /**
     * Constructor that takes the debug logging path to be used for
     * add and delete reference logging.
     */
    RefCountedObject(const char* logpath);

    /**
     * Virtual destructor declaration.
     */
    virtual ~RefCountedObject();

    /**
     * Bump up the reference count.
     *
     * @param what1 debugging string identifying who is incrementing
     *              the refcount
     * @param what2 optional additional debugging info
     */
    void add_ref(const char* what1, const char* what2 = "") const;

    /**
     * Bump down the reference count.
     *
     * @param what1 debugging string identifying who is decrementing
     *              the refcount
     * @param what2 optional additional debugging info
     */
    void del_ref(const char* what1, const char* what2 = "") const;

    /**
     * Hook called when the refcount goes to zero.
     */
    virtual void no_more_refs() const;

    /**
     * Virtual from Formatter
     */
    int format(char* buf, size_t sz) const;

    /**
     * Accessor for the refcount value.
     */
    u_int32_t refcount() const { return refcount_.value; }
    
protected:
    /// The reference count
    mutable atomic_t refcount_;
    
    /// Logger object used for debug logging
    Logger logger_;
};

} // namespace oasys

#endif /* _REFCOUNTEDOBJECT_H_ */
