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
#include "SQLSerialize.h"
#include "SQLImplementation.h"

#include "debug/Debug.h"
#include "debug/Log.h"
#include "util/StringUtils.h"

namespace oasys {

/******************************************************************************
 *
 * SQLQuery
 *
 *****************************************************************************/

/**
 * Constructor.
 */
SQLQuery::SQLQuery(action_t type, const char* table_name,
                   SQLImplementation* impl, const char* initial_query)
    
    : SerializeAction(type, Serialize::CONTEXT_LOCAL),
      table_name_(table_name),
      sql_impl_(impl),
      query_(256, initial_query)
{
}

/******************************************************************************
 *
 * SQLInsert
 *
 *****************************************************************************/

/**
 * Constructor.
 */
SQLInsert::SQLInsert(const char* table_name, SQLImplementation* impl)
    : SQLQuery(Serialize::MARSHAL, table_name, impl)
{
}


// Virtual functions inherited from SerializeAction

/**
 * Initialize the query buffer.
 */
void 
SQLInsert::begin_action() 
{
    query_.appendf("INSERT INTO %s  VALUES(",table_name_);
}

/**
 * Clean the query in the end, trimming the trailing ',' and adding a
 * closing parenthesis.
 */
void 
SQLInsert::end_action() 
{
    if (query_.data()[query_.length() - 1] == ',') {
	query_.data()[query_.length() - 1] = ')';
    }
}


void 
SQLInsert::process(const char* name, u_int32_t* i)
{
    query_.appendf("%u,", *i);
}

void 
SQLInsert::process(const char* name, u_int16_t* i)
{
    query_.appendf("%u,", *i);
}

void 
SQLInsert::process(const char* name, u_int8_t* i)
{
    query_.appendf("%u,", *i);
}

void 
SQLInsert::process(const char* name, int32_t* i)
{
#ifdef __CYGWIN__
    query_.appendf("%ld,", *i);
#else
    query_.appendf("%d,", *i);
#endif
}

void 
SQLInsert::process(const char* name, int16_t* i)
{
    query_.appendf("%d,", *i);
}

void 
SQLInsert::process(const char* name, int8_t* i)
{
    query_.appendf("%d,", *i);
}

void 
SQLInsert::process(const char* name, bool* b)
{

    if (*b) {
        query_.append("'TRUE',");
    } else {
        query_.append("'FALSE',");
    }
}


void 
SQLInsert::process(const char* name, std::string* s) 
{
    query_.appendf("'%s',", sql_impl_->escape_string(s->c_str()));
}

void 
SQLInsert::process(const char* name, u_char* bp, size_t len)
{
    query_.appendf("'%s',", sql_impl_->escape_binary(bp, len));
}

void 
SQLInsert::process(const char* name, u_char** bp, size_t* lenp, bool alloc_copy)
{
    NOTIMPLEMENTED;
}

/******************************************************************************
 *
 * SQLUpdate
 *
 *****************************************************************************/

/**
 * Constructor.
 */
SQLUpdate::SQLUpdate(const char* table_name, SQLImplementation* impl)
    : SQLQuery(Serialize::MARSHAL, table_name, impl)
{
}


// Virtual functions inherited from SerializeAction
void 
SQLUpdate::begin_action() 
{
    query_.appendf("UPDATE %s SET ",table_name_);
}

void 
SQLUpdate::end_action() 
{
    if (query_.data()[query_.length() - 2] == ',') {
	query_.data()[query_.length() - 2] =  ' ';
    }
}

void 
SQLUpdate::process(const char* name, u_int32_t* i)
{
    query_.appendf("%s = %u, ", name, *i);
}

void 
SQLUpdate::process(const char* name, u_int16_t* i)
{
    query_.appendf("%s = %u, ", name, *i);
}

void 
SQLUpdate::process(const char* name, u_int8_t* i)
{
    query_.appendf("%s = %u, ", name, *i);
}

void 
SQLUpdate::process(const char* name, int32_t* i)
{
#ifdef __CYGWIN__
    query_.appendf("%s = %ld, ", name, *i);
#else
    query_.appendf("%s = %d, ", name, *i);
#endif
}

void 
SQLUpdate::process(const char* name, int16_t* i)
{
    query_.appendf("%s = %d, ", name, *i);
}

void 
SQLUpdate::process(const char* name, int8_t* i)
{
    query_.appendf("%s = %d, ", name, *i);
}

void 
SQLUpdate::process(const char* name, bool* b) {

    if (*b) {
        query_.appendf("%s = 'TRUE', ", name);
    } else {
        query_.appendf("%s = 'FALSE', ", name);
    }
}


void 
SQLUpdate::process(const char* name, std::string* s) 
{
    query_.appendf("%s = '%s', ", name, sql_impl_->escape_string(s->c_str()));
}

void 
SQLUpdate::process(const char* name, u_char* bp, size_t len)
{
    query_.appendf("%s = '%s', ", name, sql_impl_->escape_binary(bp, len));
}

void 
SQLUpdate::process(const char* name, u_char** bp, size_t* lenp, bool alloc_copy)
{
    NOTIMPLEMENTED;
}

/******************************************************************************
 *
 * SQLTableFormat
 *
 *****************************************************************************/

/**
 * Constructor.
 */

SQLTableFormat::SQLTableFormat(const char* table_name,
                               SQLImplementation* impl)
    : SQLQuery(Serialize::INFO, table_name, impl)
{
}


// Virtual functions inherited from SerializeAction

void 
SQLTableFormat::begin_action() 
{
    query_.appendf("CREATE TABLE  %s  (",table_name_);
}

void 
SQLTableFormat::end_action() 
{
    if (query_.data()[query_.length() - 1] == ',') {
	query_.data()[query_.length() - 1] =  ')';
    }
}


void
SQLTableFormat::process(const char* name,  SerializableObject* object) 
{
    int old_len = column_prefix_.length();

    column_prefix_.appendf("%s__", name);
    object->serialize(this);
    column_prefix_.trim(column_prefix_.length() - old_len);
}

void 
SQLTableFormat::append(const char* name, const char* type)
{
    query_.appendf("%.*s%s %s,",
                   (int)column_prefix_.length(), column_prefix_.data(),
                   name, type);
}

// Virtual functions inherited from SerializeAction
void 
SQLTableFormat::process(const char* name, u_int32_t* i)
{
    append(name, "BIGINT");
}

void 
SQLTableFormat::process(const char* name, u_int16_t* i)
{
    append(name, "INTEGER");
}

void
SQLTableFormat::process(const char* name, u_int8_t* i) {

    append(name, "SMALLINT");
}
 

// Mysql and Postgres do not have common implementation of bool
// XXX/demmer fix this
void 
SQLTableFormat::process(const char* name, bool* b)
{
//    append(name, "BOOLEAN");
    append(name, "TEXT");
}

void 
SQLTableFormat::process(const char* name, std::string* s) 
{
    append(name, "TEXT");
}

void 
SQLTableFormat::process(const char* name, u_char* bp, size_t len)
{
    append(name,sql_impl_->binary_datatype());
}
    
void 
SQLTableFormat::process(const char* name, u_char** bp, size_t* lenp, bool alloc_copy)
{
    append(name,sql_impl_->binary_datatype());
}

/******************************************************************************
 *
 * SQLExtract
 *
 *****************************************************************************/

/**
 * Constructor.
 */

SQLExtract::SQLExtract(SQLImplementation* impl)
    : SerializeAction(Serialize::UNMARSHAL, 
                      Serialize::CONTEXT_LOCAL)
{
    field_ = 0;
    sql_impl_ = impl;
}

const char* 
SQLExtract::next_field()
{
    return sql_impl_->get_value(0, field_++);
}

void
SQLExtract::process(const char* name, u_int32_t* i)
{
    const char* buf = next_field();
    if (buf == NULL) return;
    
    *i = atoi(buf) ; 

    if (log_) logf(log_, LOG_DEBUG, "<=int32(%d)", *i);
}

void 
SQLExtract::process(const char* name, u_int16_t* i)
{
    const char* buf = next_field();
    if (buf == NULL) return;

    *i = atoi(buf) ; 
    
    if (log_) logf(log_, LOG_DEBUG, "<=int16(%d)", *i);
}



void 
SQLExtract::process(const char* name, u_int8_t* i)
{
    const char* buf = next_field();
    if (buf == NULL) return;
    
    *i = buf[0];
    
    if (log_) logf(log_, LOG_DEBUG, "<=int8(%d)", *i);
}

void 
SQLExtract::process(const char* name, bool* b)
{
    const char* buf = next_field();

    if (buf == NULL) return;

    switch(buf[0]) {
    case 'T':
    case 't':
    case '1':
    case '\1':
        *b = true;
        break;
    case 'F':
    case 'f':
    case '0':
    case '\0':
        *b = false;
        break;
    default:
        logf("/sql", LOG_ERR, "unexpected value '%s' for boolean column", buf);
        error_ = true;
        return;
    }
    
    if (log_) logf(log_, LOG_DEBUG, "<=bool(%c)", *b ? 'T' : 'F');
}

void 
SQLExtract::process(const char* name, std::string* s)
{
    const char* buf = next_field();
    if (buf == NULL) return;
    
    s->assign(buf);
    
    size_t len = s->length();
    
    if (log_) logf(log_, LOG_DEBUG, "<=string(%d: '%.*s')",
                   len, (int)(len < 32 ? len : 32), s->data());
}

void 
SQLExtract::process(const char* name, u_char* bp, size_t len)
{
    const char* buf = next_field();
    if (buf == NULL) return;
 
    const u_char* v = sql_impl_->unescape_binary((const u_char*)buf);

    memcpy(bp, v, len);
    if (log_) {
        std::string s;
        hex2str(&s, bp, len < 16 ? len : 16);
        logf(log_, LOG_DEBUG, "<=bufc(%d: '%.*s')",
             len, (int)s.length(), s.data());
    }

}


void 
SQLExtract::process(const char* name, u_char** bp, size_t* lenp, bool alloc_copy)
{
    NOTIMPLEMENTED;
}

} // namespace oasys
