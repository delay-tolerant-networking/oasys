// XXX/demmer add copyright
#ifndef tier_md5_h
#define tier_md5_h

#include <sys/types.h>
#include <string>
#include "StringUtils.h"

extern "C" {
#define PROTOTYPES 1
#include "md5.h"
}

#define MD5LEN 16

class MD5 {
public:
    MD5() {
        init();
    }
    ~MD5() {}

    void init()
    {
        MD5Init(&ctx_);
    }
    
    void update(const u_char* data, size_t len)
    {
        MD5Update(&ctx_, (u_char*)data, len);
    }

    void update(const char* data, size_t len)
    {
        MD5Update(&ctx_, (u_char*)data, len);
    }

    void finalize()
    {
        MD5Final(digest_, &ctx_);
    }
    
    const u_char* digest()
    {
        return digest_;
    }

    static void digest_ascii(std::string* str,
                             const u_char* digest)
    {
        hex2str(str, digest, MD5LEN);
    }

    static std::string digest_ascii(const u_char* digest)
    {
        std::string str;
        digest_ascii(&str, digest);
        return str;
    }

    void digest_ascii(std::string* str)
    {
        digest_ascii(str, digest_);
    }

    std::string digest_ascii()
    {
        return digest_ascii(digest_);
    }

    static void digest_fromascii(const char* str, u_char* digest)
    {
        str2hex(str, digest, MD5LEN);
    }

private:
    MD5_CTX ctx_;
    u_char digest_[MD5LEN];
};

#endif
