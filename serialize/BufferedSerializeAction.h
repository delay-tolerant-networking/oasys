#ifndef __BUFFEREDSERIALIZEACTION_H__
#define __BUFFEREDSERIALIZEACTION_H__

#include "Serialize.h"

namespace oasys {

//////////////////////////////////////////////////////////////////////////////
/**
 * Common base class for Marshal and Unmarshal that manages the flat
 * buffer.
 */
class BufferedSerializeAction : public SerializeAction {
public:
    BufferedSerializeAction(action_t action, context_t context,
                            u_char* buf, size_t length, 
                            int options = 0);

    /** 
     * Since BufferedSerializeAction ignores the name field, calling
     * process() on a contained object is the same as just calling the
     * contained object's serialize() method.
     */
    virtual void process(const char* name, SerializableObject* object)
    {
        object->serialize(this);
    }
    
protected:
    /**  
     * Get the next R/W length of the buffer.
     *
     * @return R/W buffer of size length or NULL on error
     */
    u_char* next_slice(size_t length);
    
    /** @return buffer */
    u_char* buf() { return error() ? 0 : buf_; }

    /** @return buffer length */
    size_t length() { return length_; }
    
    /** @return Current offset into buf */
    size_t offset() { return offset_; }

 private:
    u_char* buf_;		///< Buffer that is un/marshalled
    size_t  length_;		///< Length of the buffer.
    size_t  offset_;
};

} // namespace oasys

#endif /* __BUFFEREDSERIALIZEACTION_H__ */
