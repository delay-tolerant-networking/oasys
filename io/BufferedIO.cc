#include "errno.h"
#include "lib/IO.h"
#include "lib/Utils.h"
#include "BufferedIO.h"

/******************************************************************
 *
 * BufferedInput
 *
 ******************************************************************/
#define DEFAULT_BUFSIZE 1024

BufferedInput::BufferedInput(IOClient* client, const char* logbase)
    : Logger(logbase), client_(client), buf_(DEFAULT_BUFSIZE), 
      seen_eof_(false)
 {}

BufferedInput::~BufferedInput()
{}

int 
BufferedInput::read_line(const char* nl, char** buf, int timeout)
{
    int endl;
    while((endl = find_nl(nl)) == -1)
    {
        // can't find a newline, so read in another chunk of data
        int cc = internal_read(buf_.fullbytes() + BufferedInput::READ_AHEAD,
                               timeout);
        
	logf(LOG_DEBUG, "readline: cc = %d", cc);
        if(cc <= 0)
        {
            logf(LOG_DEBUG, "%s: read %s", 
                 __func__, (cc == 0) ? "eof" : strerror(errno));
            return cc;
        }
    }

    *buf = buf_.start();

    logf(LOG_DEBUG, "endl = %d", endl);
    buf_.consume(endl + strlen(nl));

    return endl + strlen(nl);
}

int 
BufferedInput::read_bytes(size_t len, char** buf, int timeout)
{
    ASSERT(len > 0);
    
    logf(LOG_DEBUG, "read_bytes %d (timeout %d)", len, timeout);
    
    size_t total = buf_.fullbytes();
    
    while (total < len)
    {
        // fill up the buffer (if possible)
        logf(LOG_DEBUG, "read_bytes calling internal_read for %d needed bytes",
             len - total);
	int cc = internal_read(len, timeout);
        if(cc <= 0)
        {
            logf(LOG_DEBUG, "%s: read %s", 
                 __func__, (cc == 0) ? "eof" : strerror(errno));
            return cc;
        }
        
	total += cc;
    }

    *buf = buf_.start();

    // don't consume more than the user asked for
    buf_.consume(len);
    
    return len;
}

int 
BufferedInput::read_some_bytes(char** buf, int timeout)
{
    int cc;

    // if there's nothing in the buffer, then issue one call to read,
    // trying to fill up as much as possible
    if (buf_.fullbytes() == 0) {
        ASSERT(buf_.start() == buf_.end());
        
        cc = internal_read(buf_.tailbytes(), timeout);

        if (cc == 0) {
            logf(LOG_DEBUG, "%s: read eof", __func__);
            return cc; // eof ok
        }

        if (cc < 0) {
            logf(LOG_ERR, "%s: read error %s", __func__, strerror(errno));
            return cc;
        }

        ASSERT(buf_.fullbytes() > 0);
    }

    *buf = buf_.start();
    
    cc = buf_.fullbytes();
    buf_.consume(cc);
    
    logf(LOG_DEBUG, "read_some_bytes ret %d (timeout %d)", cc, timeout);

    return cc;
}

char
BufferedInput::get_char(int timeout)
{
    if (buf_.fullbytes() == 0) 
    {
        int cc = internal_read(buf_.tailbytes(), timeout);
        
        if (cc <= 0) {
            logf(LOG_ERR, "%s: read %s", 
                 __func__, (cc == 0) ? "eof" : strerror(errno));
            
            return 0;
        }

        ASSERT(buf_.fullbytes() > 0);
    }

    char ret = *buf_.start();
    buf_.consume(1);

    return ret;
}

bool
BufferedInput::eof()
{
    return buf_.fullbytes() == 0 && seen_eof_;
}

