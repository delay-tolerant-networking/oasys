#ifndef __BUFFERED_IO_H__
#define __BUFFERED_IO_H__

#include "lib/Logger.h"
#include "lib/IOClient.h"
#include "lib/StreamBuffer.h"

/**
 * Wrapper class for a tcp client socket that includes an in-memory
 * buffer for reading and/or writing. Note that the same buffer is
 * used for both.
 */
class BufferedInput : public Logger {
public:
    BufferedInput(IOClient* client, const char* logbase = "/bufferedInput");
    ~BufferedInput();
    
    /*! 
     * Read in a line of input, newline characters included. 
     * 
     * \param buf buf is valid until next function call to
     * BufferedInput.
     * \return Length of line, including nl characters. <0 on error, 0
     * on eof.
     */
    int read_line(const char* nl, char** buf, int timeout = -1);

    /*!
     * Read len bytes. Blocking until specified amount of bytes is
     * read.
     *
     * \param buf buf is valid until next fcn call to BufferedInput
     * \return Length of segment read. <0 upon error, 0 on eof. Return 
     * will only be < len if eof is reached before fully read len
     * bytes.
     */
    int read_bytes(size_t len, char** buf, int timeout = -1);

    /*!
     * Read in a single character from the protocol stream. Returns 0
     * if at the end of the stream or error.
     */
    char getc(int timeout = -1);

    /*!
     * Returns true if at the end of file.
     */
    bool eof();

private:    
    /*!
     * Read in len bytes into the buffer. If there are enough bytes
     * already present in buf_, no call to read will occur.
     *
     * \param len If len = 0, then read() will try to fill buf_.
     * \param timeout_ms Timeout to the read call. UNIMPLEMENTED
     * \return Bytes available, can be less than len.
     */
    int internal_read(size_t len = 0, int timeout_ms = -1);

    /*!
     * \return Index of the start of the sequence of the newline
     * character string
     */
    int find_nl(const char* nl);
    
    IOClient*    client_;
    StreamBuffer buf_;

    bool seen_eof_;

    static const size_t READ_AHEAD = 256;  //! Amount to read when buffer is full
    static const size_t MAX_LINE   = 4096; //! Maximum line length
};

class BufferedOutput : public Logger {
public:
    BufferedOutput(IOClient* client, const char* logbase = "/bufferedOutput");

    /*!
     * Write len bytes from bp. Output may be buffered.
     *
     * \return the number of bytes successfully written.
     */
    int write(const char* bp, int len);

    //! Write that always flushes
    int writef(const char* bp, int len);

    /*!
     * Fills the buffer via printf style args, returning the length or
     * -1 if there's an error.
     */
    int format_buf(const char* format, ...) PRINTFLIKE(2, 3);
    int vformat_buf(const char* format, va_list args);
        
    /*!
     * Writes the full buffer, potentially in multiple calls to write.
     * \return <0 on error, otherwise the number of bytes written
     */
    int flush();
    
    /*!
     * If the buffer reaches size > limit, then the buffer is automatically
     * flushed
     */
    void set_flush_limit(size_t limit);
    
    /*!  
     * Do format_buf() and flush() in one call.
     */
    int printf(const char* format, ...) PRINTFLIKE(2, 3);

private:
    Logger       log_;
    IOClient*    client_;
    StreamBuffer buf_;

    size_t flush_limit_;

    static const size_t DEFAULT_FLUSH_LIMIT = 256;
};

#endif //__BUFFERED_IO_H__
