#ifndef __DURABLESTOREKEY_H__
#define __DURABLESTOREKEY_H__

#include <string>

#include "../serialize/Serialize.h"

namespace oasys {

/**
 * SerializableKey - Used by the keys to the store to avoid
 * unnecessary serialization.
 */
class DurableStoreKey : public SerializableObject {
public:
    /**
     * Compare function used by the tables for finding the key.
     */
    virtual int compare(const DurableStoreKey& other) = 0;

    /**
     * @return Key expressed as an ascii string. This is easier than a
     * generalized marshalling/unmarshalling which is quite a heavy
     * weight process.
     */
    virtual std::string as_ascii() = 0;

    /**
     * @return True if a key could be converted from the ascii.
     */
    virtual bool from_ascii(const std::string& ascii) = 0;
};

} // namespace oasys

#endif /* __DURABLESTOREKEY_H__ */
