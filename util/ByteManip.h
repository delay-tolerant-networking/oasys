#ifndef __BYTE_MANIP_H__
#define __BYTE_MANIP_H__

/**
 * Convert 4 bytes in to an int.
 */
#define 4CHAR2INT(_1, _2, _3, _4) \
   (((_1)&0xff)<<24) | (((_1)&0xff)<<16) | (((_1)&0xff)<<8) | ((_1)&0xff)

#endif
