#include "errno.h"
#include "lib/IO.h"
#include "BufferedIO.h"

/******************************************************************
 *
 * BufferedInput
 *
 ******************************************************************/
#define DEFAULT_BUFSIZE 1024

BufferedInput::BufferedInput(IOClient* client, const char* logbase)
    : Logger(logbase), client_(client), buf_(DEFAULT_BUFSIZE), 
      seen_eof(false)
 {}

BufferedInput::~BufferedInput()
{}

int 
BufferedInput::read_line(const char* nl, char** buf, int timeout)
{
    int endl;
    while((endl = find_nl(nl)) == -1)
    {
        int cc = read(0, timeout);
        
	logf(LOG_DEBUG, "readline: cc = %d", cc);
        if(cc <= 0)
        {
            logf(LOG_DEBUG, "%s: read %s", 
                 __func__, (cc == 0) ? "eof" : strerror(errno));
            return (cc < 0) ? -1 : 0;
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
    int total = 0;
    
    while(buf_.fullbytes() < len)
    {
	int cc = read(len - buf_.fullbytes(), timeout);
	
	if(cc == 0)
	{
	    break;
	}
	else if(cc < 0)
	{
	    return cc;
	}

	total += cc;
    }

    *buf = buf_.start();
    buf_.consume(total);
    
    return total;
}

char
BufferedInput::getc(int timeout)
{
    if(buf_.fullbytes() == 0) 
    {
        int cc = read(0, timeout);

        if(cc <= 0) 
        {
            logf(LOG_ERR, "%s: read %s", 
                 __func__, (cc == 0) ? "eof" : strerror(errno));
            
            return 0;
        }
    }

    char ret = *buf_.start();
    buf_.consume(1);

    return ret;
}

bool
BufferedInput::eof()
{
    return buf_.fullbytes() == 0 && seen_eof;
}

int
BufferedInput::read(size_t len, int timeout_ms)
{
    size_t toread;

    logf(LOG_DEBUG, "read(%u, %d)", len, timeout_ms);

    if(len == 0) 
    {
        toread = (buf_.emptybytes() == 0) ? BufferedInput::READ_AHEAD : 
            buf_.emptybytes();
    } 
    else 
    {
        toread = len - buf_.fullbytes();
    }
    buf_.reserve(toread);
    
    int cc = 0;
    if(toread > 0)
    {
        cc = client_->read(buf_.end(), toread); // XXX/bowei timeout?
        if(cc < 0)
        {
            logf(LOG_ERR, "fill(): error in read: %s", strerror(errno));
        
            return -1;
        }
        else if(cc == 0) 
        {
            seen_eof = true;
        }
    }

    logf(LOG_DEBUG, "cc = %d", cc);
    buf_.fill(cc);

    if(len == 0)
    {
	return buf_.fullbytes();
    }
    else
    {
	return (buf_.fullbytes() < len) ? buf_.fullbytes() : len;
    }
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

        if(offset == 0 || static_cast<int>(bytes_left) < nl_len)
            return -1;
        
        if(memcmp(offset, nl, nl_len) == 0)
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
BufferedOutput::write(const char* bp, int len) 
{
    buf_.reserve(len);
    memcpy(buf_.end(), bp, len);
    buf_.fill(len);
    
    if(buf_.fullbytes() > flush_limit_)
    {
	flush();
    }

    return len;
}

int 
BufferedOutput::writef(const char* bp, int len)
{
    int ret = write(bp, len);
    flush();

    return ret;
}

int
BufferedOutput::vformat_buf(const char* fmt, va_list ap)
{
    int len = vsnprintf(buf_.end(), 0, fmt, ap);
    buf_.reserve(len);

    len = vsnprintf(buf_.end(), len, fmt, ap);

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
