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
#include "debug/Debug.h"
#include "debug/Log.h"
#include "util/StringUtils.h"
#include "util/CRC32.h"

#include "MarshalSerialize.h"

namespace oasys {

/******************************************************************************
 *
 * BufferedSerializeAction
 *
 *****************************************************************************/
BufferedSerializeAction::BufferedSerializeAction(
    action_t  action,
    context_t context,
    u_char*   buf, 
    size_t    length,
    int       options
    ) : SerializeAction(action, context, options),
      buf_(buf), length_(length), offset_(0)
{
}

/**
 * Return the next chunk of buffer. If there was a previous error or
 * if the buffer isn't big enough, set the error_ flag and return
 * NULL.
 */
u_char*
BufferedSerializeAction::next_slice(size_t length)
{
    
    if (error_)
        return NULL;
    
    if (offset_ + length > length_) {
        error_ = true;
        return NULL;
    }

    u_char* ret = &buf_[offset_];
    offset_ += length;
    return ret;
}

/******************************************************************************
 *
 * Marshal
 *
 *****************************************************************************/
Marshal::Marshal(context_t context, u_char* buf, size_t length, int options)
    : BufferedSerializeAction(MARSHAL, context, buf, length, options)
{
}

Marshal::Marshal(u_char* buf, size_t length)
    : BufferedSerializeAction(MARSHAL, CONTEXT_UNKNOWN, buf, length, 0)
{
}

void
Marshal::end_action()
{
    if (options_ & USE_CRC)
    {
        CRC32 crc;

        if (buf() != 0) 
        {
            crc.update(buf(), offset());
            CRC32::CRC_t crc_val = crc.value();
            process("crc", &crc_val);

            if (log_) {
                logf(log_, LOG_DEBUG, "crc32 is 0x%x", crc_val);
            }
        }
    }
}

void
Marshal::process(const char* name, u_int32_t* i)
{
    u_char* buf = next_slice(4);
    if (buf == NULL) return;

    buf[0] = ((*i)>>24) & 0xff;
    buf[1] = ((*i)>>16) & 0xff;
    buf[2] = ((*i)>>8)  & 0xff;
    buf[3] = (*i)       & 0xff;

    if (log_) logf(log_, LOG_DEBUG, "int32  %s=>(%d)", name, *i);
}

void 
Marshal::process(const char* name, u_int16_t* i)
{
    u_char* buf = next_slice(2);
    if (buf == NULL) return;

    buf[0] = ((*i)>>8) & 0xff;
    buf[1] = (*i)      & 0xff;
    
    if (log_) logf(log_, LOG_DEBUG, "int16  %s=>(%d)", name, *i);
}

void 
Marshal::process(const char* name, u_int8_t* i)
{
    u_char* buf = next_slice(1);
    if (buf == NULL) return;
    
    buf[0] = (*i);
    
    if (log_) logf(log_, LOG_DEBUG, "int8   %s=>(%d)", name, *i);
}

void 
Marshal::process(const char* name, bool* b)
{
    u_char* buf = next_slice(1);
    if (buf == NULL) return;

    buf[0] = (*b) ? 1 : 0;
    
    if (log_) logf(log_, LOG_DEBUG, "bool   %s=>(%c)", name, *b ? 'T' : 'F');
}

void 
Marshal::process(const char* name, u_char* bp, size_t len)
{
    u_char* buf = next_slice(len);
    if (buf == NULL) return;

    memcpy(buf, bp, len);
    if (log_) {
        std::string s;
        hex2str(&s, bp, len < 16 ? len : 16);
        logf(log_, LOG_DEBUG, "bufc   %s=>(%d: '%.*s')",
             name, len, (int)s.length(), s.data());
    }
}
    
void 
Marshal::process(const char* name, u_char** bp,
                 size_t* lenp, int flags)
{
    ASSERT(*lenp > 0 || (flags & Serialize::NULL_TERMINATED));
    int str_len;

    if(flags & Serialize::NULL_TERMINATED) {
        str_len = strlen(reinterpret_cast<char*>(*bp)) + 1;
    } else {
        process(name, (u_int32_t*)lenp);
        str_len = *lenp;
    }

    u_char* buf = next_slice(str_len);
    if (buf == NULL) return;
    
    memcpy(buf, *bp, str_len);
    
    if (log_) {
        std::string s;
        hex2str(&s, *bp, *lenp < 16 ? *lenp : 16);
        logf(log_, LOG_DEBUG, "bufc   %s=>(%d: '%.*s')",
             name, str_len, (int)s.length(), s.data());
    }
}

void 
Marshal::process(const char* name, std::string* s)
{
    size_t len = s->length();
    process(name, (u_int32_t*)&len);

    u_char* buf = next_slice(len);
    if (buf == NULL) return;
    
    memcpy(buf, s->data(), len);
    
    if (log_) {
        if (len < 32)
            logf(log_, LOG_DEBUG, "string %s=>(%d: '%.*s')",
                 name, len, (int)len, s->data());
        else 
            logf(log_, LOG_DEBUG, "string %s=>(%d: '%.*s'...)",
                 name, len, 32, s->data());
    }
}

/******************************************************************************
 *
 * Unmarshal
 *
 *****************************************************************************/
Unmarshal::Unmarshal(context_t context, const u_char* buf, size_t length,
                     int options)
    : BufferedSerializeAction(UNMARSHAL, context, 
                              (u_char*)(buf), length, options)
{
}

Unmarshal::Unmarshal(const u_char* buf, size_t length)
    : BufferedSerializeAction(UNMARSHAL, CONTEXT_UNKNOWN,
                              (u_char*)(buf), length, 0)
{
}

void
Unmarshal::begin_action()
{
    if (options_ & USE_CRC)
    {
        CRC32 crc;
        CRC32::CRC_t crc_val;
        
        crc_val = CRC32::from_bytes(buf() + length() -
                                    sizeof(CRC32::CRC_t)); 
        crc.update(buf(), length() - sizeof(CRC32::CRC_t));
        
        if (crc.value() != crc_val)
        {
            if (log_)
            {
                logf(log_, LOG_WARN, "crc32 mismatch, 0x%x != 0x%x",
                     crc.value(), crc_val);
                error_ = true;
            }
        }
        else
        {
            logf(log_, LOG_INFO, "crc32 is good");
        }
    }
}

void
Unmarshal::process(const char* name, u_int32_t* i)
{
    u_char* buf = next_slice(4);
    if (buf == NULL) return;
    
    *i = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];        
    if (log_) logf(log_, LOG_DEBUG, "int32  %s<=(%d)", name, *i);
}

