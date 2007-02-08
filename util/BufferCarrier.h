#ifndef __BUFFERCARRIER_H__
#define __BUFFERCARRIER_H__

#include "../debug/DebugUtils.h"

namespace oasys {

/*!
 * This little wart of a class handles the case when a virtual
 * function interface can sometimes return a buffer without allocation
 * of memory but sometimes must, depending on the implementation of
 * the subclass.
 * 
 *
 */
template<typename _Type>
class BufferCarrier {
    NO_ASSIGN_COPY(BufferCarrier);

public:
    /*!
     * @param owned True iff you are passing ownership of the buffer.
     */
    BufferCarrier(_Type* buf, bool owned)
        : buf_(buf), 
          owned_(owned) 
    {}

    /*!
     * If we own the buffer, then destroy it when we die.
     */
    ~BufferCarrier() 
    { 
        if (owned_) { 
            free(buf_);
            buf_ = 0;
        }
    }

    _Type* buf()   
    { 
        return buf_; 
    }
    
    bool owned() 
    { 
        return owned_; 
    }

    /*!
     * Set buffer and _maybe_ ownership.
     */
    void set_buf(_Type* buf, bool pass_ownership)
    {
        buf_   = buf;
        owned_ = pass_ownership;
    }
    
    /*!
     * We must own the buffer in order to give it away.
     */
    _Type* take_buf() 
    { 
        ASSERT(owned_);
        
        _Type* ret = buf_;
        owned_     = 0;
        buf_       = 0;

        return ret;
    }
        
private:
    _Type* buf_;
    bool   owned_;
};

} // namespace oasys

#endif /* __BUFFERCARRIER_H__ */
