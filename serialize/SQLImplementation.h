#ifndef _SQL_IMPLEMENTATION_H_
#define _SQL_IMPLEMENTATION_H_


#include <string.h>
#include "Serialize.h"

/**
 * Class to encapsulate particular database specific operations.
 */
class SQLImplementation {
public:
    /**
     * Constructor.
     */
    SQLImplementation(const char* binary_datatype,
                      const char* boolean_datatype)
        : binary_datatype_(binary_datatype),
          boolean_datatype_(boolean_datatype) {}

    /**
     * Open a connection to the database.
     */
    virtual int connect(const char* dbname) = 0;

    /**
     * Close the connection to the database.
     */
    virtual int close() = 0;

    /**
     * Check if the database has the specified table.
     */
    virtual bool has_table(const char* tablename) = 0;

    /**
     * Run the given sql query.
     */

    virtual int exec_query(const char* query) = 0;
    
    /**
     * Return the count of the tuples in the previous query.
     */
    virtual int num_tuples() = 0;

    /**
     * Accessor for binary datatype field.
     */
    const char* binary_datatype() { return binary_datatype_; }

    /**
     * Accessor for boolean datatype field.
     */
    const char* boolean_datatype() { return boolean_datatype_; }

    /**
     * Escape a string for use in a sql query.
     */
    virtual const char* escape_string(const char* from) = 0;
    
    /**
     * Escape a binary buffer for use in a sql query.
     */
    virtual const u_char* escape_binary(const u_char* from, int len) = 0;
    
    /**
     * Unescapes the retured value of a SQL query into a binary
     * buffer.
     */
    virtual const u_char* unescape_binary(const u_char* from) = 0;
    
    /**
     * Get the value at the given tuple / field for a previous query.
     */
    virtual const char* get_value(int tuple_no, int field_no) = 0;
    
protected:
    const char* binary_datatype_;	///< column type for binary data
    const char* boolean_datatype_;	///< column type for boolean data
    
private:
    SQLImplementation();		///< default constructor isn't used
};

#endif /* _SQL_IMPLEMENTATION_H_ */
