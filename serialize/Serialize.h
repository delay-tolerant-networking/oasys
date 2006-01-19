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
#ifndef _OASYS_SERIALIZE_H_
#define _OASYS_SERIALIZE_H_

/**
 * @file
 * This file defines the core set of objects that define the
 * Serialization layer.
 */

#include <string>
#include <vector>
#include <sys/types.h>
#include "../compat/inttypes.h"

namespace oasys {

class Serialize;
class SerializeAction;
class SerializableObject;

/**
 * Empty base class that's just used for name scoping of the action
 * and context enumerated types.
 */
class Serialize {
public:
    /**
     * Action type codes, one for each basic type of SerializeAction.
     */
    typedef enum {
        MARSHAL = 1,	///< in-memory  -> serialized representation
        UNMARSHAL,	///< serialized -> in-memory  representation
        INFO		///< informative scan (e.g. size, table schema)
    } action_t;
    
    /**
     * Context type codes, one for each general context in which
     * serialization occurs. 
     */
    typedef enum {
        CONTEXT_UNKNOWN = 1,	///< no specified context (default)
        CONTEXT_NETWORK,	///< serialization to/from the network
        CONTEXT_LOCAL		///< serialization to/from local disk
    } context_t;

    /** Options for un/marshaling. */
    enum {
        USE_CRC = 1 << 0,
    };

    /** Options for StringSerialize */
    enum {
        INCLUDE_NAME    = 1 << 0, ///< Serialize as "name1 value1 name2 value2 "
        DOT_SEPARATED   = 1 << 1, ///< Use . not " " as field separations
    };

    /** Options for un/marshaling process() methods */
    enum {
        ALLOC_MEM       = 1<<0, ///< Allocated memory to be freed by the user 
        NULL_TERMINATED = 1<<1, ///< Delim by '\0' instead of storing length
    };
};

/**
 * Empty class used by various object factories (e.g. TypeCollection
 * and ObjectBuilder) to pass to the new object's constructor to
 * distinguish the call from a default construction.
 *
 * For example:
 *
 * @code
 * new SerializableObject(Builder());
 * @endcode
 */
class Builder {
public:
    Builder() {}
    Builder(const Builder& b) {}
};

/**
 * Inherit from this class to add serialization capability to a class.
 */
class SerializableObject {
public:
    virtual ~SerializableObject() {}

    /**
     * This should call v->process() on each of the types that are to
     * be serialized in the object.
     */
    virtual void serialize(SerializeAction* a) = 0;
};

/**
 * A vector of SerializableObjects.
 */
typedef std::vector<SerializableObject*> SerializableObjectVector;

/**
 * The SerializeAction is responsible for implementing callback
 * functions for all the basic types. The action object is then passed
 * to the serialize() function which will re-dispatch to the basic
 * type functions for each of the SerializableObject's member fields.
 *
 * INVARIANT: A single SerializeAction must be able to be called on
 * several different visitee objects in succession. (Basically this
 * ability is used to be able to string several Marshallable objects
 * together, either for writing or reading).
 */
class SerializeAction : public Serialize {
public:
    
    /**
     * Create a SerializeAction with the specified type code and context
     *
     * @param action serialization action type code
     * @param context serialization context
     * @param options serialization options
     */
    SerializeAction(action_t action, context_t context, int options = 0);

    /**
     * Perform the serialization or deserialization action on the object.
     *
     * @return 0 if success, -1 on error
     */
    virtual int action(SerializableObject* object);

    /**
     * Control the initialization done before executing an action.
     */
    virtual void begin_action();

    /**
     * Control the cleanup after executing an action.
     */
    virtual void end_action();

    /**
     * Accessor for the action type.
     */
    action_t action_code() { return action_; }
    
    /**
     * Accessor for the context.
     */
    context_t context() { return context_; }

    /**
     * Accessor for error
     */ 
    bool error() { return error_; }

    /***********************************************************************
     *
     * Processor functions, one for each type.
     *
     ***********************************************************************/

    /**
     * Process function for a contained SerializableObject.
     *
     * The default implementation just calls serialize() on the
     * contained object, ignoring the name value. However, a derived
     * class can of course override it to make use of the name (as is
     * done by SQLTableFormat, for example).
     */
    virtual void process(const char* name, SerializableObject* object)
    {
        object->serialize(this);
    }
    
    /**
     * Process function for a 4 byte integer.
     */
    virtual void process(const char* name, u_int32_t* i) = 0;

    /**
     * Process function for a 2 byte integer.
     */
    virtual void process(const char* name, u_int16_t* i) = 0;

    /**
     * Process function for a byte.
     */
    virtual void process(const char* name, u_int8_t* i) = 0;

    /**
     * Process function for a boolean.
     */
    virtual void process(const char* name, bool* b) = 0;

    /**
     * Process function for a constant length char buffer.
     * 
     * @param name   field name
     * @param bp     buffer
     * @param len    buffer length
     */
    virtual void process(const char* name, u_char* bp, size_t len) = 0;

    /**
     * Process function for a variable length char buffer.
     *
     * @param name   field name
     * @param bp     buffer, allocated by SerializeAction if ALLOC_MEM 
     *               flag is set.
     * @param lenp   IN: If ALLOC_MEM flags is set, then len is the 
     *               length of the buffer allocated.
     *               OUT: contains the length of the buffer
     * @param flags  ALLOC_MEM as above, NULL_TERMINATED specifies that
     *               the data stored will be a null-terminated C-string. 
     */
    virtual void process(const char* name, u_char** bp,
                         size_t* lenp, int flags) = 0;

    /**
     * Process function for a c++ string.
     */
    virtual void process(const char* name, std::string* s) = 0;

    ///@{
    /**
     * Adaptor functions for signed/unsigned compatibility
     */
    virtual void process(const char* name, int32_t* i)
    {
        process(name, (u_int32_t*)i);
    }
    
    virtual void process(const char* name, int16_t* i)
    {
        process(name, (u_int16_t*)i);
    }

    virtual void process(const char* name, int8_t* i)
    {
        process(name, (u_int8_t*)i);
    }

    virtual void process(const char* name, char* bp, size_t len)
    {
        process(name, (u_char*)bp, len);
    }

    /// @}
    
    /** Set a log target for verbose serialization */
    void logpath(const char* log) { log_ = log; }
    
    /**
     * Destructor.
     */
    virtual ~SerializeAction();

protected:
    action_t  action_;	///< Serialization action code
    context_t context_;	///< Serialization context

    int       options_; ///< Serialization options
    const char* log_;	///< Optional log for verbose marshalling
    
    /**
     * Signal that an error has occurred.
     */
    void signal_error() { error_ = true; }
    
private:
    bool      error_;	///< Indication of whether an error occurred

    SerializeAction();	/// Never called
};

} // namespace oasys

#endif /* _OASYS_SERIALIZE_H_ */
