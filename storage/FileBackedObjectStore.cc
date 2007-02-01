#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "../io/FileUtils.h"
#include "FileBackedObjectStore.h"

namespace oasys {

//----------------------------------------------------------------------------
FileBackedObjectStore::FileBackedObjectStore(const std::string& root)
    : Logger("FileBackedObjectStore", "/store/file-backed"),
      root_(root)
{
    // check that the root directory is readable
    struct stat dir_stat;
    int err = stat(root_.c_str(), &dir_stat);
    err = errno;

    ASSERTF(err == 0, "Can't stat %s, maybe directory doesn't exist?, error=%s",
	    root_.c_str(), strerror(err));

    logpathf("/store/file-backed/%s", root.c_str());
    // XXX/bowei -- check the permissions are ok
}

//----------------------------------------------------------------------------
int 
FileBackedObjectStore::new_object(const std::string& key)
{
    if (object_exists(key))
    {
        return -1;
    }
    
    int fd = open(object_path(key).c_str(), O_WRONLY | O_CREAT | O_EXCL);
    ASSERT(fd != -1);
    close(fd);

    return 0;
}
    
//----------------------------------------------------------------------------
FileBackedObjectHandle
FileBackedObjectStore::get_handle(const std::string& key, int flags)
{
    ASSERT(object_exists(key));

    return FileBackedObjectHandle(
        new FileBackedObject(object_path(key), flags));
}
    
//----------------------------------------------------------------------------
bool 
FileBackedObjectStore::object_exists(const std::string& key)
{
    struct stat stat_buf;
    int err = stat(object_path(key).c_str(), &stat_buf);
    if (err == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//----------------------------------------------------------------------------
int 
FileBackedObjectStore::del_object(const std::string& key)
{
    if (! object_exists(key))
    {
        return -1;
    }

    int err = unlink(object_path(key).c_str());
    ASSERT(err == 0);
    
    return 0;
}

//----------------------------------------------------------------------------
int 
FileBackedObjectStore::copy_object(const std::string& src,  
				   const std::string& dest)
{
    std::string src_path, dest_path;

    if (! object_exists(src))
    {
	log_debug("src %s doesn't exist, not copying", src.c_str());
	return -1;
    }
    
    if (object_exists(dest))
    {
	log_debug("dest %s exists, not copying", dest.c_str());
	return -1;
    }

    int err = FileUtils::fast_copy(object_path(src).c_str(),
				   object_path(dest).c_str());
    ASSERT(err != -1);
    return 0;
}

//----------------------------------------------------------------------------
std::string
FileBackedObjectStore::object_path(const std::string& key)
{
    std::string path = root_;
    path.append("/");
    path.append(key);

    return path;
}

} // namespace oasys
