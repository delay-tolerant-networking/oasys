#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/mman.h>

#include "../util/jenkins_hash.h"

/**
 * @file Debug memory allocator that keep track of different object
 * types and other memory usage accounting statistics.
 *
 * Options:
 *
 * Define DEBUG_NEW_THREAD_SAFE at compile time to have thread safe
 * statistics at some loss of performance. Otherwise, there will be
 * occasional races in updating the number of live objects.
 *
 * The region of memory in which the debugging information is stored
 * is mmapped to a high address which (hopefully) should not intersect
 * with the addresses in standard usage. The reason for this kind of
 * implementation is that the use of the debug malloc does not change
 * the malloc allocation pattern (e.g. malloc doesn't behave
 * differently with/out the debug malloc stuff.
 *
 */

#ifdef __GNUC__
// Align the memory block that are allocated to correct (and bus error free).
#define _ALIGNED __attribute__((aligned))
#else
#error Must define aligned attribute for this compiler.
#endif 

#define _BYTE             char
#define _CHECKSUM_MAGIC   0xDEADBEEF

#define _DBG_MEM_FRAMES     4
#define _DBG_MEM_TABLE_EXP  10
#define _DBG_MEM_TABLE_SIZE 1<<_DBG_MEM_TABLE_EXP
#define _DBG_MEM_MMAP_HIGH  

/** 
 * Get the containing structure given the address of _ptr->_field.
 * This might not be portable, depending on the way pointer arithmetic
 * works.
 */ 
#define PARENT_PTR(_ptr, _type, _field)                 \
    (_type*) ((_BYTE*)_ptr - (int)&((_type*)0)->_field)

/**
 * An entry in the memory block information table.
 */ 
struct dbg_mem_entry_t {
    void* frames_[_DBG_MEM_FRAMES]; ///< # of stack frames to snarf in LIFO order
    int   live_;             ///< Objects of this type that are alive.
};

/** 
 * Allocated memory block layout will store an entry to the
 * dbg_mem_entry_t of the type of the allocation.
 */
struct dbg_mem_t {
    dbg_mem_entry_t* entry_; 
    _BYTE  block_ _ALIGNED;     ///< actual memory block
};

/**
 * Memory usage statistics class. YOU MUST CALL DbgMemInfo::init() at
 * the start of the program or else none of this will work.
 */ 
class DbgMemInfo {
public:
    /**
     * Set up the memory usage statistics table.
     */
    static void init() {
        entries_ = 0;
        table_   = (dbg_mem_entry_t*)
            calloc(_DBG_MEM_TABLE_SIZE, sizeof(dbg_mem_entry_t));
        
        memset(table_, 0, sizeof(dbg_mem_entry_t) * _DBG_MEM_TABLE_SIZE);
    }

#define MATCH(_f1, _f2)                                                 \
    (memcmp((_f1), (_f2), sizeof(void*) * _DBG_MEM_FRAMES) == 0)

#define MOD(_x, _m)                                             \
    ((_x) & ((unsigned int)(~0))>>((sizeof(int)*8) - (_m)))

    /** 
     * Find the memory block.
     */
    static inline dbg_mem_entry_t* find(void** frames) {
        int key = MOD(jenkins_hash((u_int8_t*)frames, 
                                   sizeof(void*) * _DBG_MEM_FRAMES, 0), 
                      _DBG_MEM_TABLE_EXP);
        dbg_mem_entry_t* entry = &table_[key];
    
        // XXX/bowei - may want to do quadratic hashing later if things
        // get too clustered.
        while(entry->frames_[0] != 0 &&
              !MATCH(frames, entry->frames_))
        {
            ++key;
            entry = &table_[key];
        }

        return entry;
    }

    /**
     * Increment the memory info.
     */ 
    static inline dbg_mem_entry_t* inc(void** frames) {
        dbg_mem_entry_t* entry = find(frames);

        if(entry->frames_[0] == 0)
        {
            memcpy(entry->frames_, frames, sizeof(void*) * _DBG_MEM_FRAMES);
            entry->live_ = 1;
            ++entries_;
        }
        else
        {
            ++(entry->live_);
        }

        return entry;
    }

    /**
     * Decrement the memory info.
     */ 
    static inline dbg_mem_entry_t* dec(void** frames) {
        dbg_mem_entry_t* entry = find(frames);
        
        if(entry->frames_[0] == 0)
        {
            // XXX/bowei ASSERT!
        }
        else
        {
            --(entry->live_);
            // XXX/bowei ASSERT!
        }

        return entry;
    }

    /**
     * Get the memory attribute table.
     */
    static dbg_mem_entry_t** get_table() { return &table_; }

    /**
     * Dump out debugging information
     */
    static void dump();

private:
    /** 
     * Pointer to the allocated memory hash table.
     */
    static int              entries_;
    static dbg_mem_entry_t* table_;
};

#ifndef NDEBUG_MEMORY
// new memory allocation functions ///////////////////////////////////////////
/** 
 * Put the previous stack frame information into frames
 */
static inline void 
set_frame_info(void** frames)
{
#ifdef __GNUC__
#define FILL_FRAME(_x)                                  \
    if(__builtin_frame_address(_x) == 0) {              \
        return;                                         \
    } else {                                            \
        frames[_x-1] = __builtin_return_address(_x);    \
    }

    FILL_FRAME(1);
    FILL_FRAME(2);
    FILL_FRAME(3);
    FILL_FRAME(4);
#undef FILL_FRAME

#else
#error Depends on compiler implementation, implement me.
#endif
}

/** 
 * Regular new call. Untyped allocations have type _UNKNOWN_TYPE.
 */
inline void*
operator new(size_t size)
{
    dbg_mem_t* b = static_cast<dbg_mem_t*>                  
        (malloc(sizeof(dbg_mem_t) + size));                 
    void* frames[_DBG_MEM_FRAMES];
                                                                
    memset(b, 0, sizeof(dbg_mem_t));
    
    set_frame_info(frames);
    b->entry_ = DbgMemInfo::inc(frames);
                   
    printf("new a=%08x, f=[0x%x 0x%x 0x%x 0x%x]\n",              
           &b->block_, frames[0], frames[1], frames[2], frames[3]);     
                                                                
    return (void*)&b->block_;                               
}

/**
 * Typed delete.
 */ 
inline void
operator delete(void *ptr)
{
    dbg_mem_t* b = PARENT_PTR(ptr, dbg_mem_t, block_);
    
    printf("delete a=%08x, f=[0x%x 0x%x 0x%x 0x%x]\n", 
           &b->block_, 
           b->entry_->frames_[0], b->entry_->frames_[1], 
           b->entry_->frames_[2], b->entry_->frames_[3]);
    
    // ASSERT(b->chksum() == b->chksum_);
    free(b);
}

#else // NDEBUG_MEMORY

// XXX/bowei TODO

#endif // NDEBUG_MEMORY

// clean up namespace ////////////////////////////////////////////////////////
#undef _ALIGNED
#undef _BYTE
#undef _CHECKSUM_MAGIC
#undef _UNKNOWN_TYPE
#undef _EMPTY_SLOT
#undef PARENT_PTR

#undef MATCH
#undef MOD

#endif //__MEMORY_H__
