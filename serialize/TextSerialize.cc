#include "TextSerialize.h"

namespace oasys {

//----------------------------------------------------------------------------
TextMarshalSize(context_t context, int options, int indent_incr)
    SerializeAction(Serialize::INFO, context, options), 
    indent_incr_(indent_incr)
{}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, u_int32_t* i)
{
    
}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, u_int16_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, u_int8_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, bool* b)
{
}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, u_char* bp, size_t len)
{
}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, u_char** bp, 
                         size_t* lenp, int flags)
{
}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, std::string* s)
{
}

//----------------------------------------------------------------------------
TextMarshal::TextMarshal(context_t context, u_char* buf, 
                         size_t length, int options)
    : SerializeAction(Serialize::MARSHAL, context, options)
{}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, u_int32_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, u_int16_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, u_int8_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, bool* b)
{
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, u_char* bp, size_t len)
{
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, u_char** bp, size_t* lenp, int flags)
{
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, std::string* s)
{
}

//----------------------------------------------------------------------------
TextUnmarshal::TextUnmarshal(context_t context, u_char* buf, 
                             size_t length, int options)
    : SerializeAction(Serialize::UNMARSHAL, context, options);
{}
 
//----------------------------------------------------------------------------   
void 
TextUnmarshal::process(const char* name, u_int32_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, u_int16_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, u_int8_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, bool* b)
{
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, u_char* bp, size_t len)
{
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, u_char** bp, 
                       size_t* lenp, int flags)
{
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, std::string* s);
 

} // namespace oasys
