#ifndef tier_jenkins_hash_h
#define tier_jenkins_hash_h

#include <sys/types.h>

/*
 * Just one thing here, the jenkins hash function.
 */
extern u_int32_t
jenkins_hash(u_int8_t *k,        /* the key */
             u_int32_t length,   /* the length of the key */
             u_int32_t initval); /* the previous hash, or an arbitrary value */

#endif
