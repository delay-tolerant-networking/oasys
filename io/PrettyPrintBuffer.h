#ifndef __PRETTY_PRINT_BUFFER_H__
#define __PRETTY_PRINT_BUFFER_H__

#include <string>
#include <utility>              // for std::pair

/*!
 * Class for generating pretty printed text. Somewhat inefficient.
 */
class PrettyPrintBuf {
public:
    PrettyPrintBuf(const char* buf, int len = -1);
 
    std::pair< std::string, bool > next_str();

private:
    static const int MAX_COL;

    const char* buf_;
    int cur_;
    int len_;
};

#endif //__PRETTY_PRINT_BUFFER_H__
