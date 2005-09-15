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
#ifndef _OASYS_STRING_SERIALIZE_H_
#define _OASYS_STRING_SERIALIZE_H_

#include "Serialize.h"
#include "../util/StringBuffer.h"

namespace oasys {

/**
 * StringSerialize is a SerializeAction that "flattens" the object
 * into a oasys StringBuffer;
 */
class StringSerialize : public SerializeAction {
public:
    /**
     * Constructor
     */
    StringSerialize(context_t context, int options);
 
    /**
     * We can tolerate a const object.
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

    /**
     * Accessor for the serialized string.
     */
    const StringBuffer& buf() { return buf_; }

    /// @{
    /// Virtual functions inherited from SerializeAction
    void end_action();
    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, int flags);
    void process(const char* name, std::string* s);
    /// @}

private:
    StringBuffer buf_; ///< string buffer
    char         sep_; ///< separator character (either " " or ".")
};

} // namespace oasys

#endif /* _OASYS_STRING_SERIALIZE_H_ */
