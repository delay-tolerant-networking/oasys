#ifndef __RANDOM_H__
#define __RANDOM_H__

#include <stdlib.h>
#include <vector>

namespace oasys {

/**
 * Given a seed, generate some number n such that 1 <= n <= seed
 */
class Random {
public:
    /**
     * Seed the random number generator(s).
     */
    static void seed(unsigned int seed) {
        srandom(seed);
        srand(seed);
    }

    /**
     * Returns a random integer in the range [0..max) -- this could
     * probably be enhanced at some future time with a better random
     * number generator
     */
    static int rand(unsigned int max = RAND_MAX) {
        int ret = ::rand();
        if (max == RAND_MAX) {
            return ret;
        }
        return (ret % max);
    }
};

/**
 * Generates a some what random stream of bytes given a seed. Useful
 * for making random data. The randomizer uses a linear congruential
 * generator I_k = (a * I_{k-1} + c ) mod m with a = 3877, c = 29574,
 * m = 139968. Should probably try find better numbers here.
 */
class ByteGenerator {
public:
    ByteGenerator(unsigned int seed = 0);

    /**
     * Fill a buffer with size random bytes.
     */
    void fill_bytes(void* buf, size_t size);

    static const unsigned int A = 1277;
    static const unsigned int M = 131072;
    static const unsigned int C = 29574;

private:
    unsigned int cur_;
    
    /// Calculate next random number
    void next();
};

/**
 * Generates a random permuation of length n stored in an array
 * XXX/bowei - add seed
 */
class PermutationArray {
public:
    PermutationArray(size_t size);
    
    unsigned int map(unsigned int i);

private:
    std::vector<unsigned int> array_;
    size_t size_;
};

}; // namespace oasys

#endif //__RANDOM_H__
