#include "../debug/Debug.h"
#include "ScratchBuffer.h"


oasys::ScratchBuffer::ScratchBuffer(size_t size)
    : buf_(0), size_(size)
{
    if(size_ > 0)
    {
        buf_ = static_cast<char*>(malloc(size_));
    }
}


char*
oasys::ScratchBuffer::buf(size_t size)
{
    if(size > size_)
    {
        buf_ = static_cast<char*>(realloc(buf_, size));
        
    }

    return buf_;
}
