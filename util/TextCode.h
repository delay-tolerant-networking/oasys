#ifndef __TEXTCODE_H__
#define __TEXTCODE_H__

namespace oasys {

/*! 
 * Outputs a string that a certain column length with all non-printable
 * ascii characters rewritten in hex.
 * 
 * A TextCode block ends with a single raw control-L character
 * followed by a newline character ("\n") on a single line.
 */
class TextCode {
public:
    /*! 
     * @param input_buf Input buffer
     * @param length Length of the input buffer
     * @param buf  Buffer to put the text coded block into.
     * @param cols Number of characters to put in a column.
     * @param pad  String to put in front of each line.
     */
    TextCode(const char* input_buf, size_t length, 
             ExpandableBuffer* buf, int cols, const char* pad);
    
private:
    const char*       input_buf_;
    size_t            length_;        
    ExpandableBuffer* buf_;
    int               cols_;
    const char*       padding_;

    //! Whether or not the character is printable ascii
    bool is_printable(int c);

    //! Perform the conversion
    void textcodify();    
};

} // namespace oasys

#endif /* __TEXTCODE_H__ */
