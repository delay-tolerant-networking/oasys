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

#ifndef _OASYS_TEMPREF_H_
#define _OASYS_TEMPREF_H_

#include "../debug/DebugUtils.h"

namespace oasys {

/**
 * For functions that want to return an ObjectRef, it's irritating to
 * have to go through a series of add_ref and del_ref calls to deal
 * with the C++ temporary objects that are created.
 *
 * Therefore, this class holds a pointer to a reference counted object
 * but doesn't up the reference count, and enforces that the pointer
 * is released before the class is destroyed since the temporary
 * destructor will PANIC if the pointer is non-NULL;
 *
 */
template <typename _Type>
class TempRef {
public:
    /**
     * Constructor that initializes the pointer to be empty.
     */
    TempRef(const char* what1, const char* what2 = "")
        : object_(NULL), what1_(what1), what2_(what2)
    {
    }
    
    /**
     * Constructor that takes an initial pointer for assignment.
     */
    TempRef(_Type* object, const char* what1, const char* what2 = "")
        : object_(object), what1_(what1), what2_(what2)
    {
    }

    /**
     * Copy constructor.
     */
    TempRef(const TempRef& other)
    {
        object_ = other.object();
        other.release();
    }
    
    /**
     * Destructor that asserts the pointer was claimed.
     */
    ~TempRef() {
        if (object_ != NULL) {
            PANIC("TempRef %s %s destructor fired but object still exists!!",
                  what1_, what2_);
        }
    }

    /**
     * Assignment operator.
     */
    TempRef& operator=(const TempRef<_Type>& other)
    {
        object_ = other.object();
        other.release();
        return *this;
    }

    /**
     * Assignment operator.
     */
    TempRef& operator=(_Type* object)
    {
        ASSERTF(object_ == NULL,
                "TempRef can only assign to null reference");
        object_ = object;
        return *this;
    }

    /**
     * Accessor for the object.
     */
    _Type* object() const {
        return object_;
    }
    
    /**
     * Operator overload for pointer access.
     */
    _Type* operator->() const
    {
        return object_;
    }

    /**
     * Operator overload for pointer access.
     */
    _Type& operator*() const
    {
        return *object_;
    }

    /**
     * Release the reference to the object. Note this is declared
     * const even though it technically modifies the object.
     */
    void release() const {
        object_ = NULL;
    }

    /**
     * Equality operator.
     */
    bool operator==(_Type* o)
    {
        return (object_ == o);
    }
    
    /**
     * Inequality operator.
     */
    bool operator!=(_Type* o)
    {
        return (object_ != o);
    }

    const char* what1() const { return what1_; } 
    const char* what2() const { return what2_; }
    
private:
    /**
     * The object pointer
     */
    mutable _Type* object_;

    /**
     * Debugging strings.
     */
    const char *what1_, *what2_;
};

} // namespace oasys

#endif /* _OASYS_TEMPREF_H_ */