int
BufferedInput::internal_read(size_t len, int timeout_ms)
{
    int cc;
    ASSERT(len > 0);
    ASSERT(len > buf_.fullbytes());

    // make sure there's at least len space in buf's tailbytes
    buf_.reserve(len);

    // but always try to fill up as much as possible into tailbytes
    if (timeout_ms > 0) {
        cc = client_->timeout_read(buf_.end(), buf_.tailbytes(), timeout_ms);
    } else {
        cc = client_->read(buf_.end(), buf_.tailbytes());
    }
    
    if (cc == IOTIMEOUT)
    {
        logf(LOG_DEBUG, "internal_read %d (timeout %d) timed out",
             len, timeout_ms);
        return cc;
    }
    else if (cc == IOERROR)
    {
        logf(LOG_ERR, "internal_read %d (timeout %d) error in read: %s",
             len, timeout_ms, strerror(errno));
        
        return cc;
    }
    else if (cc == 0) 
    {
        logf(LOG_DEBUG, "internal_read %d (timeout %d) eof",
             len, timeout_ms);
        seen_eof_ = true;
        return cc;
    }
    
    buf_.fill(cc);

    int ret;
    ret = MIN(buf_.fullbytes(), len);
    
    logf(LOG_DEBUG, "internal_read %d (timeout %d): cc=%d ret %d",
         len, timeout_ms, cc, ret);

    return ret;
}

int
BufferedInput::find_nl(const char* nl)
{
    char* offset = buf_.start();
    int nl_len   = strlen(nl);
    size_t bytes_left = buf_.fullbytes();

    for(;;)
    {
        char* new_offset;
        new_offset = static_cast<char*>(memchr(offset, nl[0], bytes_left));
        
        bytes_left -= new_offset - offset;
        offset = new_offset;

        if (offset == 0 || static_cast<int>(bytes_left) < nl_len)
            return -1;
        
        if (memcmp(offset, nl, nl_len) == 0)
        {
            return offset - buf_.start();
        }
	
	offset++;
	bytes_left--;
    }
}

/***************************************************************************
 *
 * BufferedOuput
 *
 **************************************************************************/
BufferedOutput::BufferedOutput(IOClient* client, 
                               const char* logbase)
    : log_(logbase), client_(client), buf_(DEFAULT_BUFSIZE), 
      flush_limit_(DEFAULT_FLUSH_LIMIT)
{}

int
BufferedOutput::write(const char* bp, size_t len)
{
    if (len == 0)
        len = strlen(bp);
              
    buf_.reserve(len);
    memcpy(buf_.end(), bp, len);
    buf_.fill(len);
    
    if ((flush_limit_) > 0 && (buf_.fullbytes() > flush_limit_))
    {
	flush();
    }

    return len;
}

void
BufferedOutput::clear_buf()
{
    buf_.clear();
}

int
BufferedOutput::vformat_buf(const char* fmt, va_list ap)
{
    int nfree = buf_.tailbytes();
    int len = vsnprintf(buf_.end(), nfree, fmt, ap);

    if (len >= nfree) {
        buf_.reserve(len);
        nfree = len;
        len = vsnprintf(buf_.end(), nfree, fmt, ap);
        ASSERT(len <= nfree);
    }

    buf_.fill(len);
    
    if ((flush_limit_) > 0 && (buf_.fullbytes() > flush_limit_))
    {
	flush();
    }

    return len;
}

int
BufferedOutput::format_buf(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = vformat_buf(format, ap);
    va_end(ap);

    return ret;
}

int
BufferedOutput::printf(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = vformat_buf(format, ap);
    va_end(ap);

    flush();
    
    return ret;
}

int
BufferedOutput::flush()
{
    int total = 0;

    while(buf_.fullbytes() > 0)
    {
        int cc = client_->write(buf_.start(), buf_.fullbytes());

        if (cc < 0) 
	{
            logf(LOG_ERR, "write error %s", strerror(errno));

            return cc;
        }
        logf(LOG_DEBUG, "flush wrote %d bytes", cc);

	buf_.consume(cc);
	total += cc;
    }

    return total;
}

void
BufferedOutput::set_flush_limit(size_t limit)
{
    flush_limit_ = limit;
}
