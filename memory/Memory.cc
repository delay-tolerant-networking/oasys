#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "Memory.h"

#define _BYTE char
#define _DBG_MEM_MAGIC      0xf00dbeef


int              DbgMemInfo::entries_   = 0;
dbg_mem_entry_t* DbgMemInfo::table_     = 0;
bool             DbgMemInfo::init_      = false;
int              DbgMemInfo::dump_file_ = -1;
struct sigaction DbgMemInfo::signal_;

void
DbgMemInfo::init(   
    int   flags,
    char* dump_file
    )
{
    // XXX/bowei Needs to be changed to MMAP
    entries_ = 0;
    table_   = (dbg_mem_entry_t*)
	calloc(_DBG_MEM_TABLE_SIZE, sizeof(dbg_mem_entry_t));
    memset(table_, 0, sizeof(dbg_mem_entry_t) * _DBG_MEM_TABLE_SIZE);

    if(flags & DbgMemInfo::USE_SIGNAL) 
    {

	memset(&signal_, 0, sizeof(struct sigaction));
	signal_.sa_sigaction = DbgMemInfo::signal_handler;
	//signal_.sa_mask    = 
	signal_.sa_flags     = SA_SIGINFO;
	
	::sigaction(SIGUSR2, &signal_, 0);
    }
    
    if(dump_file) 
    {
	dump_file_ = open(dump_file, 
			  O_WRONLY | O_CREAT | O_APPEND);
    }

    init_ = true;
}


void
DbgMemInfo::debug_dump()
{
    for(int i=0; i<_DBG_MEM_TABLE_SIZE; ++i) 
    {
	dbg_mem_entry_t* entry = &table_[i];
        if(entry->frames_[0] == 0)
            continue;

        log_info("/memory", "%5d: [%p %p %p] live=%d size=%.2fkb\n",
		 i,
		 entry->frames_[0],
		 entry->frames_[1],
		 entry->frames_[2],
		 entry->live_,
		 (float)entry->size_/1000);
    }
}

void
DbgMemInfo::dump_to_file(int fd)
{
    if(fd == -1) {
	return;
    }

    struct timeval time;
    char buf[256];

    gettimeofday(&time, 0);
    ctime_r((const time_t*)&time.tv_sec, buf);
    write(fd, buf, strlen(buf));

    for(int i=0; i<_DBG_MEM_TABLE_SIZE; ++i)
    {
	dbg_mem_entry_t* entry = &table_[i];
        if(entry->frames_[0] == 0)
            continue;
        
        snprintf(buf, 256,
                 "%5d: [%p %p %p] live=%d size=%.2fkb\n",
                 i,
                 entry->frames_[0],
                 entry->frames_[1],
                 entry->frames_[2],
                 entry->live_,
                 (float)entry->size_/1000);
        
        write(fd, buf, strlen(buf));
    }
    fsync(fd);
}

void
DbgMemInfo::signal_handler(
    int        signal, 
    siginfo_t* info, 
    void*      context
    )
{
    dump_to_file(dump_file_);
}

void* 
operator new(size_t size) throw (std::bad_alloc)
{
    // The reason for these two code paths is the prescence of static
    // initializers which allocate memory on the heap. Memory
    // allocated before init is called is not tracked.
    dbg_mem_t* b = static_cast<dbg_mem_t*>                  
	(malloc(sizeof(dbg_mem_t) + size));                 

    if(b == 0) {
	throw std::bad_alloc();
    }
    

    memset(b, 0, sizeof(dbg_mem_t));
    b->magic_ = _DBG_MEM_MAGIC;
    b->size_  = size;

    // non-init allocations have frame == 0
    if(DbgMemInfo::initialized()) {
        void* frames[_DBG_MEM_FRAMES];
	
        set_frame_info(frames);
	b->entry_ = DbgMemInfo::inc(frames, size);

	log_debug("/memory", "new a=%p, f=[%p %p %p]\n",              
		  &b->block_, frames[0], frames[1], frames[2]);     
    }
								
    return (void*)&b->block_;                               
}

void operator delete(void *ptr) throw ()
{
    dbg_mem_t* b = PARENT_PTR(ptr, dbg_mem_t, block_);

    ASSERT(b->magic_ == _DBG_MEM_MAGIC);    

    if(b->entry_ != 0) {
	log_debug("/memory", "delete a=%p, f=[%p %p %p]\n", 
		  &b->block_, 
		  b->entry_->frames_[0], b->entry_->frames_[1], 
		  b->entry_->frames_[2]);

	DbgMemInfo::dec(b);
    }
    
    char* bp = (char*)(b);
    unsigned int size = b->size_;

    for(unsigned int i=0; i<size; ++i)
    {
        bp[i] = 0xF0;
    }

    free(b);
}
