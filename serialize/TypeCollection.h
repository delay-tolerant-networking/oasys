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
#ifndef __OASYS_TYPE_COLLECTION_H__
#define __OASYS_TYPE_COLLECTION_H__

#include <map>

#include "../debug/Logger.h"
#include "Serialize.h"
#include "MarshalSerialize.h"

namespace oasys {

class TypeCollectionHelper;

namespace TypeCollectionErr {
enum {
    TYPECODE = 1,
    MEMORY,
};
};

/**
 * Generic base class needed to stuff the templated class into a map.
 */
class TypeCollectionHelper {
public:
    virtual void* new_object() = 0;
    virtual const char* name() const = 0;
    virtual ~TypeCollectionHelper() {}
};

/**
 * Conversion class from C++ types to their associated type codes. See
 * TYPE_COLLECTION_MAP macro below for a instantiation (specialization)
 * of this template to define the types.
 */
template<typename _Collection, typename _Type> class TypeCollectionCode;

/**
 * This templated type collection accomplishes severals things:
 *
 * - Enables different collections of type codes which can have the
 *   same numeric value, e.g. the same type codes can be reused in
 *   different projects that are then linked together.
 * - Type checked object creation from raw bits.
 * - Abstract type groups, e.g. Object* deserialized from concrete
 *   instantiations A, B, C who inherited from Object.
 * 
 * Example of use:
 * @code
 * // Type declaration for the Foobar collection of type codes
 * struct FoobarC {};
 * 
 * // Instantiate a typecollection for this collection (this goes in
 * // the .cc file)
 * TypeCollection<FoobarC>* TypeCollection<FoobarC>::instance_;
 * 
 * // An aggregate class Obj and concrete classes Foo, Bar
 * struct Obj : public SerializableObject {};
 * struct Foo : public Obj {
 *     Foo(TypeCollection<FoobarC>* b) {}
 *     void serialize(SerializeAction* a) {}
 * };
 * struct Bar : public Obj {
 *     Bar(TypeCollection<FoobarC>* b) {}
 *     void serialize(SerializeAction* a) {}
 * };
 * 
 * // in the .h file
 * TYPE_COLLECTION_DECLARE(FoobarC, Foo, 1);
 * TYPE_COLLECTION_DECLARE(FoobarC, Bar, 2);
 * TYPE_COLLECTION_GROUP(FoobarC, Obj, 1, 2);
 * 
 * // in the .cc file
 * TYPE_COLLECTION_DEFINE(FoobarC, Foo, 1);
 * TYPE_COLLECTION_DEFINE(FoobarC, Bar, 2);
 * 
 * // example of use
 * Foo* foo;
 * TypeCollection<FoobarC>* b = TypeCollection<FoobarC>::instance();
 * int err = b->new_object(TypeCollectionCode<TestC, Foo>::TYPECODE,
 *                         &foo, buf, len, Serialize::CONTEXT_LOCAL)
 * @endcode
 */
template<typename _Collection>
class TypeCollection : public Logger {
public:    
    typedef u_int32_t TypeCode_t;

    TypeCollection<_Collection>() : Logger("/type_collection") {}

    /** 
     * Note: this should be multithread safe because the instance is
     * created by the static initializers of the program, at which
     * time there should be only one thread.
     */ 
    static TypeCollection<_Collection>* instance() {
        if(!instance_) {
            instance_ = new TypeCollection<_Collection>();
        }        
        return instance_;
    }

    void reg(TypeCode_t typecode, TypeCollectionHelper* helper) {
        ASSERT(dispatch_.find(typecode) == dispatch_.end());
        dispatch_[typecode] = helper;
    }

