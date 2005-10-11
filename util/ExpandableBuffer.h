#ifndef __EXPANDABLEBUFFER_H__
#define __EXPANDABLEBUFFER_H__

#include "../debug/DebugUtils.h"

namespace oasys {

/*!
 * ExpandableBuffer useful for pieces of memory that need to be
 * resizable. It is to be used as an interface and a base class but
 * not directly.
 */
struct ExpandableBuffer {
#define GUARD_STR  "!ExpBuf!"
#define GUARD_SIZE 8
    ExpandableBuffer(size_t size = 0) 
        : buf_(0), buf_len_(0), len_(0) 
    {
        if (size != 0) {
            reserve(size);
        }
    }

    virtual ~ExpandableBuffer() { 
        if (buf_ != 0) {
            ASSERT(check_guards());

            unpost_guards();
            free(buf_);
            buf_ = 0;
        }

        buf_len_ = 0; 
        len_     = 0; 
    }

    /*!
     * Reserve buffer space.
     *
     * @param size The size of the buffer desired. Default is double
     *     the size.
     *
     * @return 0 on success.
     */
    virtual int reserve(size_t size = 0) {
        if (size == 0) {
            size = (buf_len_ == 0) ? 1 : (buf_len_ * 2);
        }        

        if (size > buf_len_) {
            ASSERT(check_guards());
            unpost_guards();
            buf_ = static_cast<char*>
                   (realloc(buf_, buf_len_ + 2 * GUARD_SIZE));
            buf_len_ = size;
            post_guards();
        }
            
        if (buf_ == 0) {
            return -1;
        }

        ASSERT(check_guards());

        return 0;
    }

    //! @return bytes free
    int nfree() const {
        ASSERT(buf_len_ >= len_);
        ASSERT(check_guards());

        return buf_len_ - len_;
    }
    
    //! @return raw char buffer
    char* raw_buf() const {
        ASSERT(buf_ != 0); 
        ASSERT(check_guards());

        return buf_; 
    }

    //! @return char* to offset in the buffer
    char* at(size_t offset) const { 
        ASSERT(buf_ != 0);
        ASSERT(offset < buf_len_);
        ASSERT(check_guards());

        return &buf_[offset]; 
    }
   
    //! @return char* to end of len_ bytes in the buffer
    char* end() const { 
        ASSERT(buf_ != 0);
        ASSERT(len_ < buf_len_);
        ASSERT(check_guards());

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

    bool check_guards() const
    {
        if (buf_ != 0) 
        {
            char* guard = buf_ - GUARD_SIZE;
            return (memcmp(guard, GUARD_STR, GUARD_SIZE) == 0 &&
                    memcmp(buf_ + buf_len_, GUARD_STR, GUARD_SIZE) == 0);
        }

        return true;
    }

protected:
    char*  buf_;
    size_t buf_len_;
    size_t len_;

    void post_guards() 
    {
        if (buf_ != 0) {
            memcpy(buf_, GUARD_STR, GUARD_SIZE);
            buf_ += GUARD_SIZE;
            memcpy(buf_ + buf_len_, GUARD_STR, GUARD_SIZE);
        }
    }

    void unpost_guards() 
    {
        if (buf_ != 0) {
            memset(buf_ + buf_len_, 0, GUARD_SIZE);
            buf_ -= GUARD_SIZE;
            memset(buf_,  0, GUARD_SIZE);
        }
    }
};

#undef GUARD_STR
#undef GUARD_SIZE

} // namespace oasys

#endif /* __EXPANDABLEBUFFER_H__ */
