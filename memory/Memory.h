#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <signal.h>
#include <sys/mman.h>

#include "../debug/Debug.h"
#include "../debug/Log.h"
#include "../util/jenkins_hash.h"

/**
 * @file Debug memory allocator that keep track of different object
 * types and other memory usage accounting statistics. 
 *
 * Implementation note: When the tracking hash table gets near 95%
 * full, new will stop adding entries into the hash table, and a
 * warning will be signaled.
 *
 * Optionally, the allocator can be configured so that when it
 * received a user signal, it dumps the current memory use information
 * to dump file.
 *
 * Options:
 *
 * Define DEBUG_NEW_THREAD_SAFE at compile time to have thread safe
 * statistics at some loss of performance. Otherwise, there will be
 * occasional races in updating the number of live objects.
 *
 * Define _DBG_MEM_TABLE_EXP to be power of 2 size of the number of
 * memory entries.
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

#define _BYTE               char
#define _DBG_MEM_MAGIC      0xabcdefab

#define _DBG_MEM_FRAMES     3

#ifndef _DBG_MEM_TABLE_EXP
#define _DBG_MEM_TABLE_EXP  10
#endif 

#define _DBG_MEM_TABLE_SIZE 1<<_DBG_MEM_TABLE_EXP
#define _DBG_MEM_MMAP_HIGH  

/** 
 * Get the containing structure given the address of _ptr->_field.
 */ 
#define PARENT_PTR(_ptr, _type, _field)                 \
    ( (_type*) ((_BYTE*)_ptr - offsetof(_type, _field)) )

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
    unsigned long    magic_;
    dbg_mem_entry_t* entry_; 
    _BYTE            block_ _ALIGNED; ///< actual memory block
};

/**
 * Memory usage statistics class. YOU MUST CALL DbgMemInfo::init() at
 * the start of the program or else none of this will work.
 */ 
class DbgMemInfo {
public:
    enum {
	NO_FLAGS   = 0,
	USE_SIGNAL = 1,   // set up a signal handler for dumping memory information
    };

    
    /**
     * Set up the memory usage statistics table.
     *
     * @param dump_file Dump of the memory usage characteristics.
     */
    static void init(int flags, char* dump_file = 0);

// Find a matching set of frames
#define MATCH(_f1, _f2)                                                 \
    (memcmp((_f1), (_f2), sizeof(void*) * _DBG_MEM_FRAMES) == 0)

// Mod by a power of 2
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
	    PANIC("Decrementing memory entry with no frame info");
        }
        else
        {
            --(entry->live_);
	    if(entry->live_ < 0)
	    {
		PANIC("Memory object live count < 0");
	    }
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
    static void debug_dump();

    /**
     * Dump out memory usage summary to file.
     *
     * @param fd File to output to.
     */
    static void dump_to_file(FILE* f);

    /**
     * Getter for init state
     */
    static bool initialized() { return init_; }

    /**
     * Signal handler for dump file
     */
    static void signal_handler(int signal, siginfo_t* info, void* context);

private:
    /** 
     * Pointer to the allocated memory hash table.
     */
    static int              entries_;
    static dbg_mem_entry_t* table_;
    static bool             init_;
    static FILE*            dump_file_;
    static struct sigaction signal_;
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
    // The reason for these two code paths is the prescence of static
    // initializers which allocate memory on the heap. Memory
    // allocated before init is called is not tracked.
    dbg_mem_t* b = static_cast<dbg_mem_t*>                  
	(malloc(sizeof(dbg_mem_t) + size));                 

    if(b == 0) {
	throw std::bad_alloc();
    }
    
    void* frames[_DBG_MEM_FRAMES];
    memset(b, 0, sizeof(dbg_mem_t));
    b->magic_ = _DBG_MEM_MAGIC;

    // non-init allocations have frame == 0
    if(DbgMemInfo::initialized()) {
	set_frame_info(frames);
	b->entry_ = DbgMemInfo::inc(frames);

	log_debug("/memory", "new a=%p, f=[%p %p %p]\n",              
		  &b->block_, frames[0], frames[1], frames[2]);     
    }
								
    return (void*)&b->block_;                               
}

/**
 * Delete operator. If the memory frame info is 0, then this memory
 * allocation is ignored.
 */ 
inline void
operator delete(void *ptr)
{
    dbg_mem_t* b = PARENT_PTR(ptr, dbg_mem_t, block_);

    ASSERT(b->magic_ == _DBG_MEM_MAGIC);    

    if(b->entry_ != 0) {
	log_debug("/memory", "delete a=%p, f=[%p %p %p]\n", 
		  &b->block_, 
		  b->entry_->frames_[0], b->entry_->frames_[1], 
		  b->entry_->frames_[2]);

	DbgMemInfo::dec(b->entry_->frames_);
    }
    
    free(b);
}

#else // NDEBUG_MEMORY

// XXX/bowei TODO

#endif // NDEBUG_MEMORY

// clean up namespace
#undef _ALIGNED
#undef _BYTE
#undef _DBG_MEM_MAGIC
#undef _UNKNOWN_TYPE
#undef _EMPTY_SLOT

#undef PARENT_PTR
#undef MATCH
#undef MOD

#endif //__MEMORY_H__
