#ifndef __EXPANDABLEBUFFER_H__
#define __EXPANDABLEBUFFER_H__

#include "../debug/DebugUtils.h"

namespace oasys {

struct ExpandableBuffer {
    ExpandableBuffer() 
        : buf_(0), buf_len_(0), len_(0) 
    {}

    ~ExpandableBuffer() { 
        delete_z(buf_); 
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
    int reserve(size_t size = 0, size_t grow = 0) {
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

    int nfree() { 
        return buf_len_ - len_; 
    }
    
    char* buf_at(size_t offset) { 
        return &buf_[offset]; 
    }
   
    char* buf_end() { 
        return buf_at(len_); 
    }
    
    void add_to_len(int amount) { 
        len_ += amount; 
    }

    char*  buf_;
    size_t buf_len_;
    size_t len_;
};

} // namespace oasys

#endif /* __EXPANDABLEBUFFER_H__ */
