#ifndef __EXPANDABLEBUFFER_H__
#define __EXPANDABLEBUFFER_H__

#include <cstring>

#include "../debug/DebugUtils.h"

namespace oasys {

/*!
 * ExpandableBuffer useful for pieces of memory that need to be
 * resizable. It is to be used as an interface and a base class but
 * not directly.
 */
struct ExpandableBuffer {
    ExpandableBuffer(size_t size = 0) 
        : buf_(0), buf_len_(0), len_(0) 
    {
        if (size != 0) {
            reserve(size);
        }
    }

    ExpandableBuffer(const ExpandableBuffer& other) 
        : buf_(0), buf_len_(0), len_(0)
    {
        if (other.buf_ == 0) {
            return;
        }

        reserve(other.buf_len_);
        memcpy(other.buf_, buf_, buf_len_);
        len_ = other.len_;
    }

    virtual ~ExpandableBuffer() { 
        if (buf_ != 0) {
            free(buf_);
            buf_ = 0;
        }

        buf_len_ = 0; 
        len_     = 0; 
    }

    /*!
     * Reserve buffer space.
     *
     * @param size The size of the buffer desired
     *
     * @return 0 on success.
     */
    virtual void reserve(size_t size) {
        if (size > buf_len_) {
            buf_ = static_cast<char*>(realloc(buf_, size));
            if (buf_ == 0) {
                PANIC("out of memory");
            }
            buf_len_ = size;
        }
    }

    //! @return bytes free
    int nfree() const {
        ASSERT(buf_len_ >= len_);
        return buf_len_ - len_;
    }
    
    //! @return raw char buffer
    char* raw_buf() const {
        ASSERT(buf_ != 0); 
        return buf_; 
    }

    //! @return char* to offset in the buffer, 0 if past the end of
    //! the buffer.
    char* at(size_t offset) const { 
        ASSERT(buf_ != 0);

        if (offset >=  buf_len_) {
            return 0;
        }

        return &buf_[offset]; 
    }
   
    //! @return char* to end of len_ bytes in the buffer
    char* end() const { 
        ASSERT(buf_ != 0);
        ASSERT(len_ < buf_len_);
        return at(len_); 
    }
    
    //! @return Length of the scratch buffer
    size_t buf_len() const { 
        return buf_len_; 
    }

    //! @return Length of the bytes that have been requested
    size_t len() const { return len_; }

    //! Set the length to this amount
    void set_len(size_t len) { 
        len_ = len; 
        ASSERT(len_ <= buf_len_);
    }

    void clear() {
        set_len(0);
    }

protected:
    char*  buf_;
    size_t buf_len_;
    size_t len_;
};

} // namespace oasys

#endif /* __EXPANDABLEBUFFER_H__ */
