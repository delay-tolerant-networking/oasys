#include "BufferedSerializeAction.h"

namespace oasys {
/******************************************************************************
 *
 * BufferedSerializeAction
 *
 *****************************************************************************/
BufferedSerializeAction::BufferedSerializeAction(
    action_t  action,
    context_t context,
    u_char*   buf, 
    size_t    length,
    int       options
    ) : SerializeAction(action, context, options),
      buf_(buf), length_(length), offset_(0)
{
}

/**
 * Return the next chunk of buffer. If there was a previous error or
 * if the buffer isn't big enough, set the error_ flag and return
 * NULL.
 */
u_char*
BufferedSerializeAction::next_slice(size_t length)
{
    
    if (error())
        return NULL;
    
    if (offset_ + length > length_) {
        signal_error();
        return NULL;
    }

    u_char* ret = &buf_[offset_];
    offset_ += length;
    return ret;
}

} // namespace oasys
