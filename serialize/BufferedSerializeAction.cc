#include "BufferedSerializeAction.h"
#include "util/ExpandableBuffer.h"

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
    int       options)

    : SerializeAction(action, context, options),
      expandable_buf_(NULL),
      buf_(buf), length_(length), offset_(0)
{
}

BufferedSerializeAction::BufferedSerializeAction(
    action_t          action,
    context_t         context,
    ExpandableBuffer* buf,
    int               options)

    : SerializeAction(action, context, options),
      expandable_buf_(buf),
      buf_(NULL), length_(0), offset_(0)
{
    expandable_buf_->set_len(0);
}

u_char*
BufferedSerializeAction::buf()
{
    return (expandable_buf_ ? (u_char*)expandable_buf_->raw_buf() : buf_);
}

size_t
BufferedSerializeAction::length()
{
    return (expandable_buf_ ? expandable_buf_->buf_len() : length_);
}

size_t
BufferedSerializeAction::offset()
{
    return (expandable_buf_ ? expandable_buf_->len() : offset_);
}

/**
 * Return the next chunk of buffer. If there was a previous error or
 * if we're in fixed-length mode and the buffer isn't big enough, set
 * the error_ flag and return NULL.
 */
u_char*
BufferedSerializeAction::next_slice(size_t length)
{
    u_char* ret;
    
    if (error())
        return NULL;
    
    if (expandable_buf_ != NULL) {
        ret = (u_char*)expandable_buf_->tail_buf(length);
        expandable_buf_->incr_len(length);
        return ret;
    }
    
    if (offset_ + length > length_) {
        signal_error();
        return NULL;
    }

    ret = &buf_[offset_];
    offset_ += length;
    return ret;
}

} // namespace oasys
