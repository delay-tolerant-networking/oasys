#include "FileBackedObjectStream.h"

#include "../storage/FileBackedObject.h"

namespace oasys {

//----------------------------------------------------------------------------
FileBackedObjectOutStream::FileBackedObjectOutStream(FileBackedObject* obj)
    : object_(obj),
      offset_(0)
{
}

//----------------------------------------------------------------------------
int 
FileBackedObjectOutStream::begin()
{
    return 0;
}

//----------------------------------------------------------------------------
int 
FileBackedObjectOutStream::write(const u_char* buf, size_t len)
{
    size_t cc = object_->write_bytes(offset_, buf, len);
    offset_ += len;
    
    ASSERT(cc == len);

    return 0;
}

//----------------------------------------------------------------------------
int 
FileBackedObjectOutStream::end()
{
    return 0;
}

//----------------------------------------------------------------------------
FileBackedObjectInStream::FileBackedObjectInStream(FileBackedObject* obj)
    : object_(obj),
      offset_(0)
{
}

//----------------------------------------------------------------------------
int 
FileBackedObjectInStream::begin()
{
    return 0;
}

//----------------------------------------------------------------------------
int 
FileBackedObjectInStream::read(u_char* buf, size_t len)
{
    size_t cc = object_->read_bytes(offset_, buf, len);
    offset_ += len;

    ASSERT(len == cc);

    return 0;
}

//----------------------------------------------------------------------------
int 
FileBackedObjectInStream::end()
{
    return 0;
}

} // namespace oasys
