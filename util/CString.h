#ifndef __CSTRING_H__
#define __CSTRING_H__

namespace oasys {

//
// Byte oriented functions
// 

/*!
 * Copy src to dest. Copies at most dest_size - 1 characters and NULL
 * terminates the result in dest. Returns the number of characters
 * copied. If src or dest is null, then nothing is done. src is
 * assumed to be null terminated.
 *
 * NB. strncpy behavior is fairly broken if you think about it.
 */
int cstring_copy(char* dest, size_t dest_size, const char* src)
{
    if (dest == 0 || src == 0) 
    {
        return 0;
    }

    int cc = 0;
    while (dest_size > 1 && *src != '\0') 
    {
        *dest = *src;
        ++dest;
        ++src;
        --dest_size;
        ++cc;
    }

    *dest = '\0';

    return cc;
}

//
// Wide-Character oriented functions (XXX/bowei - TODO: support for
// wide chars)
// 

};

#endif /* __CSTRING_H__ */
