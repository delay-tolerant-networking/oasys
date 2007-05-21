#ifndef __CHECKEDLOG_H__
#define __CHECKEDLOG_H__

namespace oasys {

/*!
 * A CRC protected log to guard against short writes.
 *
 * Each record is [CRC, length, data]. The CRC is on on the length and
 * data.
 */
class CheckedLogWriter {
public:
    /*!
     * Interpret the object as a checked log and write to it.
     */
    CheckedLogWriter(FileBackedObject* obj);

    /*!
     * Write a single record to the log file. This does _not_ force
     * the log file to disk. 
     */
    void write_record(SerializableObject* contents);

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
        EOF     = -1,
        BAD_CRC = -2,
    };

    /*!
     * Interpret the object as a checked log and write to it.
     */
    CheckedLogReader(FileBackedObject* obj);

    /*!
     * Read a record from the log.
     *
     * @return 0 on no error, see enum above.
     */
    int read_record(SerializableObject* obj);

private:
    FileBackedObject* obj_;
};

} // namespace oasys

#endif /* __CHECKEDLOG_H__ */
