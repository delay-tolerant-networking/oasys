#ifndef __STREAMSERIALIZE_H__
#define __STREAMSERIALIZE_H__

namespace oasys {

class OutByteStream;

/*!
 * XXX/bowei - Ideally, we would like every serialization to move to
 * the stream format.
 */
class StreamSerialize : public SerializeAction {
public:
    StreamSerialize(OutByteStream* stream);

    //! @{ virtual from SerializeAction
    int action(SerializableObject* object);

    void begin_action();
    void end_action();

    void process(const char* name, u_int64_t* i);
    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, u_int32_t len);
    void process(const char* name, u_char** bp,
                 u_int32_t* lenp, int flags);
    void process(const char* name, std::string* s);
    //! @}
    
private:
    OutByteStream* stream_;
};

} // namespace oasys

#endif /* __STREAMSERIALIZE_H__ */
