#ifndef __TEXTSERIALIZE_H__
#define __TEXTSERIALIZE_H__

#include "serialize/Serialize.h"
#include "serialize/BufferedSerializeAction.h"

namespace oasys {

/*!
 *
 * Text marshal format:
 *
 * # Comments
 * {\t\ }*fieldname: value\n
 *   # additional fields...
 * .  # single period
 * 
 */
class TextMarshal : public BufferedSerializeAction {
public:
    TextMarshal(context_t context, u_char* buf, 
                size_t length, 
                int options         = 0, 
                const char* comment = 0, 
                int indent_incr     = 4);

    //! @{ Virtual functions inherited from SerializeAction
    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, int flags);
    void process(const char* name, std::string* s);
    //! @}

private:
    void indent()   { indent_ += indent_incr_; }
    void unindent() { 
        indent_ -= indent_incr_; 
        ASSERT(indent_ >= 0); 
    }

    int indent_;
    int indent_incr_;
};

class TextUnmarshal {
public:
    TextMarshal(context_t context, u_char* buf, 
                size_t length, int options = 0);
    
    //! @{ Virtual functions inherited from BufferedSerializeAction
    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, int flags);
    void process(const char* name, std::string* s);
    //! @}
private:
};

} // namespace oasys

#endif /* __TEXTSERIALIZE_H__ */
