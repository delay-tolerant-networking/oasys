#ifndef __STREAM_BUFFER_H__
#define __STREAM_BUFFER_H__

#include <stdlib.h>

/**
 * @brief Stream oriented resizable buffer.
 *
 * StreamBuffer is a resizable buffer which is designed to efficiently
 * support growing at the end of the buffer and consumption of the
 * buffer from the front.
 */
class StreamBuffer {
public:
    /**
     * Create a new StreamBuffer with initial size
     */
    StreamBuffer(size_t size = DEFAULT_BUFSIZE);
    ~StreamBuffer();

    /**
     * Set the size of the buffer. New size should not be smaller than
     * size of data in the StreamBuffer.
     */
    void set_size(size_t size);

    /** @return Pointer to the beginning of the data. */
    char* start();
    
    /** @return Pointer to the end of the data. */
    char* end();
    
    /** Reserve amount bytes in the buffer */
    void reserve(size_t amount);

    /** Fill amount bytes, e.g. move the end ptr up by that amount */
    void fill(size_t amount);
    
    /** Consume amount bytes from the front of the buffer */
    void consume(size_t amount);

    /** Clear all data from the buffer */
    void clear();
    
    /** Amount of bytes stored in the stream buffer */
    size_t fullbytes();

    /** Amount of free bytes available at the end of the buffer */
    size_t tailbytes();

private:
    /** Resize buffer to size. */
    void realloc(size_t size);

    /** Compact buffer, moving all data up to the start of the
     * dynamically allocated buffer, eliminating the empty space at
     * the front. */
    void moveup();

    size_t start_;
    size_t end_;
    size_t size_;

    char* buf_;

    static const size_t DEFAULT_BUFSIZE = 512;
};

#endif //__STREAM_BUFFER_H__
