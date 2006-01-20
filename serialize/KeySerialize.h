#ifndef __KEYSERIALIZE_H__
#define __KEYSERIALIZE_H__

#include "Serialize.h"
#include "../util/ExpandableBuffer.h"

namespace oasys {

/**
 * Marshaller for key-type objects. The serialized format is filename
 * and null character friendly. (Note: This marshaller will not check
 * to see if stored strings conform, so users of this class will have
 * to do that check themselves).
 *
 * - Numbers are encoded in hex with a fixed length of digits, zero padded.
 * - Booleans are '0' or '1'
 * - Fixed length strings are stored directly
 * - Variable length strings are stored with the length in hex
 *   followed by the string.
 * - Each field is terminated by the border str
 */
class KeyMarshal : public SerializeAction {
public:
    KeyMarshal(ExpandableBuffer* buf,
               const char*       border = 0);
    
    //! @{ Virtual functions inherited from SerializeAction
    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, int flags);
    void process(const char* name, std::string* s);
    void process(const char* name, SerializableObject* object);
    //! @}

    //! Const object handler
    int action(const SerializableObject* object) {
        return SerializeAction::action(
            const_cast<SerializableObject*>(object));
    }

    
private:
    ExpandableBuffer* buf_;
    const char*       border_;

    void process_int(u_int32_t i, size_t size, const char* format);
    void border();
};

/**
 * Unmarshaller for keys.
 */
class KeyUnmarshal : public SerializeAction {
public:
    KeyUnmarshal(const char* buf,
                 size_t      buf_len,
                 const char* border = 0);
    
    //! @{ Virtual functions inherited from SerializeAction
    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, int flags);
    void process(const char* name, std::string* s);
    void process(const char* name, SerializableObject* object);

    //! @}
private:
    const char* buf_;
    size_t      buf_len_;
    size_t      border_len_;
    size_t      cur_;

    u_int32_t process_int(size_t size);
    void border();
};

} // namespace oasys

#endif /* __KEYSERIALIZE_H__ */
