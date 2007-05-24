#ifndef __CHECKEDLOG_H__
#define __CHECKEDLOG_H__

#include "../storage/FileBackedObject.h"
#include "../util/ExpandableBuffer.h"

namespace oasys {

/*!
 * A CRC protected log to guard against short writes.
 *
 * Each record is [CRC, length, data]. The CRC is on on the length and
 * data.
 */
class CheckedLogWriter {
public:
     typedef size_t Handle;

    /*!
     * Interpret the object as a checked log and write to it.
     */
    CheckedLogWriter(FileBackedObject* obj);

    /*!
     * Close fd on destroy.
     */
    ~CheckedLogWriter();

    /*!
     * Write a single record to the log file. This does _not_ force
     * the log file to disk.
     *
     * @return handle to the record for later manipulation.
     */
    Handle write_record(const u_char* buf, size_t len);

    /*!
     * Write that the record can be ignored so does not need to be
     * replayed.
     */
    void ignore(Handle h);

    /*!
     * For all log files written thus far to the disk.
     */ 
    void force();

private:
    FileBackedObject* obj_;
};

/*!
 * Read in the log file record by record.
 */
class CheckedLogReader {
public:
    enum {
        NO_ERR  = 0,
        END     = -1,
        BAD_CRC = -2,
        IGNORE  = -3,
    };

    /*!
     * Interpret the object as a checked log and write to it.
     */
    CheckedLogReader(FileBackedObject* obj);

    /*!
     * Close the file.
     */
    ~CheckedLogReader();

    /*!
     * Read a record from the log.
     *
     * @return 0 on no error, see enum above.
     */
    int read_record(ExpandableBuffer* buf);

private:
    FileBackedObject* obj_;
    
    size_t cur_offset_;
};

} // namespace oasys

#endif /* __CHECKEDLOG_H__ */
