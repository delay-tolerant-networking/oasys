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

#ifndef _OASYS_STRING_UTILS_H_
#define _OASYS_STRING_UTILS_H_

/**
 * Utilities and stl typedefs for basic_string.
 */

#include <ctype.h>
#include <string>
#include <vector>
#include <set>
#include <map>

// Though hash_set was part of std:: in the 2.9x gcc series, it's been
// moved to ext/__gnu_cxx:: in 3.x
#if (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#include <hash_set>
#include <hash_map>
#define _std std
#else
#include <ext/hash_set>
#include <ext/hash_map>
#define _std __gnu_cxx
#endif

namespace oasys {

/**
 * Hashing function class for std::strings.
 */
struct StringHash {
    size_t operator()(const std::string& str) const
    {
        return _std::__stl_hash_string(str.c_str());
    }
};

/**
 * Less than function.
 */
struct StringLessThan {
    bool operator()(const std::string& str1, const std::string& str2) const
    {
        return (str1.compare(str2) < 0);
    }
};

/**
 * Greater than function.
 */
struct StringGreaterThan {
    bool operator()(const std::string& str1, const std::string& str2) const
    {
        return (str1.compare(str2) > 0);
    }
};

/**
 * Equality function class for std::strings.
 */
struct StringEquals {
    bool operator()(const std::string& str1, const std::string& str2) const
    {
        return (str1 == str2);
    }
};

/**
 * A StringSet is a set with std::string members
 */
class StringSet : public std::set<std::string, StringLessThan> {
public:
    void dump(const char* log) const;
};

/**
 * A StringMap is a map with std::string keys.
 */
template <class _Type> class StringMap :
    public std::map<std::string, _Type, StringLessThan> {
};

/**
 * A StringMultiMap is a multimap with std::string keys.
 */
template <class _Type> class StringMultiMap :
    public std::multimap<std::string, _Type, StringLessThan> {
};

/**
 * A StringHashSet is a hash set with std::string members.
 */
class StringHashSet :
        public _std::hash_set<std::string, StringHash, StringEquals> {
public:
    void dump(const char* log) const;
};

/**
 * A StringHashMap is a hash map with std::string keys.
 */
template <class _Type> class StringHashMap :
    public _std::hash_map<std::string, _Type, StringHash, StringEquals> {
};

/**
 * A StringVector is a std::vector of std::strings.
 */
class StringVector : public std::vector<std::string> {
};

/**
 * Tokenize a single string into a vector.
 * Return the number of tokens parsed.
 */
int
tokenize(const std::string& str,
         const std::string& sep,
         std::vector<std::string>* tokens);

/**
 * Generate a hex string from a binary buffer.
 */
inline void
hex2str(std::string* str, const u_char* bp, size_t len)
{
    static const char hex[] = "0123456789abcdef";
    str->erase();
    for (size_t i = 0; i < len; ++i) {
        str->push_back(hex[(bp[i] >> 4) & 0xf]);
        str->push_back(hex[bp[i] & 0xf]);
    }
}

/**
 * Parse a hex string into a binary buffer. Results undefined if the
 * string contains characters other than [0-9a-f].
 */
inline void
str2hex(const std::string& str, u_char* bp, size_t len)
{
#define HEXTONUM(x) ((x) < 'a' ? (x) - '0' : x - 'a' + 10)
    const char* s = str.data();
    for (size_t i = 0; i < len; ++i) {
        bp[i] = (HEXTONUM(s[2*i]) << 4) + HEXTONUM(s[2*i + 1]);
    }
#undef HEXTONUM
}

/**
 * Return true if the string contains only printable characters.
 */
inline bool
str_isascii(const u_char* bp, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        if (!isascii(*bp++)) {
            return false;
        }
    }

    return true;
}

/**
 * Convert an unsigned long to ascii in the given base. The pointer to
 * the tail end of an adequately sized buffer is supplied, and the
 * number of characters written is returned.
 *
 * Implementation largely copied from the FreeBSD 5.0 distribution.
 *
 * @return the number of bytes used or the number that would be used
 * if there isn't enough space in the buffer
 */
inline size_t
fast_ultoa(unsigned long val, int base, char* endp)
{
#define	to_char(n)	((n) + '0')
    char* cp = endp;
    long sval;
    
    static const char xdigs[] = "0123456789abcdef";
    
    switch(base) {
    case 10:
        // optimize one-digit numbers
        if (val < 10) {
            *cp = to_char(val);
            return 1;
        }

        // according to the FreeBSD folks, signed arithmetic may be
        // faster than unsigned, so do at most one unsigned mod/divide
        if (val > LONG_MAX) {
            *cp-- = to_char(val % 10);
            sval = val / 10;
        } else {
            sval = val;
        }
        
        do {
            *cp-- = to_char(sval % 10);
            sval /= 10;
        } while (sval != 0);

        break;
    case 16:
        do {
            *cp-- = xdigs[val & 15];
            val >>= 4;
        } while (val != 0);
        break;

    default:
        return 0; // maybe should do NOTREACHED??
    }

    return endp - cp;

#undef to_char
}

} // namespace oasys

#endif /* _OASYS_STRING_UTILS_H_ */
