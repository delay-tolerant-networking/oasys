#include "KeySerialize.h"

namespace oasys {

//////////////////////////////////////////////////////////////////////////////
KeyMarshal::KeyMarshal(context_t         context,
                       ExpandableBuffer* buf)
    : SerializeAction(Serialize::MARSHAL, context),
      buf_(buf)
{}
    
//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char* name,
                    u_int32_t*  i)
{
    process_int(*i, 8, "%08x");
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char* name,
                    u_int16_t*  i)
{
    process_int(*i, 4, "%04x");
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char* name,
                    u_int8_t*   i)
{
    process_int(*i, 2, "%02x");
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char* name,
                    bool*       b)
{
    process_int( (*b) ? 1 : 0, 1, "%1u");
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char* name,
                    u_char*     bp,
                    size_t      len)
{
    if (error()) 
        return;

    buf_->reserve(buf_->len() + len);
    memcpy(buf_->end(), bp, len);
    buf_->set_len(buf_->len() + len);
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char* name,
                    u_char**    bp,
                    size_t*     lenp,
                    int         flags)
{
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
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char*  name,
                    std::string* s)
{
    if (error()) 
        return;

    process_int(s->length(), 8, "%08x");
    buf_->reserve(buf_->len() + s->size());
    memcpy(buf_->end(), s->c_str(), s->size());
    buf_->set_len(buf_->len() + s->size());
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyMarshal::process(const char*         name,
                    SerializableObject* object)
{
    if (error()) 
        return;

    int err = action(object);
    if (err != 0) {
        signal_error();
    }
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

//////////////////////////////////////////////////////////////////////////////
KeyUnmarshal::KeyUnmarshal(context_t         context,
                           ExpandableBuffer* buf)
    : SerializeAction(Serialize::UNMARSHAL, context),
      buf_(buf)
{}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, u_int32_t* i)
{
    u_int32_t val = process_int(8);
    if (! error()) {
        *i = val;
    }
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, u_int16_t* i)
{
    u_int16_t val = static_cast<u_int16_t>(process_int(4));
    if (! error()) {
        *i = val;
    }
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, u_int8_t* i)
{
    u_int8_t val = static_cast<u_int8_t>(process_int(2));
    if (! error()) {
        *i = val;
    }
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, bool* b)
{
    if (error()) {
        return;
    }

    if (cur_ + 1 > buf_->len()) {
        signal_error();
        return;
    }
    
    cur_ += 1;
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, u_char* bp, size_t len)
{
    if (error()) {
        return;
    }

    if (cur_ + len > buf_->len()) {
        signal_error();
        return;
    }

    memcpy(bp, buf_->at(cur_), len);
    cur_ += len;
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, u_char** bp, 
                      size_t* lenp, int flags)
{
    if (error()) {
        return;
    }

    size_t len = process_int(8);
    if (error()) {
        return;
    }
    
    if (flags & Serialize::ALLOC_MEM) {
        *bp = static_cast<u_char*>(malloc(len));
        if (*bp == 0) {
            signal_error();
            return;
        }
    }

    if (flags & Serialize::NULL_TERMINATED) {
        len++;
    }

    ASSERT(*bp);
    if (lenp) {
        *lenp = len;
    }
    
    if (cur_ + len > buf_->len()) {
        signal_error();
        return;
    }

    if (flags & Serialize::NULL_TERMINATED) {
        memcpy(*bp, buf_->at(cur_), len);
        *buf_->at(cur_ + len) = '\0';
    } else {
        memcpy(*bp, buf_->at(cur_), len);
    }
    
    cur_ += len;
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, std::string* s) 
{
    if (error()) {
        return;
    }

    size_t len = process_int(8);
    if (error()) {
        return;
    }

    s->assign(buf_->at(cur_), len);
    cur_ += len;
}

//////////////////////////////////////////////////////////////////////////////
void 
KeyUnmarshal::process(const char* name, SerializableObject* object)
{
    if (error()) {
        return;
    }

    if (action(object) != 0) {
        signal_error();
    }
}

//////////////////////////////////////////////////////////////////////////////
u_int32_t
KeyUnmarshal::process_int(size_t size)
{
    char buf[9];

    if (cur_ + size > buf_->len()) {
        signal_error();
        return 0;
    }

    memset(buf, 0, 9);
    memcpy(buf, buf_->at(cur_), size);
    
    char* endptr;
    u_int32_t val = strtoul(buf_->at(cur_), &endptr, 16);
    
    if (endptr == buf_->at(cur_)) {
        signal_error();
        return 0;
    }

    cur_ += size;

    return val;
}

} // namespace oasys
