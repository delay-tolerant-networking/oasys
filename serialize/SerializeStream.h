#ifndef __SERIALIZESTREAM_H__
#define __SERIALIZESTREAM_H__

#include "../io/IOClient.h"

namespace oasys {

/*!
 * Stream is the consumer/producer of bits which the Serialization
 * adaptors use to reconstitute objects.
 */
class SerializeStream {
public:
    //! Allow const buffers to be passed in
    int process_bits(const char* buf, size_t size) {
        return process_bits(const_cast<char*>(buf), size);
    }

    /*!
     * Process size bits into/out of the stream.
     *
     * @return Bytes processed, less than 0 on error.
     */
    virtual int process_bits(char* buf, size_t size) = 0;
};

/*!
 * A stream which wraps writing byte array.
 */
template<typename _Copy>
class MemoryStream : public SerializeStream {
public:
    MemoryStream(const char* buf, size_t size)
        : buf_(const_cast<char*>(buf)), 
          size_(size), pos_(0) {}

    //! virtual from SerializeStream
    int process_bits(char* buf, size_t size) {
        if (pos_ + size > size_) {
            return -1;
        }
        _Copy cp;
        cp(buf, buf_ + pos_, size);            
        pos_ += size;

        return size;
    }
    
private:
    char*  buf_;
    size_t size_;
    size_t pos_;                //!< Current position in the stream
};

template<typename _IO_Op>
class IOStream : public SerializeStream {
public:
    IOStream(IOClient* io) : io_(io) {}
    
    //! virtual from SerializeStream
    int process_bits(char* buf, size_t size) {
        return _IOOp(io_, buf, size);
    }

private:
    IOClient* io_;
};

namespace StreamOps {

struct CopyTo {
    void* operator()(void* dest, void* src, size_t n) {
        return memcpy(dest, src, n);
    }
};
struct CopyFrom {
    void* operator()(void* src, void* dest, size_t n) {
        return memcpy(dest, src, n);
    }
};

struct Read {
    int operator()(IOClient* io, char* buf, size_t size) {
        return io->readall(buf, size);
    }
};

struct Write {
    int operator()(IOClient* io, char* buf, size_t size) {
        return io->readall(buf, size);
    }
};

} // namespace StreamOps

typedef MemoryStream<StreamOps::CopyTo>   ReadMemoryStream;
typedef MemoryStream<StreamOps::CopyFrom> WriteMemoryStream;
typedef IOStream<StreamOps::Read>         ReadIOStream;
typedef IOStream<StreamOps::Write>        WriteIOStream;

} // namespace oasys

#endif /* __SERIALIZESTREAM_H__ */
