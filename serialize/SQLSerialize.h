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
#ifndef _SQL_SERIALIZE_H_
#define _SQL_SERIALIZE_H_

/**
 * @file
 *
 * This file defines the serialization and deserialization objects to
 * be used in an SQL storage context.
 */

#include "Serialize.h"
#include "../util/StringBuffer.h"

// XXX/namespace
using oasys::StringBuffer;

class SQLImplementation;

/**
 * SQLQuery implements common functionality used when building up a
 * SQL query string.
 */
class SQLQuery : public SerializeAction {
public:
    /**
     * Constructor.
     */
    SQLQuery(action_t type, const char* table_name, SQLImplementation* impl,
             const char* initial_query = 0);
    
    /**
     * Return the constructed query string.
     */
    const char* query() { return query_.c_str(); }
    
    /**
     * Return a reference to the query buffer.
     */
    StringBuffer* querybuf() { return &query_; }
    
protected:
    const char* table_name_;
    SQLImplementation* sql_impl_ ;
    StringBuffer query_;
};

/**
 * SQLInsert is a SerializeAction that builts up a SQL "INSERT INTO"
 * query statement based on the values in an object.
 */
class SQLInsert : public SQLQuery {
public:
    /**
     * Constructor.
     */
    SQLInsert(const char* table_name, SQLImplementation *impl);
  
    virtual void begin_action();
    virtual void end_action();
    
    /**
     * Since insert doesn't modify the object, define a variant of
     * action() that operates on a const SerializableObject.
     */
    int action(const SerializableObject* const_object)
    {
        return(SerializeAction::action((SerializableObject*)const_object));
    }
        
    // Virtual functions inherited from SerializeAction
    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, int32_t* i);
    void process(const char* name, int16_t* i);
    void process(const char* name, int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, bool alloc_copy);
    void process(const char* name, std::string* s);
};

/**
 * SQLUpdate is a SerializeAction that builts up a SQL "UPDATE"
 * query statement based on the values in an object.
 */
class SQLUpdate : public SQLQuery {
public:
    /**
     * Constructor.
     */
    SQLUpdate(const char* table_name, SQLImplementation *impl);
  
    virtual void begin_action();
    virtual void end_action();
    
    /**
     * Since update doesn't modify the object, define a variant of
     * action() that operates on a const SerializableObject.
     */
    int action(const SerializableObject* const_object)
    {
        return(SerializeAction::action((SerializableObject*)const_object));
    }
        
    // Virtual functions inherited from SerializeAction
    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, int32_t* i);
    void process(const char* name, int16_t* i);
    void process(const char* name, int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, bool alloc_copy);
    void process(const char* name, std::string* s);
};

/**
 * SQLTableFormat is a SerializeAction that builts up a SQL "CREATE
 * TABLE" query statement based on the values in an object.
 */
class SQLTableFormat : public SQLQuery {
public:
    /**
     * Constructor.
     */
    SQLTableFormat(const char* table_name, SQLImplementation *impl);
    
    virtual void begin_action();
    virtual void end_action();
    
    /**
     * Since table format doesn't modify the object, define a variant
     * of action() that operates on a const SerializableObject.
     */
    int action(const SerializableObject* const_object)
    {
        return(SerializeAction::action((SerializableObject*)const_object));
    }
        
    // Virtual functions inherited from SerializeAction
    void process(const char* name,  SerializableObject* object);
    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, bool alloc_copy);
    void process(const char* name, std::string* s);

 protected:
    void append(const char* name, const char* type);
    StringBuffer column_prefix_;
};

/**
 * SQLExtract is a SerializeAction that constructs an object's
 * internals from the results of a SQL "select" statement.
 */
class SQLExtract : public SerializeAction {
public:
    SQLExtract(SQLImplementation *impl);
    
    // get the next field from the db
    const char* next_field() ;

    // Virtual functions inherited from SerializeAction
    void process(const char* name, u_int32_t* i);
    void process(const char* name, u_int16_t* i);
    void process(const char* name, u_int8_t* i);
    void process(const char* name, bool* b);
    void process(const char* name, u_char* bp, size_t len);
    void process(const char* name, u_char** bp, size_t* lenp, bool alloc_copy);
    void process(const char* name, std::string* s); 

 protected:
    int field_;		///< counter over the fields in the returned tuple
    
 private:
    SQLImplementation *sql_impl_;
};

#endif /* _SQL_SERIALIZE_H_ */
