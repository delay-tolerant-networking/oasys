/*
 *    Copyright 2004-2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */


#ifndef __TYPE_SHIMS_H__
#define __TYPE_SHIMS_H__

#include <string>
#include "../debug/Formatter.h"
#include "Serialize.h"

namespace oasys {

//----------------------------------------------------------------------------
class IntShim : public Formatter, public SerializableObject {
public:
    IntShim(int32_t value = 0, const char* name = "int")
        : name_(name), value_(value) {}
    IntShim(const Builder&) {}

    // virtual from Formatter
    int format(char* buf, size_t sz) const {
        return snprintf(buf, sz, "%d", value_);
    }
    
    // virtual from SerializableObject
    void serialize(SerializeAction* a) {
	a->process(name_.c_str(), &value_);
    }

    int32_t value() const { return value_; }
    void assign(int32_t value) { value_ = value; }

    
private:
    std::string name_;
    int32_t     value_;
};

//----------------------------------------------------------------------------
class UIntShim : public Formatter, public SerializableObject {
public:
    UIntShim(u_int32_t value = 0, const char* name = "u_int")
        : name_(name), value_(value) {}
    UIntShim(const Builder&) {}
    
    // virtual from Formatter
    int format(char* buf, size_t sz) const {
        return snprintf(buf, sz, "%u", value_);
    }
    
    // virtual from SerializableObject
    void serialize(SerializeAction* a) {
	a->process(name_.c_str(), &value_);
    }

    u_int32_t value() const { return value_; }
    void assign(u_int32_t value) { value_ = value; }

    bool operator==(const UIntShim& other) const {
        return value_ == other.value_;
    }
    
private:
    std::string name_;
    u_int32_t   value_;
};

//----------------------------------------------------------------------------
class StringShim : public Formatter, public SerializableObject {
public:
    StringShim(const std::string& str = "", const char* name = "string")
        : name_(name), str_(str) {}
    StringShim(const Builder&) {}
    
    virtual ~StringShim() {}
    
    // virtual from Formatter
    int format(char* buf, size_t sz) const {
        return snprintf(buf, sz, "%s", str_.c_str());
    }
    
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

//----------------------------------------------------------------------------
class NullStringShim : public Formatter, public SerializableObject {
public:
    NullStringShim(const char* str, const char* name = "string") 
	: name_(name), str_(const_cast<char*>(str)) 
    {
	free_mem_ = (str == 0);
    }

    NullStringShim(const Builder&)
        : name_("string"), str_(NULL), free_mem_(false)
    {}

    ~NullStringShim() { if(free_mem_) { free(str_); } }

    // virtual from Formatter
    int format(char* buf, size_t sz) const {
        return snprintf(buf, sz, "%s", str_);
    }
    
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

//----------------------------------------------------------------------------
class ByteBufShim : public Formatter, public SerializableObject {
public:
    ByteBufShim(char* buf, size_t size)
	: buf_(buf), size_(size), own_buf_(false) {}

    ByteBufShim(const Builder&) 
	: buf_(0), size_(0), own_buf_(false) {}

    ~ByteBufShim() {
	if (buf_ != 0 && own_buf_) {
	    free(buf_);
	}
    }

    // virtual from Formatter
    int format(char* buf, size_t sz) const {
        return snprintf(buf, sz, "NEED TO IMPLEMENT ByteBufShim::format");
    }

    // virtual from SerializableObject
    void serialize(SerializeAction* a)
    {
	a->process("bytes", (u_char**)&buf_,
                   &size_, Serialize::ALLOC_MEM);

        if (a->action_code() == Serialize::UNMARSHAL) {
            own_buf_ = true;
        }
    }
 
    const char* value() const { return buf_; }
    char*       take_buf()    { own_buf_ = false; return buf_; }
    size_t      size()  const { return size_; }

private:
    char*  buf_;
    size_t size_;    
    bool   own_buf_;
};

//----------------------------------------------------------------------------
/*!
 * This is used for generating prefixes for the SerializableObject
 * strings used as keys for tables. Instead of creating by hand
 * another kind of SerializableObject that introduces some kind of
 * prefixing system in the serialize call of the key, you can use
 * prefix_adapter:
 *
 * SerializableObject* real_key;
 * table->get(oasys::prefix_adapter(1000, real_key));
 *
 * Which will automatically append 1000 to the key entry.
 *
 */
template<typename _SerializablePrefix, typename _SerializableObject>
struct PrefixAdapter : public SerializableObject {
    PrefixAdapter(_SerializablePrefix* prefix,
                  _SerializableObject* obj)
        : prefix_(prefix),
          obj_(obj)
    {}
    
    void serialize(SerializeAction* a) 
    {
        a->process("prefix", prefix_);
        a->process("obj",    obj_);
    }

    _SerializablePrefix* prefix_;
    _SerializableObject* obj_;
};

template<typename _SerializablePrefix, typename _SerializableObject>
PrefixAdapter<_SerializablePrefix, _SerializableObject>
prefix_adapter(_SerializablePrefix* prefix,
               _SerializableObject* obj)
{
    return PrefixAdapter<_SerializablePrefix,
                         _SerializableObject>(prefix, obj);
}

}; // namespace oasys

#endif //__TYPE_SHIMS_H__
