#ifndef __EXPANDABLEBUFFER_H__
#define __EXPANDABLEBUFFER_H__

#include "../debug/DebugUtils.h"

namespace oasys {

struct ExpandableBuffer {
    ExpandableBuffer(size_t size = 0) 
        : buf_(0), buf_len_(0), len_(0) 
    {
        if (size != 0) {
            reserve(size, size);    
        }
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
     * @param size the minimum required free buffer space (default is
     *     2x buflen)
     * @param grow size to grow to (default is 2x buflen)
     *
     * @return 0 on success.
     */
    virtual int reserve(size_t size = 0, size_t grow = 0) {
        if (size == 0) {
            size = buf_len_ * 2;
        }
        
        if ((len_ + size) > buf_len_) {
            if (grow == 0) {
                grow = buf_len_ * 2;
            }
            
            ASSERT(grow > buf_len_);
            buf_len_ = grow;

            while ((len_ + size) > buf_len_) {
                buf_len_ *= 2;
                
                // bump buf_len_ > 0, o.w. we have an infinite loop
                if (buf_len_ == 0) 
                {
                    buf_len_++;
                }
            }
            
            buf_ = (char*)realloc(buf_, buf_len_);
            if (buf_ == 0) {
                return -1;
            }
        }
        
        return 0;
    }

    //! @return bytes free
    int nfree() const { return buf_len_ - len_; }
    
    //! @return char* to offset in the buffer
    char* buf_at(size_t offset) const { return &buf_[offset]; }
   
    //! @return char* to end of len_ bytes in the buffer
    char* buf_end() const { return buf_at(len_); }
    
    //! @return Length of the scratch buffer
    size_t buf_len() const { return buf_len_; }

    //! @return Length of the bytes that have been requested
    size_t len() const { return len_; }

    //! Set the length to this amount
    void set_len(size_t len) { len_ = len; }

    //! @return raw char buffer
    char* raw_buf() const { return buf_; }

    //! Add amount to the length
    void add_to_len(int amount) {
        len_ += amount; 
        ASSERT(len_ <= buf_len_);
    }

protected:
    char*  buf_;
    size_t buf_len_;
    size_t len_;
};

} // namespace oasys

#endif /* __EXPANDABLEBUFFER_H__ */
