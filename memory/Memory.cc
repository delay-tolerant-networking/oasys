#include "Memory.h"

int              DbgMemInfo::entries_ = 0;
dbg_mem_entry_t* DbgMemInfo::table_   = 0;

void
DbgMemInfo::dump()
{
    for(int i=0; i<_DBG_MEM_TABLE_SIZE; ++i)
    {
        if(table_[i].frames_[0] == 0)
            continue;

        printf("%d: [%08x %08x %08x %08x] %d\n",
               i,
               table_[i].frames_[0],
               table_[i].frames_[1],
               table_[i].frames_[2],
               table_[i].frames_[3],
               table_[i].live_);
    }
}
