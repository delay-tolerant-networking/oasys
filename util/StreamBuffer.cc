#include "lib/Debug.h"
#include "StreamBuffer.h"

/***************************************************************************
 *
 * StreamBuffer
 *
 **************************************************************************/
StreamBuffer::StreamBuffer(size_t size) : 
    start_(0), end_(0), size_(size)
{
    if(size_ == 0)
        size_ = 4;

    buf_ = static_cast<char*>(malloc(size_));
}

void
StreamBuffer::set_size(size_t size)
{
    ASSERT(fullbytes() <= size);
    moveup();
    
    realloc(size);
}

char&
StreamBuffer::operator[](size_t offset)
{
    ASSERT(offset + start_ < end_);
    ASSERT(buf_ != 0);
   
    return buf_[offset + start_];
}

char*
StreamBuffer::buffer()
{
    return &buf_[start_];
}

void
StreamBuffer::fill(size_t amount)
{
    if(amount <= size_ - end_) 
    {
        // do nothing
    } 
    else if(amount <= start_) 
    {
        moveup();
    } 
    else
    {
        moveup();
        realloc((amount > size_*2) ? amount : (size_*2));
    }

    end_ += amount;
}

void
StreamBuffer::consume(size_t amount)
{
    ASSERT(amount <= fullbytes());

    start_ += amount;
    if(start_ == end_)
    {
        start_ = end_ = 0;
    }
}

void
StreamBuffer::clear()
{
    start_ = end_ = 0;
}

size_t
StreamBuffer::fullbytes() 
{
    return end_ - start_;
}

size_t
StreamBuffer::emptybytes() 
{
    return size_ - end_ + start_;
}

void
StreamBuffer::realloc(size_t size)
{
    if(size < size_)
        return;

    buf_ = (char*)::realloc(buf_, size);
    if(buf_ == 0) 
    {
        logf("/StreamBuffer", LOG_CRIT, "Out of memory");
        ASSERT(0);
    }
    
    size_ = size;
}

void
StreamBuffer::moveup()
{
    memmove(&buf_[0], &buf_[start_], end_ - start_);
    end_   = end_ - start_;
    start_ = 0;
}
