
#include <oasys/util/StringBuffer.h>

#include "PrettyPrintBuffer.h"

const int PrettyPrintBuf::MAX_COL = 40;

//////////////////////////////////////////////////////////////////////////////
PrettyPrintBuf::PrettyPrintBuf(const char* buf, int len)
    : buf_(buf), cur_(0), len_(len)
{
    if(len_ == -1)
    {
        len_ = strlen(buf);
    }
}

//////////////////////////////////////////////////////////////////////////////
std::pair<std::string, bool>
PrettyPrintBuf::next_str()
{
    StringBuffer buf;

    int bound = std::min(cur_ + MAX_COL, len_);
    for(int i = cur_; i<bound; ++i, ++cur_)
    {
        switch(buf_[i])
        { 
        case '\n': buf.append("\\n"); break;
        case '\r': buf.append("\\r"); break;
        case '\t': buf.append("\\t"); break;
        case '\0': buf.append("\\0"); break;

        default:
            buf.append(buf_[i]);
        }
    }
    
    if(bound == len_)
    {
        return std::pair<std::string, bool>(buf.c_str(), false);
    }
    else
    {
        return std::pair<std::string, bool>(buf.c_str(), true);
    }
}
