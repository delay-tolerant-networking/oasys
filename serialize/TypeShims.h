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

#ifndef __TYPE_SHIMS_H__
#define __TYPE_SHIMS_H__

#include <string>
#include "Serialize.h"

namespace oasys {

struct IntShim : public SerializableObject {
    IntShim(int i = 0) : i_(i) {}
    
    // virtual from SerializableObject
    void serialize(SerializeAction* a) {
	a->process("int", &i_);
    }

    int value() const { return i_; }
    
    int i_;
};

struct StringShim : public SerializableObject {
    StringShim(std::string* str) : str_(str) {}
    
    // virtual from SerializableObject
    void serialize(SerializeAction* a) {
	a->process("string", str_);
    }

    std::string* value() const { return str_; }

    std::string* str_;
};

struct NullStringShim : public SerializableObject {
    NullStringShim(const char* str = 0) 
	: str_(const_cast<char*>(str)) 
    {
	free_mem_ = (str == 0);
    }

    ~NullStringShim() { if(free_mem_) { free(str_); } }

    // virtual from SerializableObject
    void serialize(SerializeAction* a) {
        size_t len;
        a->process("string", 
		   reinterpret_cast<u_char**>(&str_), 
		   &len, 
		   Serialize::NULL_TERMINATED | Serialize::ALLOC_MEM);
    }

    const char* value() const { return str_; }

    char* str_;
    bool free_mem_;
};

}; // namespace oasys

#endif //__TYPE_SHIMS_H__
