#ifndef __FILEBACKEDOBJECT_H__
#define __FILEBACKEDOBJECT_H__

#include <memory>

#include "../debug/Logger.h"

// predecl
struct stat;

namespace oasys {

class FileBackedObject;
class FileBackedObjectStore;

typedef std::auto_ptr<FileBackedObject> FileBackedObjectHandle;

class FileBackedObject {
    class Tx;
    
    friend class FileBackedObject::Tx;
    friend class FileBackedObjectStore;
    
public:
    /*!
     * Parameters to the FileBackedObject
     */
    enum {
        KEEP_OPEN = 1 << 0,     // keep the fd open while FileBackedObject exists.
    };
    
    /*! 
     * This implements a simple per object transaction mechanism using
     * the atomic rename provided by the filesystem.
     *
     * No more than one transaction should be active at a time. (right
     * now).
     *
     * @param flags to the FileBackedObject.
     */
    class Tx : public Logger {
    public:
        Tx(const FileBackedObject* backing_file, int flags);

	/*!
	 * Destructor commits the transaction if it already hasn't
	 * been committed.
	 */
	~Tx();
        
        /*!
         * @return The FileObject that is going to be committed. const
         * because all of the operations should go through the
         * transaction.
         */
        FileBackedObject* object();

        /*!
         * Abort the transaction.
         */
        void abort();
        
    private:
        std::string original_filename_;
        FileBackedObject* tx_file_;
    };

    /*!
     * Use TxHandle to manage the Tx.
     */
    typedef std::auto_ptr<Tx> TxHandle;

    /*!
     * Closes file on destruction.
     */
    ~FileBackedObject();

    /*!
     * @return A new transaction on the file.
     */
    TxHandle start_tx(int flags);
    
    /*!
     * @return new Tx object. Caller assumes responsibility of the TX.
     */

    /*!
     * Return the stats maintained by the file system.
     */
    void get_stats(struct stat* stat_buf) const;

    /*!
     * This only sets the following fields:
     *   uid, gid, mode, times
     */
    void set_stats(struct stat* stat_buf);

    /*!
     * @return Number of bytes read.
     */
    size_t read_bytes(size_t offset, u_char* buf, size_t length) const;

    /*!
     * Will extend the length of the file if needed.
     * 
     * @return Number of bytes written.
     */
    size_t write_bytes(size_t offset, const u_char* buf, size_t length);

    /*!
     * Make the file size.
     */
    void truncate(size_t size);

    /*!
     * @return Name of the backing file for the object.
     */
    const std::string& filename() const { return filename_; }

private:
    std::string filename_;
    int         fd_;
    int         flags_;

    /*!
     * Opens file on construction.
     *
     * @param filename Name of the backing file.
     * @param flags    Flags for behavior (see above).
     */
    FileBackedObject(const std::string& filename, int flags);
    
    /*!
     * Open the file if needed and according to the flags.
     */
    void open();
    
    /*!
     * Close the file if needed and according to the flags.
     */
    void close();
    
    /*!
     * Delete the file from the filesystem.
     */
    void unlink();
};

} // namespace oasys

#endif /* __FILEBACKEDOBJECT_H__ */