void 
Unmarshal::process(const char* name, u_int16_t* i)
{
    u_char* buf = next_slice(2);
    if (buf == NULL) return;
    
    *i = (buf[0] << 8) | buf[1];        
    if (log_) logf(log_, LOG_DEBUG, "int16  %s<=(%d)", name, *i);
}

void 
Unmarshal::process(const char* name, u_int8_t* i)
{
    u_char* buf = next_slice(1);
    if (buf == NULL) return;
    
    *i = buf[0];        
    if (log_) logf(log_, LOG_DEBUG, "int8   %s<=(%d)", name, *i);
}

void 
Unmarshal::process(const char* name, bool* b)
{
    u_char* buf = next_slice(1);
    if (buf == NULL) return;
    
    *b = buf[0];
    if (log_) logf(log_, LOG_DEBUG, "bool   %s<=(%c)", name, *b ? 'T' : 'F');
}

void 
Unmarshal::process(const char* name, u_char* bp, size_t len)
{
    u_char* buf = next_slice(len);
    if (buf == NULL) return;
    
    memcpy(bp, buf, len);

    if (log_) {
        std::string s;
        hex2str(&s, bp, len < 16 ? len : 16);
        logf(log_, LOG_DEBUG, "bufc   %s<=(%d: '%.*s')",
             name, len, (int)s.length(), s.data());
    }
}

