#include "StreamSerialize.h"

namespace oasys {

//------------------------------------------------------------------
StreamSerialize::StreamSerialize(OutByteStream* stream)
    : stream_(stream)
{}

//------------------------------------------------------------------
int 
StreamSerialize::action(SerializableObject* object)
{
}

//------------------------------------------------------------------
void 
StreamSerialize::begin_action()
{
    stream_->begin();
}

//------------------------------------------------------------------
void 
StreamSerialize::end_action()
{
    stream_->end();
}

//------------------------------------------------------------------
void 
StreamSerialize::process(const char* name, u_int64_t* i)
{
    if (error())
        return;

    u_char buf[8];

    buf[0] = ((*i)>>56) & 0xff;
    buf[1] = ((*i)>>48) & 0xff;
    buf[2] = ((*i)>>40) & 0xff;
    buf[3] = ((*i)>>32) & 0xff;
    buf[4] = ((*i)>>24) & 0xff;
    buf[5] = ((*i)>>16) & 0xff;
    buf[6] = ((*i)>>8)  & 0xff;
    buf[7] = (*i)       & 0xff;

    int err = stream_->write(buf, sizeof(buf));
    if (err != 0) 
    {
        signal_error();
    }
}

//------------------------------------------------------------------
void 
StreamSerialize::process(const char* name, u_int32_t* i)
{
    if (error())
        return;

    u_char buf[4];

    buf[0] = ((*i)>>24) & 0xff;
    buf[1] = ((*i)>>16) & 0xff;
    buf[2] = ((*i)>>8)  & 0xff;
    buf[3] = (*i)       & 0xff;

    int err = stream_->write(buf, sizeof(buf));
    if (err != 0) 
    {
        signal_error();
    }
}

//------------------------------------------------------------------
void 
StreamSerialize::process(const char* name, u_int16_t* i)
{
    if (error())
        return;

    u_char buf[2];

    buf[0] = ((*i)>>8) & 0xff;
    buf[1] = (*i)      & 0xff;

    int err = stream_->write(buf, sizeof(buf));
    if (err != 0) 
    {
        signal_error();
    }
}

//------------------------------------------------------------------
void 
StreamSerialize::process(const char* name, u_int8_t* i)
{
    if (error())
        return;

    buf[0] = (*i);
    
    int err = stream_->write(buf, sizeof(buf));
    if (err != 0) 
    {
        signal_error();
    }
}

//------------------------------------------------------------------
void 
StreamSerialize::process(const char* name, bool* b)
{
    if (error())
        return;

    buf[0] = (*b) ? 1 : 0;

    int err = stream_->write(buf, sizeof(buf));
    if (err != 0) 
    {
        signal_error();
    }
}

//------------------------------------------------------------------
void 
StreamSerialize::process(const char* name, u_char* bp, u_int32_t len)
{
    if (error())
        return;
    
    int err = stream_->write(bp, len);
    if (err != 0) 
    {
        signal_error();
    }
}

//------------------------------------------------------------------
void 
StreamSerialize::process(const char* name, u_char** bp,
             u_int32_t* lenp, int flags);
{
    if (error())
        return;
    
    u_int32_t str_len;

    if (flags & Serialize::NULL_TERMINATED) 
    {
        str_len = strlen(reinterpret_cast<char*>(*bp)) + 1;
    } 
    else 
    {
        std::string len_name = name;
        len_name += ".len";
        process(len_name.c_str(), lenp);
        str_len = *lenp;
        if (error())
            return;
    }

    if (*lenp != 0) 
    {
        stream_->write(*bp, str_len);
    }
}

//------------------------------------------------------------------
void 
StreamSerialize::process(const char* name, std::string* s)
{
    if (error())
        return;

    u_int32_t len = s->length();
    std::string len_name = name;
    len_name += ".len";
    process(len_name.c_str(), &len);
    if (error())
        return;

    int err = stream_->write(s->data(), len);
    if (err != 0) 
    {
        signal_error();
    }
}

} // namespace oasys
