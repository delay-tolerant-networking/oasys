/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2004 Intel Corporation. All rights reserved. 
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
#ifndef __OBJECT_BUILDER_H__
#define __OBJECT_BUILDER_H__

#include <map>

#include "../debug/Logger.h"
#include "Serialize.h"
#include "MarshalSerialize.h"

namespace oasys {

class BuilderHelper;

namespace BuilderErr {
enum {
    TYPE_CODE = 1,
    CORRUPT,
    MEMORY,
};
};

template<typename _TypeCollection>
class Builder : public Logger {
public:    
    typedef u_int32_t TypeCode_t;

    Builder<_TypeCollection>() : Logger("/builder") {}

    /** 
     * Note: this should be multithread safe because the instance is
     * created by the static initializers of the program, at which
     * time there should be only one thread.
     */ 
    static Builder<_TypeCollection>* instance() {
        if(!instance_) {
            instance_ = new Builder<_TypeCollection>();
        }        
        return instance_;
    }

    void reg(TypeCode_t typecode, BuilderHelper* helper) {
        ASSERT(dispatch_.find(typecode) == dispatch_.end());
        dispatch_[typecode] = helper;
    }

    /**
     * Get a new object from the bits. NOTE: This can fail! Be sure to
     * check the return value from the function.
     *
     * @return 0 on no error, MEMORY if cannot allocate new object,
     * CORRUPT if unserialization fails and TYPE_CODE if the TYPE_CODE
     * does not match the type.
     */
    template<typename _Type>
    int new_object(TypeCode_t typecode, _Type** obj_ptr, const u_char* data, 
                   int length, Serialize::context_t context)
    {
        _Type* obj;

        // Check that the typecodes are within bounds
        // XXX/bowei this can be fixed.
   //      if(BuilderDispatch<_Type, _TypeCollection>::TYPECODE_LOW  > typecode ||
//            BuilderDispatch<_Type, _TypeCollection>::TYPECODE_HIGH < typecode)
//         {
//             return BuilderErr::TYPE_CODE;
//         }
        
        ASSERT(dispatch_.find(typecode) != dispatch_.end());

        obj = static_cast<_Type*>(dispatch_[typecode]->new_object());
        if(obj == 0) {
            log_crit("/builder", "out of memory");
            return BuilderErr::MEMORY;
        }

        oasys::Unmarshal unm(context, data, length);
        if(unm.action(obj) != 0) {
            delete obj;
            return BuilderErr::CORRUPT;
        }
        
        *obj_ptr = obj;
        return 0;
    }

private:
    std::map<TypeCode_t, BuilderHelper*> dispatch_;
    static Builder<_TypeCollection>* instance_;
};

/**
 * The generic base class is just to stuff the templated class into a
 * map.
 */
class BuilderHelper {
public:
    virtual void* new_object() = 0;
};

/**
 * Instantiate a template with the specific  class and create a
 * static instance of this to register the class. Use the
 * DECL_BUILDER macros below.
 */
template<typename _Class, typename _TypeCollection>
class BuilderDispatch : public BuilderHelper {
public:
    /** Register upon creation. */
    BuilderDispatch<_Class, _TypeCollection>
    (typename Builder<_TypeCollection>::TypeCode_t typecode) 
    {
        Builder<_TypeCollection>::instance()->reg(typecode, this);
    }

    /** 
     * The _Class takes an instance of the Builder class in order to
     * distinguish that the constructor is being called to build the
     * serializable object via a builder.
     *
     * @return The reason for the void* is to be able to virtualize
     * this class.
     */
    void* new_object() {
        return static_cast<void*>
            (new _Class(Builder<_TypeCollection>::instance()));
    }
};


/**
 * Utility macro for encapsulation.
 */
#define DECL_BUILDER(_class, _collection, _typecode)            \
    BuilderDispatch<_class, _collection> _class ## Builder(_typecode)
}; // namespace oasys

#endif //__OBJECT_BUILDER_H__
