#include "TextCode.h"

namespace tier {

TextCode::TextCode(const char* input_buf, size_t length, 
                   ExpandableBuffer* buf, int cols, char* padding)
    : input_buf_(input_buf), length_(length), 
      buf_(buf), cols_(cols), padding_(padding)
{}

bool
TextCode::is_not_escaped(int c) {
    return c >= 32 && c <= 126 && c != '\\';
}

void 
TextCode::append(int c) {
    if (is_not_escaped(c)) {
        buf_->append(static_cast<char>(c));
    } else if (c == '\\') {
        buf_->appendf("\\\\");
    } else {
        buf_->appendf("\\%02x", c);
    }
}

void
TextCode::textcodify()
{
    for (size_t i=0; i<length_; ++i) 
    {
        if (i != 0 && (i % cols_ == 0)) {
            buf_->appendf("\n%s", padding_);
        }
        buf_->append(input_buf[i]);
    }
    buf_->appendf("\n%s\n", padding_);
}

} // namespace tier