void 
Unmarshal::process(const char* name, u_char** bp, size_t* lenp, int flags)
{
    if(flags & Serialize::NULL_TERMINATED) {
        u_char* cbuf = buf() + offset();
        int new_len = 0;
	
        while(cbuf != buf() + length()) {
            if(*cbuf == '\0')
                break;
            cbuf++;
            new_len++;
        }
        
        if(cbuf == buf() + length()) {
            // no null character found
            error_ = true;
            return;
        }
        *lenp = new_len + 1; // length of string + '\0'
    } else {
        process(name, (u_int32_t*)lenp);
    }

    ASSERT(*lenp >= 0);
    ASSERT(*bp == 0);
    
    u_char* buf = next_slice(*lenp);
    if (buf == NULL) return;
    
    if (flags & Serialize::ALLOC_MEM) {
        *bp = (u_char*)malloc(*lenp);
        memcpy(*bp, buf, *lenp);
    } else {
        *bp = buf;
    }
    
    if (log_) {
        std::string s;
        hex2str(&s, *bp, *lenp < 16 ? *lenp : 16);
        logf(log_, LOG_DEBUG, "bufc   %s<=(%d: '%.*s')",
             name, *lenp, (int)s.length(), s.data());
    }
}

void 
Unmarshal::process(const char* name, std::string* s)
{
    size_t len;
    process(name, (u_int32_t*)&len);

    u_char* buf = next_slice(len);
    if (buf == NULL) return;
    
    s->assign((char*)buf, len);
    if (log_) {
        if (len < 32)
            logf(log_, LOG_DEBUG, "string %s<=(%d: '%.*s')",
                 name, len, (int)len, s->data());
        else 
            logf(log_, LOG_DEBUG, "string %s<=(%d: '%.*s'...)",
                 name, len, 32, s->data());
    }
}

/******************************************************************************
 *
 * MarshalSize 
 *
 *****************************************************************************/

void
MarshalSize::begin_action()
{
    if (options_ & USE_CRC) {
        size_ += sizeof(CRC32::CRC_t);
    }
}

void
MarshalSize::process(const char* name, u_int32_t* i)
{
    size_ += 4;
}

void
MarshalSize::process(const char* name, u_int16_t* i)
{
    size_ += 2;
}

void
MarshalSize::process(const char* name, u_int8_t* i)
{
    size_ += 1;
}

void
MarshalSize::process(const char* name, bool* b)
{
    size_ += 1;
}

void
MarshalSize::process(const char* name, u_char* bp, size_t len)
{
    size_ += len;
}

void
MarshalSize::process(const char* name, u_char** bp,
                     size_t* lenp, int flags)
{
    if(flags & Serialize::NULL_TERMINATED) {
        size_ += strlen(reinterpret_cast<char*>(*bp)) + sizeof('\0');
    } else {
        size_ += *lenp + sizeof(u_int32_t);
    }
}

void
MarshalSize::process(const char* name, std::string* s)
{
    size_ += s->length() + 4;
}



/******************************************************************************
 *
 * MarshalCRC
 *
 *****************************************************************************/
#define DECL_CRC(_type)                         \
void                                            \
MarshalCRC::process(const char* name, _type* i) \
{                                               \
    crc_.update((u_char*)i, sizeof(_type));     \
}

DECL_CRC(u_int32_t)
DECL_CRC(u_int16_t)
DECL_CRC(u_int8_t)
DECL_CRC(bool)

void
MarshalCRC::process(const char* name, u_char* bp, size_t len)
{
    crc_.update(bp, len);
}

void
MarshalCRC::process(const char* name, u_char** bp,
                     size_t* lenp, int flags)
{
    if(flags & Serialize::NULL_TERMINATED) {
        crc_.update(*bp, strlen(reinterpret_cast<char*>(*bp)));
    } else {
        crc_.update(*bp, *lenp);
    }
}

void
MarshalCRC::process(const char* name, std::string* s)
{
    crc_.update((u_char*)s->c_str(), s->size());
}

} // namespace oasys
