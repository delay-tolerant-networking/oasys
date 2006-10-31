#ifndef __SPARSE_RANGE_H__
#define __SPARSE_RANGE_H__

/*!  
 * SparseArray defines a logical array of _Type which is expected to
 * be sparse.  SparseArray can be very smart, but right now is dumb
 * but gets things right.
 * 
 */
template<typename _Type> class SparseArray { 
public: 
    class Block {
    public:
	size_t offset_;
	size_t size_;
        _Type* data_;

        Block(size_t offset, size_t size)
            : offset_(offset), size_(size), data_(0)
        {
            data_ = calloc(size, sizeof(_Type));
        }
        
	struct BlockCompare {
	    bool operator()(const Block& a, const Block& b) {
		return a.offset_ < b.offset_;
	    }
	};
    };
    typedef std::list<Block> BlockList;

    /*!  
     * In the empty sparse area, the SparseArray returns an element
     * which is the default constructor of _Type.
     */
    _Type operator[](size_t offset) const
    {}
    
    /*!
     * Copy out of the SparseRange a range of bytes. This is more
     * efficient than looping through with operator[].
     */
    void range_copy(_Type* out, size_t offset, size_t elts) const
    {}

    /*!
     * Write into the sparse map. Allocates a block if needed.
     */
    void range_write(size_t offset, size_t elts, const _Type* in)
    {
        allocate(offset, elts);
        memcpy();
    }

    /*!
     * For debugging only!
     */
    BlockList::const_iterator debug_iterator() const 
    { 
        return blocks_.begin(); 
    }
    
private:
    //! List of blocks_ sorted in ascending order by offset
    BlockList blocks_;

    /*!
     * Allocate a block starting at offset with elts. Will merge
     * blocks that it finds overlapping.
     *
     * @return Pointer to allocated region.
     */
    _Type* allocate(size_t offset, size_t elts)
    {
        bool merge = false;
        
        //
        // Take care of any existing blocks which overlap the new
        // block like this:
        //
        //     |---- new block ----| 
        // |---- old block ----|
        //
	BlockList::iterator itr = blocks_.begin();
	while (*itr != blocks_.end())
	{
            // block occurs before
            if (itr->offset_ + itr->size_ < offset)
            {
                ++itr;
                continue;
            }              

            // new block is completely contained in an existing block
            if (itr->offset_ <= offset && 
                (itr->offset_ + itr->size_ >= offset + size))
            {
                return &itr->data_[offset - itr->offset_];
            }

            // current block overlaps part of the new block
            if (itr->offset_ <= offset && 
                (itr->offset_ + itr->size_ > offset) &&
                (itr->offset_ + itr->size_ < offset + size))
            {
                merge = true;
                break;
            }

            // we stepped over to past the block
            if (itr->offset_ > offset)
            {
                break;
            }

            NOTREACHED;
	}

        if (merge) 
        {
            ASSERT(itr != blocks_.end());

            size_t new_size = offset + size - itr->offset;
            itr->data_ = realloc(itr->data_, sizeof(_Type) * new_size);
            itr->size_ = new_size;
            new_block = *itr;
        }
        else
        {
            ASSERT(itr->offset_ > offset);
            new_block = Block(offset, size);
            blocks_.insert(itr, new_block);
            --itr;
            ASSERT(itr != blocks_.begin());
        }
        
        BlockList::iterator tail = itr;
        ++tail;
        
        //
        // fixup the tail, which includes any blocks like this:
        //
        // |---- new block ----|
        //         |---- old block ----|
        //
        while (tail != blocks_.end())
        {
            // stepped past the end of the new_block
            if (tail->offset_ > itr->offset_ + itr->size_) 
            {
                break;
            }
            
            if (tail->offset_ <= itr->offset_ + itr->size_)
            {
                size_t new_size = tail->offset_ + tail->size_ - itr->offset_;
                itr->data_ = realloc(itr->data_, new_size);
                itr->size_ = new_size;
                
                // copy over the old data
                size_t offset = tail->offset_ - itr->offset_;
                for (size_t i = 0; i<tail->size; ++i)
                {
                    itr->data_[offset + i] = tail->data_[i];
                }
                blocks_.erase(tail++);
                continue;
            }
            NOTREACHED;
        }

        return &itr->data_[offset - itr->offset_];
    }
    
};

#endif // __SPARSE_RANGE_H__