    /**
     * Get a new object from the given typecode. NOTE: This can fail!
     * Be sure to check the return value from the function.
     *
     * @return 0 on no error, MEMORY if cannot allocate new object,
     * and TYPECODE if the typecode is invalid for the given type.
     */
    template<typename _Type>
    int new_object(TypeCode_t typecode, _Type** obj)
    {
        // Check that the given typecode is within the legal bounds
        // for the _Type of the return
        if(TypeCollectionCode<_Collection, _Type>::TYPECODE_LOW  > typecode ||
           TypeCollectionCode<_Collection, _Type>::TYPECODE_HIGH < typecode)
        {
            return TypeCollectionErr::TYPECODE;
        }

        // Based on the lookup in the dispatch, create a new object
        // and cast it to the given type.
        ASSERT(dispatch_.find(typecode) != dispatch_.end());
        *obj = static_cast<_Type*>(dispatch_[typecode]->new_object());
        if (*obj == NULL) {
            log_crit("out of memory");
            return TypeCollectionErr::MEMORY;
        }
        
        return 0;
    }

    /**
     * Return the stringified type code.
     */
    const char* type_name(TypeCode_t typecode) {
	if(dispatch_.find(typecode) == dispatch_.end()) {
	    return "";
	} 

	return dispatch_[typecode]->name();
    }

private:
    std::map<TypeCode_t, TypeCollectionHelper*> dispatch_;
    static TypeCollection<_Collection>*     instance_;
};

/**
 * Instantiate a template with the specific class and create a static
 * instance of this to register the class. Use the TYPE_COLLECTION_DEFINE macro
 * below.
 */
template<typename _Collection, typename _Class>
class TypeCollectionDispatch : public TypeCollectionHelper {
public:
    /** Register upon creation. */
    TypeCollectionDispatch<_Collection, _Class>
	(typename TypeCollection<_Collection>::TypeCode_t typecode,
	 const char* name) : name_(name)
    {
        TypeCollection<_Collection>::instance()->reg(typecode, this);
    }

    /** 
     * The _Class takes an instance of the TypeCollection class in order to
     * distinguish that the constructor is being called to build the
     * serializable object via a typecollection.
     *
     * @return The reason for the void* is to be able to virtualize
     * this class yet not have the problem of potentially slicing the
     * object via casting.
     */
    void* new_object() {
        return static_cast<void*>(new _Class(Builder()));
    }

    const char* name() const { return name_; }

private:
    const char* name_;
};

/**
 * Macro to use to define a class to be used by the typecollection.
 */
#define TYPE_COLLECTION_DEFINE(_collection, _class, _typecode)          \
    oasys::TypeCollectionDispatch<_collection, _class>                  \
        _class ## TypeCollection(_typecode, #_collection "::" #_class);

/**
 * Define the typecollection C++ type -> typecode converter
 */
#define TYPE_COLLECTION_DECLARE(_Collection, _Class, _code)     \
namespace oasys {                                               \
    template<>                                                  \
    struct TypeCollectionCode<_Collection, _Class> {            \
        enum {                                                  \
            TYPECODE_LOW  = _code,                              \
            TYPECODE_HIGH = _code                               \
        };                                                      \
        enum {                                                  \
            TYPECODE = _code                                    \
        };                                                      \
    };                                                          \
}

/**
 * Define an aggregate supertype, e.g. if a range of type codes {1, 2,
 * 3} are assigned to classes A, B, C and they have a common abstract
 * serializable supertype S, then to unserialize an S object (which
 * could potentially be of concrete types A, B, C), you need to use
 * this macro in the type codes header:
 * @code
 * TYPE_COLLECTION_GROUP(Collection, S, 1, 3);
 * @endcode
 */
#define TYPE_COLLECTION_GROUP(_Collection, _Class, _low, _high) \
namespace oasys {                                               \
    template<>                                                  \
    struct TypeCollectionCode<_Collection, _Class> {            \
        enum {                                                  \
            TYPECODE_LOW  = _low,                               \
            TYPECODE_HIGH = _high,                              \
        };                                                      \
    };                                                          \
}

/**
 * Macro to wrap the annoyingly finicky template static instantiation.
 */
#define TYPE_COLLECTION_INSTANTIATE(_Collection)        \
template<> class oasys::TypeCollection<_Collection>*    \
   oasys::TypeCollection<_Collection>::instance_ = 0

}; // namespace oasys

#endif //__OASYS_TYPE_COLLECTION_H__
