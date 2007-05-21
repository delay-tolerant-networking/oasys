#include "CheckedLog.h"

namespace oasys {

//----------------------------------------------------------------------------
CheckedLogWriter::CheckedLogWriter(FileBackedObject* obj)
    : log_(log)
{
}

//----------------------------------------------------------------------------
void 
CheckedLogWriter::write_record(SerializableObject* contents)
{
    
}

//----------------------------------------------------------------------------
void 
CheckedLogWriter::force()
{
}

//----------------------------------------------------------------------------
CheckedLogReader::CheckedLogReader(FileBackedObject* obj)
    : obj_(obj)
{}

//----------------------------------------------------------------------------
int 
CheckedLogReader::read_record(SerializableObject* obj)
{
}

} // namespace oasys
