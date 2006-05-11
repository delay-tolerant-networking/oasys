#include "KeySerialize.h"

namespace oasys {

//////////////////////////////////////////////////////////////////////////////
KeyMarshal::KeyMarshal(ExpandableBuffer* buf,
                       const char*       border)
    : SerializeAction(Serialize::MARSHAL, Serialize::CONTEXT_LOCAL),
      buf_(buf),
      border_(border)
{}
    
//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char* name,
                    u_int32_t*  i)
{
    (void)name;
    process_int(*i, 8, "%08x");
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char* name,
                    u_int16_t*  i)
{
    (void)name;
    process_int(*i, 4, "%04x");
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char* name,
                    u_int8_t*   i)
{
    (void)name;
    process_int(*i, 2, "%02x");
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char* name,
                    bool*       b)
{
    (void)name;
    process_int( (*b) ? 1 : 0, 1, "%1u");
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char* name,
                    u_char*     bp,
                    size_t      len)
{
    (void)name;
    if (error()) 
        return;

    buf_->reserve(buf_->len() + len);
    memcpy(buf_->end(), bp, len);
    buf_->set_len(buf_->len() + len);
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char* name,
                    u_char**    bp,
                    size_t*     lenp,
                    int         flags)
{
    (void)name;
    if (error()) 
        return;

    ASSERT(! (lenp == 0 && ! (flags & Serialize::NULL_TERMINATED)));

    size_t len;
    if (flags & Serialize::NULL_TERMINATED) {
        len = strlen(reinterpret_cast<char*>(*bp));
    } else {
        len = *lenp;
    }

    process_int(len, 8, "%08x");

    buf_->reserve(buf_->len() + len);
    memcpy(buf_->end(), *bp, len);
    buf_->set_len(buf_->len() + len);
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char*  name,
                    std::string* s)
{
    (void)name;
    if (error()) 
        return;

    process_int(s->length(), 8, "%08x");
    buf_->reserve(buf_->len() + s->size());
    memcpy(buf_->end(), s->c_str(), s->size());
    buf_->set_len(buf_->len() + s->size());
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char*         name,
                    SerializableObject* object)
{
    (void)name;
    if (error()) 
        return;

    int err = action(object);
    if (err != 0) {
        signal_error();
    }
    border();
}

//////////////////////////////////////////////////////////////////////////////
void
KeyMarshal::end_action()
{
    buf_->reserve(1);
    *(buf_->end()) = '\0';
}

//////////////////////////////////////////////////////////////////////////////
void
KeyMarshal::process_int(u_int32_t i, size_t size, const char* format)
{
    if (error()) 
        return;

    buf_->reserve(buf_->len() + size + 1);
    int cc = snprintf(buf_->end(), size + 1, format, i);
    ASSERT(cc == (int)size);
    buf_->set_len(buf_->len() + size);
}

void
KeyMarshal::border()
{
    if (error() || border_ == 0) {
        return;
    }

    size_t border_len = strlen(border_);
    buf_->reserve(border_len);
    memcpy(buf_->end(), border_, border_len);
    buf_->set_len(buf_->len() + border_len);
}

//////////////////////////////////////////////////////////////////////////////
KeyUnmarshal::KeyUnmarshal(const char* buf,
                           size_t      buf_len,
                           const char* border)
    : SerializeAction(Serialize::UNMARSHAL, Serialize::CONTEXT_LOCAL),
      buf_(buf),
      buf_len_(buf_len),
      border_len_( (border == 0) ? 0 : strlen(border)),
      cur_(0)
{}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, u_int32_t* i)
{
    (void)name;
    u_int32_t val = process_int(8);
    if (! error()) {
        *i = val;
    }
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, u_int16_t* i)
{
    (void)name;
    u_int16_t val = static_cast<u_int16_t>(process_int(4));
    if (! error()) {
        *i = val;
    }
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, u_int8_t* i)
{
    (void)name;
    u_int8_t val = static_cast<u_int8_t>(process_int(2));
    if (! error()) {
        *i = val;
    }
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, bool* b)
{
    (void)name;
    if (error()) {
        return;
    }

    if (cur_ + 1 > buf_len_) {
        signal_error();
        return;
    }
    
    *b = (buf_[cur_] == '1') ? true : false;
    cur_ += 1;
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, u_char* bp, size_t len)
{
    (void)name;
    if (error()) {
        return;
    }

    if (cur_ + len > buf_len_) {
        signal_error();
        return;
    }

    memcpy(bp, &buf_[cur_], len);
    cur_ += len;
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, u_char** bp, 
                      size_t* lenp, int flags)
{
    (void)name;
    if (error()) {
        return;
    }

    size_t len = process_int(8);
    if (error()) {
        return;
    }
    
    if (flags & Serialize::ALLOC_MEM) {
        size_t malloc_len = (flags & Serialize::NULL_TERMINATED) ? 
                            len + 1 : len;
        *bp = static_cast<u_char*>(malloc(malloc_len));
        if (*bp == 0) {
            signal_error();
            return;
        }
    }

    ASSERT(*bp);
    if (lenp) {
        *lenp = len;
    }
    
    if (cur_ + len > buf_len_) {
        signal_error();
        return;
    }

    if (flags & Serialize::NULL_TERMINATED) {
        memcpy(*bp, &buf_[cur_], len);
        (*bp)[len] = '\0';
    } else {
        memcpy(*bp, &buf_[cur_], len);
    }
    
    cur_ += len;
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, std::string* s) 
{
    (void)name;
    if (error()) {
        return;
    }

    size_t len = process_int(8);
    if (error()) {
        return;
    }

    s->assign(&buf_[cur_], len);
    cur_ += len;
    border();
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, SerializableObject* object)
{
    (void)name;
    if (error()) {
        return;
    }

    if (action(object) != 0) {
        signal_error();
    }
    border();
}

//////////////////////////////////////////////////////////////////////////////
u_int32_t
KeyUnmarshal::process_int(size_t size)
{
    char buf[9];

    if (cur_ + size > buf_len_) {
        signal_error();
        return 0;
    }

    memset(buf, 0, 9);
    memcpy(buf, &buf_[cur_], size);
    
    char* endptr;
    u_int32_t val = strtoul(buf, &endptr, 16);
    
    if (endptr == &buf_[cur_]) {
        signal_error();
        return 0;
    }

    cur_ += size;

    return val;
}

//////////////////////////////////////////////////////////////////////////////
void
KeyUnmarshal::border()
{
    cur_ += border_len_;
}

} // namespace oasys
