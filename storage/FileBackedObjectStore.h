#ifndef __FILEBACKEDINDEX_H__
#define __FILEBACKEDINDEX_H__

#include <string>
#include <memory>

#include "../debug/Logger.h"
#include "../storage/FileBackedObject.h"

struct stat;

namespace oasys {

class FileBackedObject;
class FileBackedObjectStore;

//----------------------------------------------------------------------------
class FileBackedObjectStore : public Logger {
public:
    /*!
     * @root Root of the file backed store.
     */
    FileBackedObjectStore(const std::string& root);
    
    /*!
     * Put a new object referenced by key in the store.
     *
     * @return 0 no error, -1 if the object already exists.
     */
    int new_object(const std::string& key);
    
    /*! 
     * Get the handle to object referenced by key.
     *
     * The object MUST exist already in the store.
     */
    FileBackedObjectHandle get_handle(const std::string& key, int flags);
    
    /*!
     * @return True if the object exists.
     */
    bool object_exists(const std::string& key);
    
    /*!
     * Delete the object reference by handle.
     *
     * @return 0 no error, -1 if object didn't exist.
     */
    int del_object(const std::string& key);
    
    /*!
     * Fast copy to make a new object.
     *
     * @return -1 if the src doesn't exist or the dest already exists.
     */
    int copy_object(const std::string& src, const std::string& dest);
    
private:
    std::string root_;

    /*!
     * Path to the object.
     */
    std::string object_path(const std::string& key);
};

} // namespace oasys

#endif /* __FILEBACKEDINDEX_H__ */
