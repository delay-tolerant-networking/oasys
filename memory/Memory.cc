#include <sys/time.h>
#include "Memory.h"

int              DbgMemInfo::entries_   = 0;
dbg_mem_entry_t* DbgMemInfo::table_     = 0;
bool             DbgMemInfo::init_      = false;
FILE*            DbgMemInfo::dump_file_ = 0;
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
	dump_file_ = fopen(dump_file, "w+");
    }

    
    init_ = true;
}


void
DbgMemInfo::debug_dump()
{
    for(int i=0; i<_DBG_MEM_TABLE_SIZE; ++i)
    {
	dbg_mem_entry_t* table = &table_[i];
        if(table->frames_[0] == 0)
            continue;

        log_info("/memory", "%5d: [%p %p %p] live=%d\n",
		 i,
		 table->frames_[0],
		 table->frames_[1],
		 table->frames_[2],
		 table->live_);
    }
}

void
DbgMemInfo::dump_to_file(FILE* f)
{
    if(f == 0) 
    {
	return;
    }

    struct timeval time;
    char buf[256];

    gettimeofday(&time, 0);
    ctime_r(&time.tv_sec, buf);

    fprintf(f, "* %s", buf);
    for(int i=0; i<_DBG_MEM_TABLE_SIZE; ++i)
    {
	dbg_mem_entry_t* table = &table_[i];
        if(table->frames_[0] == 0)
            continue;

	fprintf(f, "%d %p %p %p\n",
		table->live_,
		table->frames_[0],
		table->frames_[1],
		table->frames_[2]);
    }

    fflush(f);
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
