/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2004 Intel Corporation. All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 *   Neither the name of the Intel Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _OASYS_SQL_IMPLEMENTATION_H_
#define _OASYS_SQL_IMPLEMENTATION_H_


#include <string.h>
#include "Serialize.h"

namespace oasys {

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

} // namespace oasys

#endif /* _OASYS_SQL_IMPLEMENTATION_H_ */
