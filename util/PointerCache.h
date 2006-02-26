#ifndef __POINTERCACHE_H__
#define __POINTERCACHE_H__

#include <set>

#include "../debug/DebugUtils.h"

namespace oasys {

/*!
 * A cache of pointers with a at_limit() predicate that is called when
 * with each new allocation to decide which pointer to
 * evict. Reinstate is called to revive evicted objects.
 */
template <
    typename _Name,            // namespace for the cache
    typename _PtrType          // type of pointer being stored
>
class PointerCache {
public:
    /*!
     * Adds the pointer to the cache.
     */
    PointerCache(_PtrType* ptr) : ptr_(0) {
        ASSERT(ptr != 0);
        // NOTE! because of the fact that virtual functions cannot be
        // called from the constructor, this line of code:
        //
        // this->set_ptr(ptr);
        // 
        // which really should be here needs to be put in the
        // constructor of the derived class.
    }
    
    /*!
     * Unregisters the contained pointer from the cache.
     */
    virtual ~PointerCache() {
        this->set_ptr(0);
    }

    //! @{ referencing operations will resurrect pointers if need be
    _PtrType& operator*() const {
        if (ptr_ == 0) {
            this->resurrect();
        }
        return *ptr_;
    }    
    _PtrType* operator->() const {
        if (ptr_ == 0) {
            this->resurrect();
        }
        return ptr_;
    }
    //! @}

    /*! 
     * Set the contained point to something else. Replaces the
     * contained pointer in the cache with the new pointer.
     */
    PointerCache& operator=(_PtrType* ptr) {
        this->set_ptr(ptr);
        return *this;
    }

    //! Not implemented to make this illegal
    PointerCache& operator=(const PointerCache&); 
    
    //! @return Original pointer
    _PtrType* ptr() { return ptr_; }
    
protected:
    typedef std::set<_PtrType*> PtrSet;

    PtrSet pointers_;

    virtual void resurrect()             = 0;
    virtual bool at_limit(_PtrType* ptr) = 0;
    virtual void evict()                 = 0;

    virtual void register_ptr(_PtrType* ptr);
    virtual void unregister_ptr(_PtrType* ptr);

    void set_ptr(_PtrType* ptr) {
        if (ptr == ptr_) {
            return;
        }

        if (ptr_ != 0) {
            this->unregister_ptr(ptr_);
            ptr_ = 0;
        }

        if (ptr != 0) {
            while (at_limit(ptr)) {
                this->evict();
            }
            this->register_ptr(ptr);
        }

        ptr_ = ptr;
    }    

private:
    _PtrType* ptr_;
};

//----------------------------------------------------------------------------
template <typename _Name, typename _PtrType>
void 
PointerCache<_Name, _PtrType>::register_ptr(_PtrType* ptr)
{
    typename PtrSet::const_iterator i;
    i = std::find(pointers_.begin(), pointers_.end(), ptr);

    ASSERT(i == pointers_.end());
    pointers_.insert(ptr);
}

//----------------------------------------------------------------------------
template <typename _Name, typename _PtrType>
void 
PointerCache<_Name, _PtrType>::unregister_ptr(_PtrType* ptr) 
{
    typename PtrSet::const_iterator i;
    i = std::find(pointers_.begin(), pointers_.end(), ptr);
    ASSERT(i != pointers_.end());

    pointers_.erase(i);
}

} // namespace oasys

#endif /* __POINTERCACHE_H__ */
