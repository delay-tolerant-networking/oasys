#include <oasys/debug/Debug.h>
#include "ScratchBuffer.h"


oasys::ScratchBuffer::ScratchBuffer(size_t size)
    : buf_(0), size_(size)
{
    if(size_ > 0)
    {
        buf_ = static_cast<char*>(malloc(size_));
    }
}


void*
oasys::ScratchBuffer::buf(size_t size)
{
    if(size > size_)
    {
        buf_ = realloc(buf_, size);
        
    }

    return buf_;
}

void*
oasys::ScratchBuffer::buf()
{
    return buf_;
}
