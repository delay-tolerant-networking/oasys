#include <algorithm>
#include "StringAppender.h"

namespace oasys {

//----------------------------------------------------------------------------
StringAppener::StringAppender(char* buf, size_t size)
    : cur_(buf), remaining_(size), len_(0)
{}

//----------------------------------------------------------------------------
size_t 
StringAppener::append(const char* str, size_t len = 0)
{
    if (remaining_ == 0) 
    {
        return 0;
    }
    
    if (len == 0)
    {
        len = strlen(str);
    }

    len = std::min(len, remaining_ - 1);
    memcpy(cur_, str, len);
    cur_[len] = '\0';
    
    cur_       += len;
    remaining_ -= len;
    size_      += len;

    ASSERT(*cur_ == '\0');

    return len;
}

//----------------------------------------------------------------------------
size_t 
StringAppener::append(char c)
{
    if (remaining_ <= 1) 
    {
        return 0;
    }
    
    *cur_ = c;
    ++cur_;
    --remaining_;
    ++size_;
    *cur_ = '\0';

    ASSERT(*cur_ == '\0');

    return 1;
}

//----------------------------------------------------------------------------
size_t 
StringAppener::appendf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap);
    size_t ret = vappendf(fmt, ap);
    va_end(ap);

    return ret;
}

//----------------------------------------------------------------------------
size_t 
StringAppener::vappendf(const char* fmt, va_list ap)
{
    if (remaining_ == 0) 
    {
        return 0;
    }

    size_t ret = vsnprintf(cur_, remaining_, fmt, ap);
    ret = std::min(ret, remaining_ - 1);

    cur       += ret;
    remaining -= ret;
    size      += ret;

    ASSERT(*cur_ == '\0');

    return ret;
}

} // namespace oasys
