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

class IntShim : public SerializableObject {
public:
    IntShim(int value = 0, const char* name = "int")
        : name_(name), value_(value) {}
    IntShim(const Builder& b) {}
    
    // virtual from SerializableObject
    void serialize(SerializeAction* a) {
	a->process(name_.c_str(), &value_);
    }

    int value() const { return value_; }
    void assign(int value) { value_ = value; }

private:
    std::string name_;
    int         value_;
};

class StringShim : public SerializableObject {
public:
    StringShim(const std::string& str, const char* name = "string")
        : name_(name), str_(str) {}
    StringShim(const Builder& b) {}
    
    virtual ~StringShim() {}
    
    // virtual from SerializableObject
    void serialize(SerializeAction* a) {
	a->process(name_.c_str(), &str_);
    }

    const std::string& value() const { return str_; }
    void assign(const std::string& str) { str_.assign(str); }

private:
    std::string name_;
    std::string str_;
};

class NullStringShim : public SerializableObject {
public:
    NullStringShim(const char* str, const char* name = "string") 
	: name_(name), str_(const_cast<char*>(str)) 
    {
	free_mem_ = (str == 0);
    }

    NullStringShim(Builder b)
        : name_("string"), str_(NULL), free_mem_(false)
    {}

    ~NullStringShim() { if(free_mem_) { free(str_); } }

    // virtual from SerializableObject
    void serialize(SerializeAction* a)
    {
        size_t len = 0;
        a->process(name_.c_str(), 
		   reinterpret_cast<u_char**>(&str_), &len,
		   Serialize::NULL_TERMINATED | Serialize::ALLOC_MEM);
        free_mem_ = true;
    }

    const char* value() const { return str_; }

private:
    std::string name_;
    char* str_;
    bool free_mem_;
};

}; // namespace oasys

#endif //__TYPE_SHIMS_H__
