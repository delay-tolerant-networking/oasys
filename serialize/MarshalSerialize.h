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
#ifndef _MARSHAL_SERIALIZE_H_
#define _MARSHAL_SERIALIZE_H_

#include "Serialize.h"
#include "../util/CRC32.h"

namespace oasys {

//////////////////////////////////////////////////////////////////////////////
/**
 * Common base class for Marshal and Unmarshal that manages the flat
 * buffer.
 */
class BufferedSerializeAction : public SerializeAction {
public:
    /** 
     * Since BufferedSerializeAction ignores the name field, calling
     * process() on a contained object is the same as just calling the
     * contained object's serialize() method.
     */
    virtual void process(const char* name, SerializableObject* object)
    {
        object->serialize(this);
    }
    
    /** Rewind to the beginning of the buffer */
    void rewind() { offset_ = 0; }

protected:
    /**
     * Constructor
     */
    BufferedSerializeAction(action_t action, context_t context,
                            u_char* buf, size_t length, 
                            int options);

    /**  
     * Get the next R/W length of the buffer.
     *
     * @return R/W buffer of size length or NULL on error
     */
    u_char* next_slice(size_t length);
    
    /** @return buffer */
    u_char* buf() { return error_ ? 0 : buf_; }

    /** @return buffer length */
    size_t length() { return length_; }
    
    /** @return Current offset into buf */
    size_t offset() { return offset_; }

 private:
    u_char* buf_;		///< Buffer that is un/marshalled
    size_t  length_;		///< Length of the buffer.
    size_t  offset_;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Marshal is a SerializeAction that flattens an object into a byte
 * stream.
 */
class Marshal : public BufferedSerializeAction {
public:
    /**
     * Constructor
     */
    Marshal(
	context_t context, 
	u_char*   buf, 
	size_t    length, 
        int       options = 0
    );

    /**
     * Since the Marshal operation doesn't actually modify the
     * SerializableObject, define a variant of action() and process()
     * that allows a const SerializableObject* as the object
     * parameter.
     */
    int action(const SerializableObject* const_object)
    {
        SerializableObject* object = (SerializableObject*)const_object;
        return SerializeAction::action(object);
    }

    void process(const char* name, SerializableObject* const_object)
    {
        SerializableObject* object = (SerializableObject*)const_object;
        return SerializeAction::process(name, object);
    }

    // Virtual functions inherited from SerializeAction
    void end_action();

    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, bool alloc_copy);
    void process(const char* name, std::string* s);

private:
    bool add_crc_;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Unmarshal is a SerializeAction that constructs an object's
 * internals from a flat byte stream.
 */
class Unmarshal : public BufferedSerializeAction {
public:
    /**
     * Constructor
     */
    Unmarshal(
	context_t     context, 
	const u_char* buf, 
	size_t        length,
        int           options = 0
    );

    Unmarshal(const u_char* buf, size_t length);

    // Virtual functions inherited from SerializeAction
    void begin_action();

    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, bool alloc_copy);
    void process(const char* name, std::string* s); 

private:
    bool has_crc_;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * MarshalSize is a SerializeAction that determines the buffer size
 * needed to run a Marshal action over the object.
 */
class MarshalSize : public SerializeAction {
public:
    /**
     * Constructor
     */
    MarshalSize(
	context_t context, 
        int       options = 0
    ) : SerializeAction(Serialize::INFO, context, options),
	size_((options & Serialize::USE_CRC)?sizeof(u_int32_t):0) {}

    /**
     * The virtual action function. Always succeeds.
     */
    int action(SerializableObject* object);
    
    /**
     * Again, we can tolerate a const object as well.
     */
    int action(const SerializableObject* const_object)
    {
        SerializableObject* object = (SerializableObject*)const_object;
        return action(object);
    }
    
    void process(const char* name, SerializableObject* const_object)
    {
        SerializableObject* object = (SerializableObject*)const_object;
        return SerializeAction::process(name, object);
    }

    /** @return Measured size */
    size_t size() { return size_; }
    
    // Virtual functions inherited from SerializeAction
    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, bool alloc_copy);
    void process(const char* name, std::string* s);

private:
    size_t size_;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * MarshalCRC: compute the CRC32 checksum of the bits.
 */
class MarshalCRC : public SerializeAction {
public:
    MarshalCRC(context_t context)
        : SerializeAction(Serialize::INFO, context) {}
    
    u_int32_t crc() { return crc_.value(); }
    
    // virtual from SerializeAction
    virtual int action(SerializableObject* object);

    /** @{ Make it so this can take const objects */
    int action(const SerializableObject* const_object)
    {
        SerializableObject* object = (SerializableObject*)const_object;
        return action(object);
    }
    void process(const char* name, SerializableObject* const_object)
    {
        SerializableObject* object = (SerializableObject*)const_object;
        return SerializeAction::process(name, object);
    }
    /** @} */

    // virtual from SerializeAction
    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, bool alloc_copy);
    void process(const char* name, std::string* s);

private:
    CRC32 crc_;
};

} // namespace oasys

#endif /* _MARSHAL_SERIALIZE_H_ */
