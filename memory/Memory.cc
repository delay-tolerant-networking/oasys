#include "Memory.h"

int              DbgMemInfo::entries_ = 0;
dbg_mem_entry_t* DbgMemInfo::table_   = 0;
bool             DbgMemInfo::init_    = false;

void
DbgMemInfo::dump()
{
    for(int i=0; i<_DBG_MEM_TABLE_SIZE; ++i)
    {
	dbg_mem_entry_t* table = &table_[i];
        if(table->frames_[0] == 0)
            continue;

        log_info("/memory", "%5d: [%p %p %p %p] live=%d\n",
		 i,
		 table->frames_[0],
		 table->frames_[1],
		 table->frames_[2],
		 table->frames_[3],
		 table->live_);
    }
}
