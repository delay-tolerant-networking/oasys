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
    TYPECODE = 1,
    CORRUPT,
    MEMORY,
};
};

/**
 * Conversion class from C++ types to their associated type codes. See
 * BUILDER_TYPECODE macro below for a instantiation (specialization)
 * of this template to define the types.
 */
template<typename _TypeCollection, typename _Type> class BuilderCode;

/**
 * This templated builder accomplishes severals things:
 *
 * - Enables different collections of type codes which can have the
 *   same numeric value, e.g. the same type codes can be reused in
 *   different projects that are then linked together.
 * - Type checked object creation from raw bits.
 * - Abstract aggregate types, e.g. Object* deserialized from concrete
 *   instantiations A, B, C who inherited from Object.
 * 
 * Example of use:
 * @code
 * // Type declaration for the Dtn collection of type codes
 * struct DtnC {};
 * 
 * // Instantiate a builder for this collection (this goes in the .cc file)
 * Builder<DtnC>* Builder<DtnC>::instance_;
 * 
 * // An aggregate class Obj and concrete classes Foo, Bar
 * struct Obj : public SerializableObject {};
 * struct Foo : public Obj {
 *     Foo(Builder<DtnC>* b) {}
 *     void serialize(SerializeAction* a) {}
 * };
 * struct Bar : public Obj {
 *     Bar(Builder<DtnC>* b) {}
 *     void serialize(SerializeAction* a) {}
 * };
 * 
 * // in the .h file
 * BUILDER_TYPECODE(DtnC, Foo, 1);
 * BUILDER_TYPECODE(DtnC, Bar, 2);
 * BUILDER_TYPECODE_AGGREGATE(DtnC, Obj, 1, 2);
 * 
 * // in the .cc file
 * BUILDER_CLASS(DtnC, Foo, 1);
 * BUILDER_CLASS(DtnC, Bar, 2);
 * 
 * // example of use
 * Foo* foo;
 * Builder<DtnC>* b = Builder<DtnC>::instance();
 * int err = b->new_object(BuilderCode<TestC, Foo>::TYPECODE, 
 *                         &foo, buf, len, Serialize::CONTEXT_LOCAL)
 * @endcode
 */
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
     * CORRUPT if unserialization fails and TYPECODE if the typecode
     * does not match the type.
     */
    template<typename _Type>
    int new_object(TypeCode_t typecode, _Type** obj_ptr, const u_char* data, 
                   int length, Serialize::context_t context)
    {
        // Check that the typecodes are within bounds
        if(BuilderCode<_TypeCollection, _Type>::TYPECODE_LOW  > typecode ||
            BuilderCode<_TypeCollection, _Type>::TYPECODE_HIGH < typecode)
        {
            return BuilderErr::TYPECODE;
        }
        
        ASSERT(dispatch_.find(typecode) != dispatch_.end());

        _Type* obj = static_cast<_Type*>(dispatch_[typecode]->new_object());
        if(obj == 0) {
            log_crit("out of memory");
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

    const char* type_name(TypeCode_t typecode) {
	if(dispatch_.find(typecode) == dispatch_.end()) {
	    return "";
	} 

	return dispatch_[typecode]->name();
    }

private:
    std::map<TypeCode_t, BuilderHelper*> dispatch_;
    static Builder<_TypeCollection>*     instance_;
};

/**
 * The generic base class is just to stuff the templated class into a
 * map.
 */
class BuilderHelper {
public:
    virtual void* new_object() = 0;
    virtual const char* name() const = 0;
};

/**
 * Instantiate a template with the specific class and create a static
 * instance of this to register the class. Use the BUILDER_CLASS macro
 * below.
 */
template<typename _TypeCollection, typename _Class>
class BuilderDispatch : public BuilderHelper {
public:
    /** Register upon creation. */
    BuilderDispatch<_TypeCollection, _Class>
	(typename Builder<_TypeCollection>::TypeCode_t typecode,
	 const char* name) : name_(name)
    {
        Builder<_TypeCollection>::instance()->reg(typecode, this);
    }

    /** 
     * The _Class takes an instance of the Builder class in order to
     * distinguish that the constructor is being called to build the
     * serializable object via a builder.
     *
     * @return The reason for the void* is to be able to virtualize
     * this class yet not have the problem of potentially slicing the
     * object via casting.
     */
    void* new_object() {
        return static_cast<void*>
            (new _Class(Builder<_TypeCollection>::instance()));
    }

    const char* name() const { return name_; }
    
private:
    const char* name_;
};

/**
 * Macro to use to define a class to be used by the builder.
 */
#define BUILDER_CLASS(_collection, _class, _typecode)             \
    BuilderDispatch<_collection, _class>                          \
        _class ## Builder(_typecode, #_collection "::" #_class);  \

/**
 * Define the builder C++ type -> typecode converter
 */
#define BUILDER_TYPECODE(_Collection, _Class, _code)    \
namespace oasys {                                       \
    template<>                                          \
    struct BuilderCode<_Collection, _Class> {           \
        enum {                                          \
            TYPECODE_LOW  = _code,                      \
            TYPECODE_HIGH = _code,                      \
        };                                              \
        enum {                                          \
            TYPECODE = _code,                           \
        };                                              \
    };                                                  \
}

/**
 * Define an aggregate supertype, e.g. if a range of type codes {1, 2,
 * 3} are assigned to classes A, B, C and they have a common abstract
 * serializable supertype S, then to unserialize an S object (which
 * could potentially be of concrete types A, B, C), you need to use
 * this macro in the type codes header:
 * @code
 * BUILDER_TYPECODE_AGGREGATE(Collection, S, 1, 3);
 * @endcode
 */
#define BUILDER_TYPECODE_AGGREGATE(_Collection, _Class, _low, _high)    \
namespace oasys {                                                       \
    template<>                                                          \
    struct BuilderCode<_Collection, _Class> {                           \
        enum {                                                          \
            TYPECODE_LOW  = _low,                                       \
            TYPECODE_HIGH = _high,                                      \
        };                                                              \
    };                                                                  \
}

}; // namespace oasys

#endif //__OBJECT_BUILDER_H__