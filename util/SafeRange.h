#ifndef __SAFERANGE_H__
#define __SAFERANGE_H__

namespace oasys {

/*!
 * Implements a SafeRange which throws an exception if the range
 * constraints are violated. This is probably the cleanest way to
 * write safe parsing code for strings.
 */
template<typename _Type>
class SafeRange {
public:
    /*!
     * Thrown by SafeRange if the range constraint is violated. This
     * maybe? should inherit from std::out_of_range.
     */
    struct Error {
        Error(size_t offset) : offset_(offset) {}
        size_t offset_;
    };

    SafeRange(_Type* a, size_t size)
        : array_(a), size_(size) {}
    
    _Type& operator[](size_t offset) {
        if (offset >= size_) {
            throw Error(offset);
        }
        
        return array_[offset];
    }

private:
    _Type* array_;
    size_t size_;
};

} // namespace oasys

#endif /* __SAFERANGE_H__ */
