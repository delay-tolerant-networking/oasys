#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "Memory.h"

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
    for(int i=0; i<_DBG_MEM_TABLE_SIZE; ++i) {
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
    ctime_r(&time.tv_sec, buf);
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
