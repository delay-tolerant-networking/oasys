#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "CheckedLog.h"
#include "../util/CRC32.h"
#include "../storage/FileBackedObject.h"

namespace oasys {

//----------------------------------------------------------------------------
CheckedLogWriter::CheckedLogWriter(FileBackedObject* obj)
    : obj_(obj)
{
    obj_->set_flags(FileBackedObject::KEEP_OPEN);
}

//----------------------------------------------------------------------------
CheckedLogWriter::~CheckedLogWriter()
{
    obj_->set_flags(0);
}

//----------------------------------------------------------------------------
CheckedLogWriter::Handle
CheckedLogWriter::write_record(const u_char* buf, size_t len)
{
    u_char ignore = '*';
    CRC32 crc;
    
    u_char len_buf[4];
    len_buf[0] = (len >> 24) & 0xFF;
    len_buf[1] = (len >> 16) & 0xFF;
    len_buf[2] = (len >> 8)  & 0xFF;
    len_buf[3] = len         & 0xFF;

    size_t handle = obj_->size();
    crc.update(len_buf, 4);
    crc.update(buf, len);

    u_char crc_buf[4];
    crc_buf[0] = (crc.value() >> 24) & 0xFF;
    crc_buf[1] = (crc.value() >> 16) & 0xFF;
    crc_buf[2] = (crc.value() >> 8)  & 0xFF;
    crc_buf[3] = crc.value()         & 0xFF;

    obj_->append_bytes(&ignore, 1);
    obj_->append_bytes(crc_buf, 4);
    obj_->append_bytes(len_buf, 4);
    obj_->append_bytes(buf, len);

    return handle;
}

//----------------------------------------------------------------------------
void
CheckedLogWriter::ignore(Handle h)
{
    u_char ignore = '!';
    obj_->write_bytes(h, &ignore, 1);
    obj_->fsync_data();
}

//----------------------------------------------------------------------------
void 
CheckedLogWriter::force()
{
    obj_->fsync_data();
}

//----------------------------------------------------------------------------
CheckedLogReader::CheckedLogReader(FileBackedObject* obj)
    : obj_(obj),
      cur_offset_(0)
{
    obj_->set_flags(FileBackedObject::KEEP_OPEN);
}

//----------------------------------------------------------------------------
CheckedLogReader::~CheckedLogReader()
{
    obj_->set_flags(0);
}

//----------------------------------------------------------------------------
int 
CheckedLogReader::read_record(ExpandableBuffer* buf)
{
    if (cur_offset_ == obj_->size())
    {
        return END;
    }
    
    u_char ignore;
    u_char crc_buf[sizeof(CRC32::CRC_t)];
    u_char len_buf[sizeof(size_t)];
    
    int cc = obj_->read_bytes(cur_offset_, &ignore, 1);
    if (cc != 1)
    {
        return BAD_CRC;
    }
    ++cur_offset_;

    cc = obj_->read_bytes(cur_offset_, crc_buf, sizeof(CRC32::CRC_t));
    if (cc != sizeof(CRC32::CRC_t))
    {
        return BAD_CRC;
    }
    cur_offset_ += sizeof(CRC32::CRC_t);

    cc = obj_->read_bytes(cur_offset_, len_buf, sizeof(size_t));
    if (cc != sizeof(size_t))
    {
        return BAD_CRC;
    }
    cur_offset_ += 4;

    size_t len = (len_buf[0] << 24) | (len_buf[1] << 16) | 
                 (len_buf[2] << 8) | len_buf[3];

    // sanity check so we don't run out of memory due to corruption
    if (len > obj_->size())
    {
        return BAD_CRC;
    }
    
    buf->reserve(len);
    cc = obj_->read_bytes(cur_offset_, 
                          reinterpret_cast<u_char*>(buf->raw_buf()),
                          len);
    cur_offset_ += cc;

    if (cc != static_cast<int>(len))
    {
        return BAD_CRC;
    }

    CRC32 crc;
    crc.update(len_buf, 4);
    crc.update(reinterpret_cast<u_char*>(buf->raw_buf()), len);
    
    if (crc.value() != CRC32::from_bytes(crc_buf))
    {
        return BAD_CRC;
    }
    
    return (ignore == '!') ? IGNORE : 0;
}

} // namespace oasys
