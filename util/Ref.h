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
#ifndef _OASYS_REF_H_
#define _OASYS_REF_H_

#include "../debug/DebugUtils.h"
#include "TempRef.h"

namespace oasys {

/**
 * Smart pointer class to maintain reference counts on objects.
 *
 * The reference template expects the instatiating _Type to implement
 * methods for adding and deleting references that fit the following
 * signature:
 *
 * <code>
 * void add_ref(const char* what1, const char* what2);
 * void del_ref(const char* what1, const char* what2);
 * </code>
 *
 * The strings what1 and what2 can be used for debugging and are
 * stored in the reference. Note that what1 is mandatory while what2
 * is optional.
 */
template <typename _Type>
class Ref {
public:
    /**
     * Constructor that initializes the pointer to be empty.
     */
    Ref(const char* what1, const char* what2 = "")
        : object_(NULL), what1_(what1), what2_(what2)
    {
    }
    
    /**
     * Constructor that takes an initial pointer for assignment.
     */
    Ref(_Type* object, const char* what1, const char* what2 = "")
        : object_(object), what1_(what1), what2_(what2)
    {
        if (object_)
            object->add_ref(what1_, what2_);
    }

    /**
     * Constructor that takes a temporary ref.
     */
    Ref(const TempRef<_Type> temp)
        : what1_(temp.what1()),
          what2_(temp.what2())
    {
        object_ = temp.object();
        if (object_) {
            object_->add_ref(what1_, what2_);
            temp.release();
        }
    }

    /**
     * Copy constructor.
     */
    Ref(const Ref& other)
        : what1_(other.what1_),
          what2_(other.what2_)
    {
        object_ = other.object();
        if (object_) 
            object_->add_ref(what1_, what2_);
    }

    /**
     * Destructor.
     */
    ~Ref()
    {
        release();
    }

    /**
     * Release the reference on the object.
     */
    void release()
    {
        if (object_) {
            object_->del_ref(what1_, what2_);
            object_ = NULL;
        }
    }

    /**
     * Accessor for the object pointer.
     */
    _Type* object() const
    {
        return object_;
    }

    /**
     * Operator overload for pointer access.
     */
    _Type* operator->() const
    {
        ASSERT(object_ != 0);
        return object_;
    }

    /**
     * Operator overload for pointer access.
     */
    _Type& operator*() const
    {
        ASSERT(object_ != 0);
        return *object_;
    }

    /**
     * Assignment function.
     */
    void assign(_Type* new_obj)
    {
        if (object_ != new_obj)
        {
            if (new_obj != 0) {
                new_obj->add_ref(what1_, what2_);
            }
            
            if (object_ != 0) {
                object_->del_ref(what1_, what2_);
            }

            object_ = new_obj;
        }
    }

    /**
     * Assignment operator.
     */
    Ref& operator=(_Type* o)
    {
        assign(o);
        return *this;
    }

    /**
     * Assignment operator.
     */
    Ref& operator=(const Ref<_Type>& other)
    {
        assign(other.object_);
        return *this;
    }

    /**
     * Assignment operator from a temporary ref.
     */
    Ref& operator=(const TempRef<_Type>& temp)
    {
        if (object_ != temp.object()) {
            assign(temp.object());
        }
        temp.release();
        return *this;
    }
    
    /**
     * Equality operator.
     */
    bool operator==(_Type* o) const
    {
        return (object_ == o);
    }
     
    /**
     * Equality operator.
     */
    bool operator==(const Ref<_Type>& other) const
    {
        return (object_ == other.object_);
    }
    
   /**
     * Inequality operator.
     */
    bool operator!=(_Type* o) const
    {
        return (object_ != o);
    }
    
    /**
     * Equality operator.
     */
    bool operator!=(const Ref<_Type>& other) const
    {
        return (object_ != other.object_);
    }
    
private:
    /**
     * The object.
     */
    _Type* object_;

    /**
     * Debugging strings.
     */
    const char *what1_, *what2_;
};

} // namespace oasys

#endif /* _OASYS_REF_H_ */
