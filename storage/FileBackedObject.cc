#include "FileBackedObject.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../debug/DebugUtils.h"
#include "../io/FileUtils.h"

namespace oasys {

//----------------------------------------------------------------------------
FileBackedObject::Tx::Tx(const FileBackedObject* backing_file, int flags)
    : Logger("FileBackedObject", "/store/file-backed/tx"),
      original_filename_(backing_file->filename()),
      tx_file_(0)
{
    logpathf("/store/file-backed/tx/%s", original_filename_.c_str());

    std::string tx_filename = original_filename_ + ".tx";
    int err = FileUtils::fast_copy(original_filename_.c_str(), tx_filename.c_str());
    ASSERT(err == 0);

    tx_file_ = new FileBackedObject(tx_filename, flags);

    log_debug("tx started");
}
        
//----------------------------------------------------------------------------
FileBackedObject::Tx::~Tx()
{
    if (tx_file_ != 0)
    {
        int err = rename(tx_file_->filename().c_str(), 
                         original_filename_.c_str());
        ASSERT(err == 0);
        delete_z(tx_file_);
    }

    log_debug("tx committed");
}
        
//----------------------------------------------------------------------------
FileBackedObject*
FileBackedObject::Tx::object()
{
    return tx_file_;
}
        
//----------------------------------------------------------------------------
void
FileBackedObject::Tx::abort()
{
    tx_file_->unlink();
    delete_z(tx_file_);
    
    log_debug("tx aborted");
}

//----------------------------------------------------------------------------
FileBackedObject::~FileBackedObject()
{
    if (fd_ != -1)
    {
        ::close(fd_);
        fd_ = -1;
    }
}

//----------------------------------------------------------------------------
FileBackedObject::TxHandle 
FileBackedObject::start_tx(int flags)
{
    ASSERT(fd_ != -1);

    return TxHandle(new Tx(this, flags));
}
    
//----------------------------------------------------------------------------
void 
FileBackedObject::get_stats(struct stat* stat_buf) const
{
    ASSERT(fd_ != -1);
    
    int err = stat(filename_.c_str(), stat_buf);
    ASSERT(err == 0);
}

//----------------------------------------------------------------------------
void 
FileBackedObject::set_stats(struct stat* stat_buf)
{
    ASSERT(fd_ != -1);

    (void) stat_buf;
    // XXX/bowei -- LALALA
}

//----------------------------------------------------------------------------
size_t
FileBackedObject::size() const
{
    struct stat stat_buf;
    get_stats(&stat_buf);
    
    return stat_buf.st_size;
}

//----------------------------------------------------------------------------
size_t 
FileBackedObject::read_bytes(size_t offset, u_char* buf, size_t length) const
{
    open();

    ASSERT(fd_ != -1);
    
    off_t off = lseek(fd_, offset, SEEK_SET);
    ASSERT(static_cast<size_t>(off) == offset);

    int cc = read(fd_, buf, length);
    ASSERT(static_cast<size_t>(cc) == length);

    close();

    return cc;
}

//----------------------------------------------------------------------------
size_t 
FileBackedObject::write_bytes(size_t offset, const u_char* buf, size_t length)
{
    open();

    ASSERT(fd_ != -1);

    off_t off = lseek(fd_, offset, SEEK_SET);
    ASSERT(static_cast<size_t>(off) == offset);
    
    int cc = write(fd_, buf, length);
    ASSERT(static_cast<size_t>(cc) == length);

    close();
    
    return cc;
}

//----------------------------------------------------------------------------
void 
FileBackedObject::truncate(size_t size)
{
    open();

    ASSERT(fd_ != -1);
    
    int err = ftruncate(fd_, size);
    ASSERT(err == 0);

    close();
}

//----------------------------------------------------------------------------
FileBackedObject::FileBackedObject(const std::string& filename, 
                                   int flags)
    : filename_(filename),
      fd_(-1),
      flags_(flags)
{
    if (flags_ & KEEP_OPEN)
    {
        open();
    }
}

//----------------------------------------------------------------------------
void
FileBackedObject::open() const
{
    ASSERT(! (flags_ & UNLINKED));

    if (fd_ != -1)
    {
        return;
    }
    
    fd_ = ::open(filename_.c_str(), O_RDWR);
    ASSERT(fd_ != -1);
}

//----------------------------------------------------------------------------
void
FileBackedObject::close() const
{
    if (fd_ == -1 || flags_ & KEEP_OPEN)
    {
        return;
    }

    ::close(fd_);
    fd_ = -1;
}

//----------------------------------------------------------------------------
void
FileBackedObject::unlink()
{
    if (fd_ != 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
    
    int err = ::unlink(filename_.c_str());
    ASSERT(err == 0);
    
    filename_ = "/INVALID_FILE";
    flags_ |= UNLINKED;
}

} // namespace oasys
