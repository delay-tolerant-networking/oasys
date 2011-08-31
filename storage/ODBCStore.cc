/*
 *    Copyright 2004-2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

//##########################################################
//########## README on configuration/setup #################
//##########################################################
//
//  This module is a vanilla ODBC implementation (cloned from BerkeleyDBStore.cc)
//  as a DB accessor within OASYS.  It currently uses a SQLite RDBMS but can 
//  be switch to any ODBC-compliant RDBMS (see "TODO:  NONSTANDARD ODBCR" for any
//  logic that is currently SQLite specific and would need to be modified to comply
//  with the new RDBMS.
//
//  Required Software on Linux box to run: (tested on SOSCOE SDE 10.7.1 machine)
//   unixODBC  ("rpm -q unixODBC" ==> unixODBC-2.2.11-7.1)
//   SQLite    ("rpm -q sqlite"   ==> sqlite-3.3.6-2)
//   SQLite ODBC libs (libsqlite3odbc-0.83.so - download from http://www.ch-werner.de/sqliteodbc/)
//    ## ODBC Driver already in SOSCOE /vobs/fcsc_dataStore/src/COTS/sqlitelibs/i686-rhel-5.3-se/sqlite-3.4.2/lib/libsqlite3odbc-0.86.so
//
//  NOTE: built on DTN/OASYS libs of 2010/12/17 for SOSCOE w/ customized API
//
//  Required OASYS storage files (../oasys_20101217_for_SOSCOE/storage/):
//    ODBCDBStore.h
//    ODBCDBStore.cc   NOTE: need to modify init() to also CREATE TABLE for all VRL router schema tables!!!
//    SQLite_DTN_and_VRL_schema.ddl   NOTE: need to add CREATE TABLE for all VRL router schema tables!!!
//
//  Modified OASYS storage files (../oasys_20101217_for_SOSCOE/storage/):
//    DurableStore.cc
//      #include "ODBCDBStore.h"
//  
//      else if (config.type_ == "sqlitedb")
//      {
//          impl_ = new ODBCDBStore(logpath_);
//          log_debug("impl_ set to new ODBCDBStore");
//      }
//
//  Required OASYS test files (../oasys_20101217_for_SOSCOE/storage/):
//     sqlite-db-test.cc
//
//  Modified OASYS test files (../oasys_20101217_for_SOSCOE/storage/):
//     Makefile:  add "sqlite-db-test" to TESTS list
//
//  Modified OASYS files (../oasys_20101217_for_SOSCOE/):
//     Sources.mk:  add "storage/ODBCDBStore.cc" to STORAGE_SRCS list
//     Rules.make:  add " -lodbc" to end of OASYS_LDFLAGS and OASYS_LDFLAGS_STATIC
//     Makefile:    add "storage/ODBCDBStore.cc" to STORAGE_SRCS list
//     
//  Modifications to ODBC configuration files (requires root access):
//    $ODBCSYSINI/odbcinst.ini:  add new section if SQLite ODBC lib *.so not already existing
//    NOTE: ODBC Driver should already be in SOSCOE odbcinst.ini
//  
//      [SQLite]
//      Description = SQLite ODBC Driver
//      Driver          = /home0/groberts/SQLiteODBC/libsqlite3odbc-0.83.so
//      Setup           = /home0/groberts/SQLiteODBC/libsqlite3odbc-0.83.so
//      UsageCount  = 1
//  
//    $ODBCSYSINI/odbc.ini:  add new sections for OASYS test DB
//
//      [test]
//      Description     = SQLite
//      Driver          = SQLite
//      Database        = <fullPath to OASYS 10201217 dir tree>/test/output/sqlite-db-test/sqlite-db-test/test
//      Timeout         = 100000
//      StepAPI         = No
//      NoWCHAR         = No
//      LongNames       = No
//
//    $ODBCSYSINI/odbc.ini:  add new sections for DTN test/prod DB
//
//      [<config DB name for DTN DB>]
//      Description     = SQLite
//      Driver          = SQLite
//      Database        = <fullPath to DTN 10201217 config DB directory>/test/output/sqlite-db-test/sqlite-db-test/test
//      Timeout         = 100000
//      StepAPI         = No
//      NoWCHAR         = No
//
//  Creation of SQLite DB for DTN production: 
//  NOTE: should disable or not use configuration tidy/prune or any dynamic DB creation due to data loss!!
//
//  
//##########################################################


#ifdef HAVE_CONFIG_H
#include <oasys-config.h>
#endif

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include <debug/DebugUtils.h>
#include <io/FileUtils.h>

#include <util/StringBuffer.h>
#include <util/Pointers.h>
#include <util/ScratchBuffer.h>

#include <serialize/MarshalSerialize.h>
#include <serialize/TypeShims.h>

#include "ODBCStore.h"
#include "StorageConfig.h"
#include "util/InitSequencer.h"

#if LIBODBC_ENABLED

#define NO_TX  0                // for easily going back and changing TX id's later

namespace oasys
{
/******************************************************************************
 *
 * ODBCDBStore
 *
 *****************************************************************************/
const
    std::string
ODBCDBStore::META_TABLE_NAME("META_DATA_TABLES");
//----------------------------------------------------------------------------
ODBCDBStore::ODBCDBStore(const char *logpath):
DurableStoreImpl("ODBCDBStore", logpath),
init_(false),
auto_commit_(true),
serializeAll(true)
{
    logpath_appendf("/ODBCDBStore");
    log_debug("constructor enter/exit.");
}

//----------------------------------------------------------------------------
ODBCDBStore::~ODBCDBStore()
{
    log_debug("destructor enter.");
    StringBuffer err_str;

    err_str.append("Tables still open at deletion time: ");
    bool busy = false;

    for (RefCountMap::iterator iter = ref_count_.begin();
         iter != ref_count_.end(); ++iter)
    {
        if (iter->second != 0)
        {
            err_str.appendf("%s ", iter->first.c_str());
            busy = true;
        }
    }

    if (busy)
    {
        log_err("%s", err_str.c_str());
    }

    if (deadlock_timer_)
    {
        deadlock_timer_->cancel();
    }
    endTransaction(NULL, true);

    SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->trans_hstmt);
    SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->hstmt);
    SQLDisconnect(dbenv_->m_hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);

    dbenv_ = 0;
    log_info("db closed");
    log_debug("destructor exit.");
}

//----------------------------------------------------------------------------

int
ODBCDBStore::beginTransaction(void **txid)
{
	if (!auto_commit_) {
		log_debug("beginTransaction -- AUTOCOMMIT is OFF, returning");
	} else {
	    log_debug("ODBCDBStore::beginTransaction enter.");
	}

    SQLRETURN ret;

    ret = SQLFreeStmt(dbenv_->trans_hstmt, SQL_CLOSE);  //close from any prior use
    if (!SQL_SUCCEEDED(ret))
    {
        log_crit("ERROR:  beginTransaction - failed Statement Handle SQL_CLOSE");        //glr
        return(DS_ERR);
    }

    ret =
        SQLExecDirect(dbenv_->trans_hstmt,
                      (SQLCHAR *) "BEGIN TRANSACTION",
                      SQL_NTS);
    if (!SQL_SUCCEEDED(sqlRC) && (ret != SQL_NO_DATA))
    {
        log_crit("ERROR(%d):  beginTransaction - unable to SQLExecDirect 'BEGIN TRANSACTION'", ret);      //glr
        return (DS_ERR);
    }

    log_debug("ODBCDBStore::beginTransaction exit.");
    if ( txid!=NULL ) {
        *txid = (void*) dbenv_->m_hdbc;
    }
    return 0;
}

int
ODBCDBStore::endTransaction(void *txid, bool be_durable)
{
    (void) txid;

    if (be_durable)
    {
        SQLRETURN ret;

        log_debug("endTransaction enter durable section");

        ret = SQLEndTran(SQL_HANDLE_DBC, dbenv_->m_hdbc, SQL_COMMIT);

        if (!SQL_SUCCEEDED(sqlRC) && (ret != SQL_NO_DATA))
        {
            log_crit("ERROR:  endTransaction – SQLEndTran failed");       //glr
            return(DS_ERR);
        }
    }

    return 0;
}

void *
ODBCDBStore::getUnderlying()
{
    log_debug("getUnderlying enter.");
    return ((void *) dbenv_);
}

int
ODBCDBStore::parseOdbcIni(const char *dbName, char *fullPath, char*schemaPath)
{
    FILE *f;
    char odbciniFilename[1000];
    char *oneLine;
    char *tok;
    size_t lineLen;

    char odbcDatabaseIdentifier[500];
    char section_start = '[';
    char section_end = ']';
    bool foundDatabase = false;

    sprintf(odbcDatabaseIdentifier, "%c%s%c", section_start, dbName, section_end);
    fullPath[0] = '\0';
    schemaPath[0] = '\0';

    if ( getenv("ODBCSYSINI")==NULL ) {
        sprintf(odbciniFilename, "/etc/%s", "odbc.ini");
    } else {
        sprintf(odbciniFilename, "%s/%s", getenv("ODBCSYSINI"), "odbc.ini");
    }

    f = fopen(odbciniFilename, "r");
    if (f == NULL)
    {
        log_debug("Can't open odbc.ini file (%s) for reading", odbciniFilename);
        return (-1);
    }

    oneLine = (char *) malloc(1000);
    while (getline(&oneLine, &lineLen, f) > 0)
    {
        tok = strtok(oneLine, " \t\n");
        if ( tok==NULL ) {
        	// Blank line after we've found a database gets us out.
        	if (foundDatabase) {
        		break;
        	}
        	continue;
        }

        if ( !foundDatabase && (strstr(tok, odbcDatabaseIdentifier)!=NULL) )
        {
            log_debug("Found Database %s", odbcDatabaseIdentifier);
            foundDatabase = true;
            continue;
        } else if ( foundDatabase && strcmp(tok, "Database")==0) {
            // Call strtok twice more, once to get the '=', then once
            // to get the database string.
            tok = strtok(NULL, " \t\n");
            tok = strtok(NULL, " \t\n");
            log_debug("Found database (%s) and path(%s) in odbc.ini file (%s).",
                      dbName, tok, odbciniFilename);
            strcpy(fullPath, tok);
		} else if ( foundDatabase && strcmp(tok, "SchemaCreationFile")==0) {
			// Call strtok twice more, once to get the '=', then once
			// to get the database schema creation file name.
			tok = strtok(NULL, " \t\n");
			tok = strtok(NULL, " \t\n");
			log_debug("Found schema creation file (%s) for database (%s) in odbc.ini file (%s).",
					  tok, dbName, odbciniFilename);
			strcpy(schemaPath, tok);
		}
    }
    free(oneLine);
    fclose(f);

    if (foundDatabase) {
    	return(0);
    }

    return (-1);
}

int
ODBCDBStore::init(const StorageConfig & cfg)
{
    int ret;

    log_debug("init entry.");
    dbenv_ = &base_dbenv_;      //glr

    std::string dbdir = cfg.dbdir_;
    //FileUtils::abspath(&dbdir);

    db_name_ = cfg.dbname_;
    sharefile_ = cfg.db_sharefile_;

    char database_fullpath[500] = "";
    char database_dir[500] = "";
    char database_schema_fullpath[500] = "";

    // For ODBC, the cfg.dbname_ refers to the name of the database as defined
    // in the odbc.ini file (i.e. the name inside square brackets, e.g. [DTN]).
    // This code parses the odbc.ini file looking for the given database name,
    // then uses the 'Database =' entry from the odbc.ini file to determine
    // the location of the actual database file.
    //
    // Note that for odbc-based storage methods, the cfg.dbdir_ variable IS
    // IGNORED

    // Parse the odbc.ini file to find the database name; copy the full path to
    // the database file into database_fullpath
    //sprintf(sys_command, "%s%s%s",
    //        section_start, cfg.dbname_.c_str(), section_end);
    ret = parseOdbcIni(cfg.dbname_.c_str(), database_fullpath, database_schema_fullpath);

    if ( ret!=0 ) {
        log_crit
            ("ODBCDBStore::init can't find DTN2 database SECTION ('[%s]') in odbcini file (%s/%s)",
             cfg.dbname_.c_str(), getenv("ODBCSYSINI"), "odbc.ini");
        return DS_ERR;
    }
    char *c;
    strcpy(database_dir, database_fullpath);
    c = rindex(database_dir, '/');
    *c = '\0';
    dbdir = database_dir;

    // XXX/bowei need to expose options if needed later
    if (cfg.tidy_)
    {
        log_crit("init WARNING:  tidy/prune DB dir %s will delete all DB files, i.e., all DTN and VRL DB tables/data",  //glr
                 dbdir.c_str());
        prune_db_dir(dbdir.c_str(), cfg.tidy_wait_);
    }

    bool force_schema_creation = false;
#if 1
    bool db_dir_exists;
    int err = check_db_dir(dbdir.c_str(), &db_dir_exists);
    if (err != 0)
    {
        log_crit("init  ERROR:  CHECK failed on DB dir %s so exit!",     //glr
                 dbdir.c_str());
        return DS_ERR;
    }
    if (!db_dir_exists)
    {
        log_crit("init  WARNING:  DB dir %s does not exist SO WILL CREATE which means all prior data has been lost!",    //glr
                 dbdir.c_str());
        if (cfg.init_)
        {
            if (create_db_dir(dbdir.c_str()) != 0)
            {
                return DS_ERR;
            }
            force_schema_creation = true;
        } else {
            log_crit
                ("init  DB dir %s does not exist and not told to create!",
                 dbdir.c_str());
            return DS_ERR;
        }
    }
#endif
//glr --- added

    log_debug("init - Verify Env info for DB Alias=%s\n", db_name_.c_str());

	// ODBC automatically creates DB file on SQLConnect so check file first
	if (1)
	{
		struct stat statBuf;
		log_info
			("init - about to check if ODBC DB file %s exists and is non-empty.\n",
			 database_fullpath);
		if (stat(database_fullpath, &statBuf) == 0)
		{
			if (statBuf.st_size == 0)
			{
				log_crit
					("init - ODBC DB file %s exists but IS EMPTY so continue and CREATE DB SCHEMA",
					 database_fullpath);
				force_schema_creation = true;
			} else {
				log_info
					("init - ODBC DB file is ready for access");
			}
		} else {
			log_info
				("init - ODBC DB file %s DOES NOT EXIST or does not permit access so continue and CREATE DB SCHEMA",
				 database_fullpath);
			force_schema_creation = true;
		}
	}

    log_info("init initializing db name=%s (%s), dir=%s",
             db_name_.c_str(), sharefile_ ? "shared" : "not shared",
             dbdir.c_str());

    dbenv_->m_henv = SQL_NULL_HENV;
    dbenv_->m_hdbc = SQL_NULL_HDBC;

    // Allocate the ODBC environment handle for SQL
    if ((sqlRC =
         SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE,
                        &(base_dbenv_.m_henv))) != SQL_SUCCESS
        && sqlRC != SQL_SUCCESS_WITH_INFO)
    {
        log_crit
            ("init ERROR: Failed to allocate environment handle");
        return DS_ERR;
    }
    // set ODBC environment: ODBC version
    if ((sqlRC =
         SQLSetEnvAttr(dbenv_->m_henv, SQL_ATTR_ODBC_VERSION,
                       (void *) SQL_OV_ODBC3, 0)) != SQL_SUCCESS
        && sqlRC != SQL_SUCCESS_WITH_INFO)
    {
        log_crit("ERROR: Failed to set OBDC Version for data server");
        SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
        return DS_ERR;
    }
    log_debug("init - Set ODBC Version success");

    // allocate the ODBC Connection handle
    if ((sqlRC =
         SQLAllocHandle(SQL_HANDLE_DBC, dbenv_->m_henv,
                        &(base_dbenv_.m_hdbc))) != SQL_SUCCESS
        && sqlRC != SQL_SUCCESS_WITH_INFO)
    {
        log_crit
            ("init ERROR: Failed to allocate ODBC Connection handle");
        SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
        return DS_ERR;
    }

    if (dbenv_->m_hdbc == SQL_NULL_HDBC)
    {
        log_crit
            ("init ERROR: Allocated ODBC Connection Handle is null");
        return DS_ERR;
    }
    log_info
        ("init Allocated ODBC Connection Handle successully");

    // set ODBC Connection attributes: login timeout
     if ((sqlRC =
          SQLSetConnectAttr(dbenv_->m_hdbc, SQL_ATTR_LOGIN_TIMEOUT,
                            (SQLPOINTER) 10,
                            SQL_IS_UINTEGER)) != SQL_SUCCESS
         && sqlRC != SQL_SUCCESS_WITH_INFO)
     {
         log_crit("init ERROR: Failed to set DB Connect timeout");
         SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
         SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
         return DS_ERR;
     }
     log_info("init Set DB Connection timeout success");

     // set ODBC Connection attributes: auto-commit
     // if max_nondurable_transactions is >0, auto_commit is OFF;
     if (cfg.auto_commit_==false) {
    	 auto_commit_ = false;
         if ((sqlRC =
               SQLSetConnectAttr(dbenv_->m_hdbc,
                                 SQL_ATTR_AUTOCOMMIT,
                                 SQL_AUTOCOMMIT_OFF,
                                 SQL_IS_UINTEGER)) != SQL_SUCCESS
              && sqlRC != SQL_SUCCESS_WITH_INFO)
          {
              log_crit("init ERROR: Failed to set DB auto-commit");
              SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
              SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
              return DS_ERR;
          }
          log_info("init Set DB auto-commit success max_nondurable_transactions(%d)",
        		   cfg.max_nondurable_transactions_);
     }

    sqlRC = SQLConnect(dbenv_->m_hdbc,
                       (SQLCHAR *) db_name_.c_str(),
                       SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS);
    if (!SQL_SUCCEEDED(sqlRC))
    {
        log_crit
            ("init Failed to SQLConnect to <%s> with NULL login/pswd",
             db_name_.c_str());
        SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
        return DS_ERR;
    }
    log_debug
        ("init Connect to DB <%s> success w/NULL login/pswd (per odbc.ini and odbcinst.ini)",
         db_name_.c_str());

    log_info
        ("init For ODBC, AutoCommit is always ON until suspended by 'BEGIN TRANSACTION'");

    dbenv_->hstmt = SQL_NULL_HSTMT;
    if ((sqlRC =
         SQLAllocHandle(SQL_HANDLE_STMT, dbenv_->m_hdbc,
                        &(dbenv_->hstmt))) != SQL_SUCCESS
        && (sqlRC != SQL_SUCCESS_WITH_INFO))
    {
        log_crit
            ("init ERROR: Failed to allocate Statement handle");
        SQLDisconnect(dbenv_->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
        return DS_ERR;
    }

    if (dbenv_->hstmt == SQL_NULL_HSTMT)
    {
        log_crit
            ("init ERROR: Statement handle is null so skip statement");
        SQLDisconnect(dbenv_->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
        return DS_ERR;
    }

    dbenv_->trans_hstmt = SQL_NULL_HSTMT;
    if ((sqlRC =
         SQLAllocHandle(SQL_HANDLE_STMT, dbenv_->m_hdbc,
                        &(dbenv_->trans_hstmt))) != SQL_SUCCESS
        && (sqlRC != SQL_SUCCESS_WITH_INFO))
    {
        log_crit
            ("init ERROR: Failed to allocate Trans Statement handle");
        SQLDisconnect(dbenv_->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
        return DS_ERR;
    }

    if (dbenv_->trans_hstmt == SQL_NULL_HSTMT)
    {
        log_crit
            ("init ERROR: Trans Statement handle is null so skip statement");
        //SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->hstmt);
        SQLDisconnect(dbenv_->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
        return DS_ERR;
    }

    if (force_schema_creation)
    {
        log_crit
            ("init WARNING:  new DB dir %s SO MUST CREATE new DB Schema for DTN (globals, bundles & registrations; links & prophet unused/uncreated) and VRL",
             dbdir.c_str());

        if ( strlen(database_schema_fullpath)>0 ) {
        	char schema_creation_string[1024];
            log_info
                ("init: sourcing odbc schema creation file (%s) in addition to regular DTN tables",
                 database_schema_fullpath);
            sprintf(schema_creation_string, "echo '.read %s' | sqlite3 %s",
        		    database_schema_fullpath, database_fullpath);

            system(schema_creation_string);
        }

        /* Execute the query to create each table - in ODBC BLOB has datatype affinity NONE so any datatype may be stored */
        sqlRC =
            SQLExecDirect(dbenv_->hstmt,
                          (SQLCHAR *)
                          "CREATE TABLE globals(the_key varchar(100) primary key, the_data blob(1000000))",
                          SQL_NTS);
        if (!SQL_SUCCEEDED(sqlRC) && sqlRC != SQL_NO_DATA)
        {
            log_crit("init ERROR: - unable to SQLExecDirect 'CREATE TABLE globals (%d)", sqlRC); //glr
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->trans_hstmt);
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->hstmt);
            SQLDisconnect(dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
            return DS_ERR;
        }
        sqlRC =
            SQLExecDirect(dbenv_->hstmt,
                          (SQLCHAR *)
                          "CREATE TABLE bundles(the_key unsigned integer primary key, the_data blob(1000000))",
                          SQL_NTS);
        if (!SQL_SUCCEEDED(sqlRC) && sqlRC != SQL_NO_DATA)
        {
            log_crit("ERROR:  init - unable to SQLExecDirect 'CREATE TABLE bundles (%d)", sqlRC);        //glr
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->trans_hstmt);
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->hstmt);
            SQLDisconnect(dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
            return DS_ERR;
        }
        sqlRC =
            SQLExecDirect(dbenv_->hstmt,
                          (SQLCHAR *)
                          "CREATE TABLE prophet(the_key unsigned integer primary key, the_data blob(1000000))",
                          SQL_NTS);
        // if ( !SQL_SUCCEEDED(sqlRC) )
        if (!SQL_SUCCEEDED(sqlRC) && sqlRC != SQL_NO_DATA)
        {
            log_crit("ERROR:  init - unable to SQLExecDirect 'CREATE TABLE prophet(%d)", sqlRC); //glr
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->trans_hstmt);
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->hstmt);
            SQLDisconnect(dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
            return DS_ERR;
        }
        sqlRC =
            SQLExecDirect(dbenv_->hstmt,
                          (SQLCHAR *)
                          "CREATE TABLE links(the_key unsigned integer primary key, the_data blob(1000000))",
                          SQL_NTS);
        // if ( !SQL_SUCCEEDED(sqlRC) )
        if (!SQL_SUCCEEDED(sqlRC) && sqlRC != SQL_NO_DATA)
        {
            log_crit("ERROR:  init - unable to SQLExecDirect 'CREATE TABLE links(%d)", sqlRC);   //glr
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->trans_hstmt);
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->hstmt);
            SQLDisconnect(dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
            return DS_ERR;
        }

        /* Execute the query to create META_DATA table */
        sqlRC =
            SQLExecDirect(dbenv_->hstmt,
                          (SQLCHAR *)
                          "CREATE TABLE registrations(the_key unsigned integer primary key, the_data blob(1000000))",
                          SQL_NTS);
        // if ( !SQL_SUCCEEDED(sqlRC) )
        if (!SQL_SUCCEEDED(sqlRC) && sqlRC != SQL_NO_DATA)
        {
            log_crit("ERROR:  init - unable to SQLExecDirect 'CREATE TABLE registrations (%d)", sqlRC);  //glr
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->trans_hstmt);
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->hstmt);
            SQLDisconnect(dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
            return DS_ERR;
        }

        /* Execute the query to create META_DATA table */
        char my_SQL_str[500];
        snprintf(my_SQL_str, 500,
                 "CREATE TABLE %s (the_table varchar(128))",
                 META_TABLE_NAME.c_str());
        sqlRC = SQLExecDirect(dbenv_->hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);
        // if ( !SQL_SUCCEEDED(sqlRC) )
        if (!SQL_SUCCEEDED(sqlRC) && sqlRC != SQL_NO_DATA)
        {
            log_crit
                ("init ERROR: - unable to SQLExecDirect 'CREATE TABLE META_TABLE (%d)",
                 sqlRC);
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->trans_hstmt);
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->hstmt);
            SQLDisconnect(dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
            return DS_ERR;
        }

        /* Load the META_DATA table */
        snprintf(my_SQL_str, 500, "INSERT INTO %s values('globals')",
                 META_TABLE_NAME.c_str());
        sqlRC = SQLExecDirect(dbenv_->hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);
        if (!SQL_SUCCEEDED(sqlRC))
        {
            log_crit
                ("init ERROR: - unable to SQLExecDirect 'insert into META_TABLE - globals (%d)",
                 sqlRC);
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->trans_hstmt);
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->hstmt);
            SQLDisconnect(dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
            return DS_ERR;
        }

        snprintf(my_SQL_str, 500, "INSERT INTO %s values('bundles')",
                 META_TABLE_NAME.c_str());
        sqlRC = SQLExecDirect(dbenv_->hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);
        if (!SQL_SUCCEEDED(sqlRC))
        {
            log_crit
                ("init ERROR: - unable to SQLExecDirect 'insert into META_TABLE - bundles (%d)",
                 sqlRC);
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->trans_hstmt);
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->hstmt);
            SQLDisconnect(dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
            return DS_ERR;
        }

        snprintf(my_SQL_str, 500, "INSERT INTO %s values('registrations')",
                 META_TABLE_NAME.c_str());
        sqlRC = SQLExecDirect(dbenv_->hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);
        if (!SQL_SUCCEEDED(sqlRC))
        {
            log_crit
                ("init ERROR: - unable to SQLExecDirect 'insert into META_TABLE - registrations");
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->trans_hstmt);
            SQLFreeHandle(SQL_HANDLE_STMT, dbenv_->hstmt);
            SQLDisconnect(dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, dbenv_->m_hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, dbenv_->m_henv);
            return DS_ERR;
        }

        // Commit these changes to disk.
        endTransaction(NULL, true);
    }

    if (cfg.db_lockdetect_ != 0)
    {
        deadlock_timer_ =
            new DeadlockTimer(logpath_, dbenv_, cfg.db_lockdetect_);
        deadlock_timer_->reschedule();
    } else {
        deadlock_timer_ = NULL;
    }

    init_ = true;

    log_debug("init exit.");
    return 0;
}

//----------------------------------------------------------------------------
int
ODBCDBStore::get_table(DurableTableImpl ** table,
                         const std::string & name,
                         int flags, PrototypeVector & prototypes)
{
    log_debug("get_table entry.");
    (void) prototypes;

    u_int32_t db_flags;

    int db_type = 0;            //glr

    ASSERT(init_);

    db_flags = 0;

    if (flags & DS_CREATE)
    {
        log_debug("get_table DB_CREATE on");

        if (flags & DS_EXCL)
        {
            log_debug("get_table DB_EXCL on");
        }
    }

    strcpy(dbenv_->table_name, name.c_str());   //glr - save current table

    log_debug("get_table check if table exists in schema");
    char my_SQL_str[500];
    snprintf(my_SQL_str, 500, "SELECT count(*) FROM %s", dbenv_->table_name);

    sqlRC = SQLFreeStmt(dbenv_->hstmt, SQL_CLOSE);      //close from any prior use
    if (!SQL_SUCCEEDED(sqlRC))
    {
        log_crit("ERROR:  get_table - failed Statement Handle SQL_CLOSE");       //glr
    }

    sqlRC = SQLExecDirect(dbenv_->hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);
    if (!SQL_SUCCEEDED(sqlRC))
    {
        log_crit
            ("ERROR:  get_table - failed Select count(*)");

        if (flags & DS_CREATE)
        {
            log_debug
                ("get_table DB_CREATE is ON so CREATE w/ default Unsigned Int key and Blob data");
            snprintf(my_SQL_str, 500,
                     "CREATE TABLE %s(the_key unsigned integer primary key, the_data blob(1000000))",
                     dbenv_->table_name);
            sqlRC =
                SQLExecDirect(dbenv_->hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);
            if (!SQL_SUCCEEDED(sqlRC))
            {
                log_crit("ERROR:  get_table - failed CREATE table %s", dbenv_->table_name);      //glr
                return DS_ERR;
            }
            snprintf(my_SQL_str, 500, "INSERT INTO %s values('%s')",
                     META_TABLE_NAME.c_str(), dbenv_->table_name);
            sqlRC =
                SQLExecDirect(dbenv_->hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);
            if (!SQL_SUCCEEDED(sqlRC))
            {
                log_crit("ERROR:  get_table - failed table %s insert into META_DATA", dbenv_->table_name);       //glr
                return DS_ERR;
            }
        } else {
            log_debug
                ("get_table DB_CREATE is OFF so return DS_NOTFOUND");
            return DS_NOTFOUND; //glr - added to match  if (err == ENOENT)
        }
    } else {
        log_debug
            ("get_table - successful Select count(*) on table");
        if (flags & DS_CREATE)
        {
            if (flags & DS_EXCL)
            {
                log_debug
                    ("get_table DB_CREATE and DB_EXCL are ON and TABLE ALREADY EXISTS so return DS_EXISTS");
                return DS_EXISTS;       //glr - added to match  if (err == EEXIST)
            }
        }
    }

    log_debug("get_table -- opened table %s of type %d", name.c_str(), db_type);

    *table =
        new ODBCDBTable(logpath_, this, name, (flags & DS_MULTITYPE),
                          dbenv_, db_type);

    log_debug("get_table exit.");
    return 0;
}

//----------------------------------------------------------------------------
int
ODBCDBStore::del_table(const std::string & name)
{
    log_debug("del_table (%s) enter.", name.c_str());
    char sqlstr[500];

    ASSERT(init_);

    if (ref_count_[name] != 0)
    {
        log_info
            ("del_table - Trying to delete table %s with %d refs still on it",
             name.c_str(), ref_count_[name]);

        return DS_BUSY;
    }

    log_info("del_table DROPPING table %s", name.c_str());

    sqlRC = SQLFreeStmt(dbenv_->hstmt, SQL_CLOSE);      //close from any prior use
    if (!SQL_SUCCEEDED(sqlRC))
    {
        log_crit("ERROR:  del_table - failed Statement Handle SQL_CLOSE");       //glr
    }

    snprintf(sqlstr, 500, "DROP TABLE %s", name.c_str());
    sqlRC = SQLExecDirect(dbenv_->hstmt, (SQLCHAR *) sqlstr, SQL_NTS);
    if (sqlRC == SQL_NO_DATA_FOUND)
    {
        return DS_NOTFOUND;
    } else if (!SQL_SUCCEEDED(sqlRC))
    {
        log_crit("ERROR:  del_table - unable to SQLExecDirect 'DROP TABLE %s'", name.c_str());   //glr
        return DS_ERR;
    }

    ref_count_.erase(name);
    log_debug("del_table exit.");

    return 0;
}

//----------------------------------------------------------------------------
int
ODBCDBStore::get_table_names(StringVector * names)
{
    log_debug
        ("get_table_names enter - list from META_DATA table).");
    SQLRETURN sql_ret;
    char the_table_name[128];

    char my_SQL_str[500];
    snprintf(my_SQL_str, 500, "SELECT the_table FROM %s",
             META_TABLE_NAME.c_str());

    sql_ret = SQLFreeStmt(dbenv_->hstmt, SQL_CLOSE);    //close from any prior use
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_crit("ERROR:  get_table_names - failed Statement Handle SQL_CLOSE"); //glr
    }

    sql_ret = SQLPrepare(dbenv_->hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get_table_names SQLPrepare error %d", sql_ret);
        return DS_ERR;
    }

    sql_ret =
        SQLBindCol(dbenv_->hstmt, 1, SQL_C_DEFAULT, the_table_name, 128, NULL);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get_table_names SQLBindCol error %d", sql_ret);
        return DS_ERR;
    }

    sql_ret = SQLExecute(dbenv_->hstmt);

    if (sql_ret == SQL_NO_DATA_FOUND)
    {
        log_debug("get_table_names SQLExecute NO_DATA_FOUND");
        return DS_NOTFOUND;
    }
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_debug("get_table_names SQLExecute error %d", sql_ret);
        return DS_ERR;
    }

    names->clear();
    while ((sql_ret = SQLFetch(dbenv_->hstmt)) == SQL_SUCCESS)
    {
        log_debug("get_table_names fetched table <%s>",
                  the_table_name);
        names->push_back(std::string(the_table_name, strlen(the_table_name)));
    }
    if (sql_ret == SQL_NO_DATA_FOUND)
    {
        log_debug
            ("get_table_names SQLFetch NO_DATA_FOUND so done");
    } else if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get SQLFetch error %d", sql_ret);
        return DS_ERR;
    }

    sql_ret = SQLFreeStmt(dbenv_->hstmt, SQL_CLOSE);    //close from any prior use
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_crit("ERROR:  get_table_names - failed Statement Handle SQL_CLOSE"); //glr
    }

    log_debug("get_table_names exit.");
    return 0;
}

//----------------------------------------------------------------------------
std::string ODBCDBStore::get_info()const
{
    StringBuffer
        desc;

    return "ODBCDB";
}


//----------------------------------------------------------------------------
int
ODBCDBStore::acquire_table(const std::string & table)
{
    log_debug("acquire_table enter.");
    ASSERT(init_);

    ++ref_count_[table];
    ASSERT(ref_count_[table] >= 0);

    log_debug("acquire_table table %s, +refcount=%d",
              table.c_str(), ref_count_[table]);

    log_debug("acquire_table exit.");
    return ref_count_[table];
}

//----------------------------------------------------------------------------
int
ODBCDBStore::release_table(const std::string & table)
{
    log_debug("release_table (%s) enter.", table.c_str());
    ASSERT(init_);

    --ref_count_[table];
    ASSERT(ref_count_[table] >= 0);

    log_debug("release_table table %s, -refcount=%d",
              table.c_str(), ref_count_[table]);

    log_debug("release_table exit.");
    return ref_count_[table];
}


//----------------------------------------------------------------------------
void
ODBCDBStore::DeadlockTimer::reschedule()
{
    log_debug("DeadlockTimer::rescheduling in %d msecs", frequency_);
    schedule_in(frequency_);
}

//----------------------------------------------------------------------------
void
ODBCDBStore::DeadlockTimer::timeout(const struct timeval &now)
{
    (void) now;
    log_debug
        ("DeadlockTimer::timeout SO reschedule (SHOULD NEVER HAPPEN - ODBC has SQL_BUSY return not lock_detect)");

    reschedule();
}

/******************************************************************************
 *
 * ODBCDBTable
 *
 *****************************************************************************/
ODBCDBTable::ODBCDBTable(const char *logpath,
                             ODBCDBStore * store,
                             const std::string & table_name,
                             bool multitype, ODBC_dbenv * db, int db_type):
DurableTableImpl(table_name, multitype),
Logger("ODBCDBTable", "%s/%s", logpath, table_name.c_str()),
//Logger("ODBCDBTable", "/dtn/storage/ODBCDBTable/"+table_name),
db_(db),
db_type_(db_type),
store_(store)
{
    SQLRETURN sqlRC;

    log_debug("ODBCDBTable constructor for table %s", table_name.c_str());
    //logpath_appendf("/ODBCDBTable/%s", table_name.c_str());
    log_debug("logpath is: <%s>", logpath);
    store_->acquire_table(table_name);

    hstmt = SQL_NULL_HSTMT;
    if ((sqlRC =
         SQLAllocHandle(SQL_HANDLE_STMT, db_->m_hdbc,
                        &hstmt)) != SQL_SUCCESS
        && (sqlRC != SQL_SUCCESS_WITH_INFO))
    {
        log_crit
            ("init ERROR: Failed to allocate Statement handle");
        SQLDisconnect(db->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, db->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, db->m_henv);
    }

    if (hstmt == SQL_NULL_HSTMT)
    {
        log_crit
            ("init ERROR: Statement handle is null so skip statement");
        SQLDisconnect(db->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, db->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, db->m_henv);
    }

    iterator_hstmt = SQL_NULL_HSTMT;
    if ((sqlRC = SQLAllocHandle(SQL_HANDLE_STMT, db->m_hdbc, &(iterator_hstmt))) != SQL_SUCCESS
       && sqlRC != SQL_SUCCESS_WITH_INFO)
    {
        log_crit("init ERROR: Failed to allocate Iterator Statement handle");
        SQLDisconnect(db->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, db->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, db->m_henv);
    }

    if (iterator_hstmt == SQL_NULL_HSTMT)
    {
        log_crit("init ERROR: Iterator Statement handle is null so skip statement");
        //SQLFreeHandle(SQL_HANDLE_STMT, db->hstmt);
        SQLDisconnect(db->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, db->m_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, db->m_henv);
    }
}

//----------------------------------------------------------------------------
ODBCDBTable::~ODBCDBTable()
{
    // Note: If we are to multithread access to the same table, this
    // will have potential concurrency problems, because close can
    // only happen if no other instance of Db is around.
    store_->release_table(name());

    // SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    // SQLFreeHandle(SQL_HANDLE_STMT, iterator_hstmt);

    log_debug("destructor");
}

//----------------------------------------------------------------------------
int
ODBCDBTable::get(const SerializableObject & key, SerializableObject * data)
{
    log_debug("get enter.");
    ASSERTF(!multitype_, "single-type get called for multi-type table");
    if ( store_->serializeAll && !(store_->serialization_lock_.is_locked_by_me()) ) {
        ScopeLock sl(&store_->serialization_lock_, "Access by get()");
    }
    ScopeLock l(&lock_, "Access by get()");

    ScratchBuffer < u_char *, 256 > key_buf;
    size_t key_buf_len = flatten(key, &key_buf);
    ASSERT(key_buf_len != 0);

    __my_dbt k;
    memset(&k, 0, sizeof(k));
    k.data = key_buf.buf();
    k.size = key_buf_len;
    __my_dbt d;
    memset(&d, 0, sizeof(d));

    SQLRETURN sql_ret;
    char *tmp = NULL;
    SQLLEN user_data_size;

    log_debug("get  Table=%s", name());
    char my_SQL_str[500];
    snprintf(my_SQL_str, 500, "SELECT the_data FROM %s WHERE the_key = ?",
             name());

    //sql_ret = SQLFreeStmt(db_->hstmt, SQL_CLOSE);       //close from any prior use
    sql_ret = SQLFreeStmt(hstmt, SQL_CLOSE);       //close from any prior use
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_crit("ERROR:  get - failed Statement Handle SQL_CLOSE");     //glr
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
    }

    sql_ret = SQLPrepare(hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get SQLPrepare error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    if (strcmp(name(), "globals") == 0)
    {
        char myString[100];
        memset(myString, '\0', 100);
        memcpy(myString, ((char *) k.data) + 4, k.size - sizeof(int));
#if 0
        log_debug
            ("get  len=%d, Key+4 as string=<%s> (leading 4-bytes length) plus size=%d",
             (int) k.data, (char *) k.data + 4, k.size);
        sql_ret =
            SQLBindParameter(db_->hstmt, 1, SQL_PARAM_INPUT, SQL_C_DEFAULT,
                             SQL_CHAR, 0, 0, (char *) k.data + 4, 0, NULL);
#else
        log_debug
            ("get  len=%d, Key+4 as string=<%s> (leading 4-bytes length) plus size=%d",
             *((u_int32_t*) k.data), myString, k.size);
        sql_ret =
            SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_DEFAULT,
                             SQL_CHAR, 0, 0, myString, 0, NULL);
#endif
    } else {
        log_debug
            ("get  Key(reverse byte order) unsignedInt=%u int=%d plus size=%d",
             realKey(k.data), *((u_int32_t*) k.data), k.size);
        sql_ret =
            SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG,
                             SQL_INTEGER, 0, 0, k.data, 0, NULL);
    }

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get SQLBindParameter error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    if ((tmp = (char *) malloc(DATA_MAX_SIZE)) == NULL)
    {
        log_err("get malloc(DATA_MAX_SIZE) error");
        return DS_ERR;
    }

    sql_ret =
        SQLBindCol(hstmt, 1, SQL_C_BINARY, tmp, DATA_MAX_SIZE,
                   &user_data_size);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get SQLBindCol error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        free(tmp);
        tmp = NULL;
        return DS_ERR;
    }

    sql_ret = SQLExecute(hstmt);

    if (sql_ret == SQL_NO_DATA_FOUND)
    {
        log_debug("get SQLExecute NO_DATA_FOUND");
        free(tmp);
        tmp = NULL;
        return DS_NOTFOUND;
    }
    switch(sql_ret) {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        break;
    case SQL_NO_DATA:
        log_debug("get SQLExecute returns SQL_NO_DATA after check");
        break;
    default:
        log_debug("get SQLExecute error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        free(tmp);
        tmp = NULL;
        return DS_ERR;
    }

    sql_ret = SQLFetch(hstmt);

    if (sql_ret == SQL_NO_DATA_FOUND)
    {
        log_debug("get SQLFetch NO_DATA_FOUND");
        free(tmp);
        tmp = NULL;
        return DS_NOTFOUND;
    }
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get SQLFetch error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        free(tmp);
        tmp = NULL;
        return DS_ERR;
    }

    if (user_data_size == SQL_NULL_DATA)
    {
        log_err("get SQLFetch SQL_NULL_DATA");
        free(tmp);
        tmp = NULL;
        return DS_ERR;
    }


    d.data = (void *) tmp;
    d.size = user_data_size;
    log_debug("get first 8-bytes of DATA=%x08 plus size=%d",
              *((u_int32_t *) d.data), d.size);

    u_char *bp = (u_char *) d.data;
    size_t sz = d.size;

    Unmarshal unmarshaller(Serialize::CONTEXT_LOCAL, bp, sz);

    if (unmarshaller.action(data) != 0)
    {
        log_err("get: error unserializing data object");
        free(tmp);
        tmp = NULL;
        return DS_ERR;
    }

    free(tmp);
    tmp = NULL;
    user_data_size = 0;
    log_debug("ODBCDBStore::get exit.");
    return 0;
}

//----------------------------------------------------------------------------
int
ODBCDBTable::get(const SerializableObject & key,
                   SerializableObject ** data,
                   TypeCollection::Allocator_t allocator)
{
    log_debug("ODBCDBStore::get2 enter.");
    ASSERTF(multitype_, "multi-type get called for single-type table");
    if ( store_->serializeAll && !store_->serialization_lock_.is_locked_by_me() ) {
        ScopeLock sl(&store_->serialization_lock_, "Access by get()");
    }
    ScopeLock l(&lock_, "Access by get2()");

    ScratchBuffer < u_char *, 256 > key_buf;
    size_t key_buf_len = flatten(key, &key_buf);
    if (key_buf_len == 0)
    {
        log_err("get2 zero or too long key length");
        return DS_ERR;
    }
//glr add (duplicated/copied from get() above)
    __my_dbt k;
    memset(&k, 0, sizeof(k));
    k.data = key_buf.buf();
    k.size = key_buf_len;
    __my_dbt d;
    memset(&d, 0, sizeof(d));

    SQLRETURN sql_ret;
    char *tmp = NULL;
    SQLLEN user_data_size;

    log_debug("get2  Table=%s", name());
    char my_SQL_str[500];
    snprintf(my_SQL_str, 500, "SELECT the_data FROM %s WHERE the_key = ?",
             name());

    sql_ret = SQLFreeStmt(hstmt, SQL_CLOSE);       //close from any prior use
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_crit("ERROR:  ODBCDBStore::get - failed Statement Handle SQL_CLOSE");     //glr
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
    }

    sql_ret = SQLPrepare(hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get2 SQLPrepare error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    if (strcmp(name(), "globals") == 0)
    {
        char myString[100];
        memset(myString, '\0', 100);
        memcpy(myString, ((char *) k.data) + 4, k.size - sizeof(int));
#if 0
        log_debug
            ("get2  len=%d Key+4 as string=<%s> (leading 4-bytes length) plus size=%d",
             (int) k.data, (char *) k.data + 4, k.size);
        sql_ret =
            SQLBindParameter(db_->hstmt, 1, SQL_PARAM_INPUT, SQL_C_DEFAULT,
                             SQL_CHAR, 0, 0, (char *) k.data + 4, 0, NULL);
#else
        log_debug
            ("get2  len=%d Key+4 as string=<%s> (leading 4-bytes length) plus size=%d",
             *((u_int32_t *)k.data), myString, k.size);
        sql_ret =
            SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_DEFAULT,
                             SQL_CHAR, 0, 0, myString, 0, NULL);
#endif
    } else {
        log_debug
            ("get2  Key(reverse byte order) unsignedInt=%u int=%d plus size=%d",
             realKey(k.data), *((u_int32_t *)k.data), k.size);
        sql_ret =
            SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG,
                             SQL_INTEGER, 0, 0, k.data, 0, NULL);
    }

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get2 SQLBindParameter error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    if ((tmp = (char *) malloc(DATA_MAX_SIZE)) == NULL)
    {
        log_err("get2 malloc(DATA_MAX_SIZE) error");
        return DS_ERR;
    }

    sql_ret =
        SQLBindCol(hstmt, 1, SQL_C_BINARY, tmp, DATA_MAX_SIZE,
                   &user_data_size);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get2 SQLBindCol error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        free(tmp);
        tmp = NULL;
        return DS_ERR;
    }

    sql_ret = SQLExecute(hstmt);

    if (sql_ret == SQL_NO_DATA_FOUND)
    {
        log_debug("get2 SQLExecute NO_DATA_FOUND");
        free(tmp);
        tmp = NULL;
        return DS_NOTFOUND;
    }
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get2 SQLExecute error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        free(tmp);
        tmp = NULL;
        return DS_ERR;
    }

    sql_ret = SQLFetch(hstmt);

    if (sql_ret == SQL_NO_DATA_FOUND)
    {
        log_debug("get2 SQLFetch NO_DATA_FOUND");
        free(tmp);
        tmp = NULL;
        return DS_NOTFOUND;
    }
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get2 SQLFetch error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        free(tmp);
        tmp = NULL;
        return DS_ERR;
    }

    if (user_data_size == SQL_NULL_DATA)
    {
        log_err("get2 SQLFetch SQL_NULL_DATA");
        free(tmp);
        tmp = NULL;
        return DS_ERR;
    }


    d.data = (void *) tmp;
    d.size = user_data_size;
    log_debug
        ("get2 first 8-bytes of DATA=%x08 plus size=%d",
         *((u_int32_t *)d.data), d.size);

    u_char *bp = (u_char *) d.data;
    size_t sz = d.size;

    TypeCollection::TypeCode_t typecode;
    size_t typecode_sz = MarshalSize::get_size(&typecode);

    Builder b;
    UIntShim type_shim(b);
    Unmarshal type_unmarshaller(Serialize::CONTEXT_LOCAL, bp, typecode_sz);

    if (type_unmarshaller.action(&type_shim) != 0)
    {
        log_err("ODBCDBStore::get2: error unserializing type code");
        free(tmp);
        tmp = NULL;
        return DS_ERR;
    }

    typecode = type_shim.value();

    bp += typecode_sz;
    sz -= typecode_sz;

    int err = allocator(typecode, data);
    if (err != 0)
    {
        log_err("ODBCDBStore::get2: error in allocator");
        free(tmp);
        tmp = NULL;
        *data = NULL;
        return DS_ERR;
    }

    ASSERT(*data != NULL);

    Unmarshal unmarshaller(Serialize::CONTEXT_LOCAL, bp, sz);

    if (unmarshaller.action(*data) != 0)
    {
        log_err("ODBCDBStore::get2: error unserializing data object");
        delete *data;
        *data = NULL;
        free(tmp);
        tmp = NULL;
        return DS_ERR;
    }

    free(tmp);
    tmp = NULL;
    user_data_size = 0;
    log_debug("ODBCDBStore::get2 exit.");
    return DS_OK;
}

//----------------------------------------------------------------------------
int
ODBCDBTable::realKey(void *p2)
{
    int ret = 0;
    char *p = (char *) p2;
    char *t = (char *) &ret;
    t[0]=p[3]; t[1]=p[2]; t[2]=p[1]; t[3]=p[0];

    return(ret);
}

//----------------------------------------------------------------------------
int
ODBCDBTable::put(const SerializableObject & key,
                   TypeCollection::TypeCode_t typecode,
                   const SerializableObject * data,
                   int flags)
{
    if ( store_->serializeAll && !store_->serialization_lock_.is_locked_by_me() ) {
        ScopeLock sl(&store_->serialization_lock_, "Access by put()");
    }
    ScopeLock l(&lock_, "Access by put()");
    log_debug("put thread(%08X) flags(%02X)",
              (unsigned int) pthread_self(),
              flags);

    ScratchBuffer < u_char *, 256 > key_buf;
    size_t key_buf_len = flatten(key, &key_buf);

    // flatten and fill in the key
    __my_dbt k;
    memset(&k, 0, sizeof(k));
    k.data = key_buf.buf();
    k.size = key_buf_len;

    SQLRETURN sql_ret;
    SQLLEN user_data_size;
    char my_SQL_str[500];
    int insert_sqlcode;

    if ( strcmp(name(), "globals")==0 ) {
        log_debug ("put on globals table");
    } else {
        log_debug
            ("put insert  Key(reverse byte order) unsignedInt=%u int=%d plus size=%d",
             realKey(k.data), *((u_int32_t *)k.data), k.size);
    }
    
    // if the caller does not want to create new entries, first do a
    // db get to see if the key already exists
    if ((flags & DS_CREATE) == 0)
    {
        log_debug
            ("put DS_CREATE is OFF so check PK prior existance");
        //check if PK row already exists
        int err = key_exists(k.data, k.size);
        if (err == DS_NOTFOUND)
        {
            log_debug("put return DS_NOTFOUND per key_exists");
            return DS_NOTFOUND;
        }
        if (err == DS_ERR)
        {
            log_debug("put return DS_ERR per key_exists");
            return DS_ERR;
        }
    } else {
        log_debug
            ("put DS_CREATE is ON so create new entry if needed");
    }                         //end if (DB_CREATE) - check prior PK existance

    // figure out the size of the data
    MarshalSize sizer(Serialize::CONTEXT_LOCAL);
    if (sizer.action(data) != 0)
    {
        log_err("put error sizing data object");
        return DS_ERR;
    }
    size_t object_sz = sizer.size();

    // and the size of the type code (if multitype)
    size_t typecode_sz = 0;
    if (multitype_)
    {
        typecode_sz = MarshalSize::get_size(&typecode);
    }
    // XXX/demmer -- one little optimization would be to pass the
    // calculated size out to the caller (the generic DurableTable),
    // so we don't have to re-calculate it in the object cache code

    log_debug
        ("put serializing %zu byte object (plus %zu byte typecode)",
         object_sz, typecode_sz);

    ScratchBuffer < u_char *, 1024 > scratch;
    u_char *buf = scratch.buf(typecode_sz + object_sz);
    __my_dbt d;
    memset(&d, 0, sizeof(d));
    d.data = buf;
    d.size = typecode_sz + object_sz;

    // if we're a multitype table, marshal the type code
    if (multitype_)
    {
        log_debug("marhaling type code");
        Marshal typemarshal(Serialize::CONTEXT_LOCAL, buf, typecode_sz);
        UIntShim type_shim(typecode);

        if (typemarshal.action(&type_shim) != 0)
        {
            log_err("put error serializing type code");
            return DS_ERR;
        }
    }

    Marshal m(Serialize::CONTEXT_LOCAL, buf + typecode_sz, object_sz);
    if (m.action(data) != 0)
    {
        log_err("put error serializing data object");
        return DS_ERR;
    }
//glr added - always do insert with NULL the_data blob (followed by update - easier since PUT() supports both)
    log_debug("put inserting NULL the_data blob into table row");
    snprintf(my_SQL_str, 500, "INSERT INTO %s values(?, NULL)", name());

    sql_ret = SQLFreeStmt(hstmt, SQL_CLOSE);       //close for later reuse
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_crit("ERROR:  ::put insert - failed Statement Handle SQL_CLOSE");      //glr
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
    }

    sql_ret = SQLPrepare(hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("put insert SQLPrepare error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    if (strcmp(name(), "globals") == 0)
    {
        char myString[100];
        memset(myString, '\0', 100);
        memcpy(myString, ((char *) k.data) + 4, k.size - sizeof(int));
#if 0
        log_debug
            ("put insert  Key(len and string) len=%d string=%s plus size=%d",
             (int) k.data, (char *) k.data + 4, k.size);
        sql_ret =
            SQLBindParameter(db_->hstmt, 1, SQL_PARAM_INPUT, SQL_C_DEFAULT,
                             SQL_CHAR, 0, 0, (char *) k.data + 4, 0, NULL);
#else
        log_debug
            ("put insert  Key(len and string) len=%d string=%s plus size=%d",
             *((u_int32_t *)k.data), myString, k.size);
        sql_ret =
            SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_DEFAULT,
                             SQL_CHAR, 0, 0, myString, 0, NULL);
#endif
    } else {
        log_debug
            ("put insert  Key(reverse byte order) unsignedInt=%u int=%d plus size=%d",
             realKey(k.data), *((u_int32_t *)k.data), k.size);
        sql_ret =
            SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG,
                             SQL_INTEGER, 0, 0, k.data, 0, NULL);
    }

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("put insert SQLBindParameter error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    insert_sqlcode = 0;
    sql_ret = SQLExecute(hstmt);

    if (!SQL_SUCCEEDED(sql_ret))
    {                           //need to capture internal SQLCODE from SQLError for processing Duplicate PK
        log_debug("put insert SQLExecute error %d", sql_ret);
        insert_sqlcode = print_error(db_->m_henv, db_->m_hdbc, hstmt);
    } else {
        log_debug("put insert SQLExecute succeeded");
    }

//TODO:  NONSTANDARD ODBC - would need to be changed to specific RDBMS handling of duplicate PK on INSERT
    if ((sql_ret == SQLITE_DB_CONSTRAINT) || (insert_sqlcode == 19))    //SQLite returning on Duplicate Key???
    {
        log_debug
            ("put insert SQLExecute CONSTRAINT VIOLATION - duplicate PK");
        if (flags & DS_EXCL)
        {
            log_debug
                ("put  insert - PK exists and DS_EXCL set so return DB_EXISTS");
            return DS_EXISTS;
        }
        log_debug
            ("put  insert PK exists but DS_EXCL is off so CONTINUE w/ update");
    } else if (!SQL_SUCCEEDED(sql_ret))
    {
        return DS_ERR;
    }

//glr added - after insert, now do update of the_data blob (update is easier since PUT() supports both)
    log_debug("put update table row (real data this time)");
    snprintf(my_SQL_str, 500,
             "UPDATE %s SET the_data = ? WHERE the_key = ?", name());

    sql_ret = SQLFreeStmt(hstmt, SQL_CLOSE);       //close for later reuse
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_crit("ERROR:  ODBCDBStore::put update - failed Statement Handle SQL_CLOSE");      //glr
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
    }

    sql_ret = SQLPrepare(hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("put update SQLPrepare error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    user_data_size = d.size;
    log_debug
        ("put update first 8-bytes of DATA=%x08 plus size=%d",
         *((u_int32_t* ) d.data), d.size);
    sql_ret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, //glr SQL_BLOB,
                               0, 0, d.data, 0, &user_data_size);
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err
            ("put update SQLBindParameter DATA  error %d",
             sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    if (strcmp(name(), "globals") == 0)
    {
        char myString[100];
        memset(myString, '\0', 100);
        memcpy(myString, ((char *) k.data) + 4, k.size - sizeof(int));
        sql_ret =
            SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_DEFAULT,
                             SQL_CHAR, 0, 0, (char *) myString, 0, NULL);
    } else
    {
        sql_ret =
            SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG,
                             SQL_INTEGER, 0, 0, k.data, 0, NULL);
    }
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("put update SQLBindParameter PK  error %d",
                sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    sql_ret = SQLExecute(hstmt);

    switch (sql_ret)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        break;
    case SQL_NO_DATA:
        log_debug
            ("put update SQLExecute error %d (SQL_NO_DATA) - ignorning",
             sql_ret);
        break;
    default:
        log_debug("put update SQLExecute error %d", sql_ret);
        log_err("Note: SQL_SUCCESS_WITH_INFO is: %d", SQL_SUCCESS_WITH_INFO);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    log_debug("put exit thread(%08X)", (u_int32_t) pthread_self());
    return 0;
}

//----------------------------------------------------------------------------
int
ODBCDBTable::del(const SerializableObject & key)
{
    ScopeLock l(&lock_, "Access by del()");
    log_debug("del enter thread(%08X).",(u_int32_t) pthread_self());
    u_char key_buf[256];
    size_t key_buf_len;

    key_buf_len = flatten(key, key_buf, 256);
    if (key_buf_len == 0)
    {
        log_err("del - zero or too long key length");
        return DS_ERR;
    }
//glr add (duplicated/copied from get() above)
    __my_dbt k;
    memset(&k, 0, sizeof(k));
    k.data = key_buf;
    k.size = key_buf_len;
    __my_dbt d;
    memset(&d, 0, sizeof(d));

//check if PK row already exists
    int err = key_exists(k.data, k.size);
    if (err == DS_NOTFOUND)
    {
        log_debug("::del return NO_DATA_FOUND per key_exists");
        return DS_NOTFOUND;
    }

    SQLRETURN sql_ret;

    log_debug("del");
    char my_SQL_str[500];
    snprintf(my_SQL_str, 500, "DELETE FROM %s WHERE the_key = ?", name());

    sql_ret = SQLFreeStmt(hstmt, SQL_CLOSE);       //close from any prior use
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_crit("ERROR:  del - failed Statement Handle SQL_CLOSE");     //glr
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
    }

    sql_ret = SQLPrepare(hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("del SQLPrepare error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    if (strcmp(name(), "globals") == 0)
    {
        char myString[100];
        memset(myString, '\0', 100);
        memcpy(myString, ((char *) k.data) + 4, k.size - sizeof(int));
#if 0
        log_debug
            ("del  len= %d, Key+4 as string=<%s> (leading 4-bytes length) plus size=%d",
             (int) k.data, (char *) k.data + 4, k.size);
        sql_ret =
            SQLBindParameter(db_->hstmt, 1, SQL_PARAM_INPUT, SQL_C_DEFAULT,
                             SQL_CHAR, 0, 0, (char *) k.data + 4, 0, NULL);
#else
        log_debug
            ("del  len= %d, Key+4 as string=<%s> (leading 4-bytes length) plus size=%d",
             *((u_int32_t *)k.data), myString, k.size);
        sql_ret =
            SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_DEFAULT,
                             SQL_CHAR, 0, 0, myString, 0, NULL);
#endif
    } else
    {
        log_debug
            ("del  Key(reverse byte order) unsignedInt=%u int=%d plus size=%d",
             realKey(k.data), *((u_int32_t *)k.data), k.size);
        sql_ret =
            SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG,
                             SQL_INTEGER, 0, 0, k.data, 0, NULL);
    }

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("del SQLBindParameter error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    sql_ret = SQLExecute(hstmt);

    if (sql_ret == SQL_NO_DATA_FOUND)
    {
        log_debug
            ("del SQLExecute NO_DATA_FOUND (SQLite does not indicate NDF)");
        return DS_NOTFOUND;
    }
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_debug("del SQLExecute error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    log_debug("del exit thread (%08X)", (u_int32_t) pthread_self());
    return 0;
}

//----------------------------------------------------------------------------
size_t
ODBCDBTable::size() const
{
    ScopeLock l(&lock_, "Access by size()");
    log_debug("size enter thread(%08X)", (u_int32_t) pthread_self());
    SQLRETURN sql_ret;
    SQLINTEGER my_count;   //long int
    size_t ret;

    log_debug("size  Table=%s", name());
    char my_SQL_str[500];
    snprintf(my_SQL_str, 500, "SELECT count(*) FROM %s", name());

    sql_ret = SQLFreeStmt(hstmt, SQL_CLOSE);        //close from any prior use
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_crit("ERROR:  ODBCDBStore::size - failed Statement Handle SQL_CLOSE");       //glr
    }

    sql_ret = SQLPrepare(hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("size SQLPrepare error %d", sql_ret);
        return DS_ERR;
    }

    sql_ret = SQLBindCol(hstmt, 1, SQL_C_SLONG, &my_count, 0, NULL);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("size SQLBindCol error %d", sql_ret);
        return DS_ERR;
    }

    sql_ret = SQLExecute(hstmt);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("size SQLExecute error %d", sql_ret);
        return DS_ERR;
    }

    sql_ret = SQLFetch(hstmt);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("get SQLFetch error %d", sql_ret);
        return DS_ERR;
    }

    ret = (size_t) my_count;
    log_debug ("size exit thread(%08X) w/ret=%d my_count(long int)=%ld  my_count(int)=%d.",
               (u_int32_t) pthread_self(), (int) ret, (long int) my_count, (int) my_count);
    return ret;
}

//----------------------------------------------------------------------------
DurableIterator *
ODBCDBTable::itr()
{
    return new ODBCDBIterator(this);
}

//----------------------------------------------------------------------------
int
ODBCDBTable::key_exists(const void *key, size_t key_len)
{
    log_debug("key_exists enter thread(%08X)",(u_int32_t) pthread_self());

//glr add - duplicate/copy from get()
    __my_dbt k;
    memset(&k, 0, sizeof(k));
    k.data = const_cast < void *>(key);
    k.size = key_len;

    SQLRETURN sql_ret;
    SQLINTEGER my_count;        //long int

    log_debug("key_exists.");
    char my_SQL_str[500];
    snprintf(my_SQL_str, 500, "SELECT count(*) FROM %s WHERE the_key = ?",
             name());

    sql_ret = SQLFreeStmt(hstmt, SQL_CLOSE);       //close from any prior use
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_crit("ERROR:  ODBCDBStore::get - failed Statement Handle SQL_CLOSE");     //glr
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
    }

    sql_ret = SQLPrepare(hstmt, (SQLCHAR *) my_SQL_str, SQL_NTS);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("key_exists SQLPrepare error %d",
                sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    if (strcmp(name(), "globals") == 0)
    {
        char myString[100];
        memset(myString, '\0', 100);
        memcpy(myString, ((char *) k.data) + 4, k.size - sizeof(int));
#if 0
        log_debug
            ("key_exists  len=%d, Key+4 as string=<%s> (leading 4-bytes length) plus size=%d",
             (int) k.data, (char *) k.data + 4, k.size);
        sql_ret =
            SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_DEFAULT,
                             SQL_CHAR, 0, 0, (char *) k.data + 4, 0, NULL);
#else
        log_debug
            ("key_exists  len=%d, Key+4 as string=<%s> (leading 4-bytes length) plus size=%d",
             *((u_int32_t*) k.data), myString, k.size);
        sql_ret =
            SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_DEFAULT,
                             SQL_CHAR, 0, 0, myString, 0, NULL);
#endif
    } else
    {
        log_debug
            ("key_exists  Key(reverse byte order) unsignedInt=%u int=%d plus size=%d",
             realKey(k.data), *((u_int32_t *)k.data), k.size);
        sql_ret =
            SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG,
                             SQL_INTEGER, 0, 0, k.data, 0, NULL);
    }

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("key_exists SQLBindParameter error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    sql_ret = SQLBindCol(hstmt, 1, SQL_C_SLONG, &my_count, 0, NULL);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("key_exists SQLBindCol error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    sql_ret = SQLExecute(hstmt);

    switch ( sql_ret ) {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        break;
    case SQL_NO_DATA:
        log_debug("key_exists SQLExecute returns SQL_NO_DATA");
        break;
    default:
        log_err("key_exists SQLExecute error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    sql_ret = SQLFetch(hstmt);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("key_exists SQLFetch error %d", sql_ret);
        print_error(db_->m_henv, db_->m_hdbc, hstmt);
        return DS_ERR;
    }

    if (my_count == 0)
    {
        log_debug("key_exists return DS_NOTFOUND.");
        return DS_NOTFOUND;
    }

    log_debug("key_exists exit (return 0 for FOUND) thread(%08X)",
              (u_int32_t) pthread_self());
    return 0;
}

/******************************************************************************
 *
 * ODBCDBIterator
 *
 *****************************************************************************/
ODBCDBIterator::ODBCDBIterator(ODBCDBTable* t):
    Logger("ODBCDBIterator", "%s/iter", t->logpath()),
    cur_(0),
    valid_(false)
{
    myTable = t;

    logpath_appendf("ODBCDBIterator/%s", t->name());
    myTable->iterator_lock_.lock("Iterator");
    log_debug("constructor enter.");
    if ( t->store_->serializeAll ) {
        t->store_->serialization_lock_.lock("Access by get()");
    }

    SQLRETURN sql_ret;
    //cur_ = t->db_->iterator_hstmt;
    cur_ = t->iterator_hstmt;
    strcpy(cur_table_name, t->name());

    log_debug
        ("constructor SELECT the_key,the_data FROM %s (unqualified)", t->name());

    char my_SQL_str[500];
    snprintf(my_SQL_str, 500, "SELECT the_key,the_data FROM %s", t->name());

    // was:
    //sql_ret = SQLFreeStmt(t->db_->hstmt, SQL_CLOSE);    //close from any prior use
    sql_ret = SQLFreeStmt(cur_, SQL_CLOSE);    //close from any prior use
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_crit("ERROR:  ODBCDBIterator(%s)::constructor - failed Statement Handle SQL_CLOSE",
                 t->name());     //glr
        t->print_error(t->db_->m_henv, t->db_->m_hdbc, cur_);
    }

    sql_ret = SQLPrepare(cur_, (SQLCHAR *) my_SQL_str, SQL_NTS);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("constructor SQLPrepare error %d", sql_ret);
        t->print_error(t->db_->m_henv, t->db_->m_hdbc, cur_);
        return;
    }

    if (strcmp(t->name(), "globals") == 0)
    {
        sql_ret =
            SQLBindCol(cur_, 1, SQL_C_CHAR, bound_key_string,
                       MAX_GLOBALS_KEY_LEN, NULL);
    } else {
        sql_ret =
            SQLBindCol(cur_, 1, SQL_C_LONG, &bound_key_int, 0, NULL);
    }

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("constructor SQLBindParameter error %d",
                sql_ret);
        t->print_error(t->db_->m_henv, t->db_->m_hdbc, cur_);
        return;
    }

    if ((bound_data_char_ptr = (char *) malloc(DATA_MAX_SIZE)) == NULL)
    {
        log_err("constructor malloc(DATA_MAX_SIZE) error");
        return;
    }

    sql_ret =
        SQLBindCol(cur_, 2, SQL_C_BINARY, bound_data_char_ptr,
                   DATA_MAX_SIZE, &bound_data_size);

    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("constructor SQLBindCol error %d", sql_ret);
        t->print_error(t->db_->m_henv, t->db_->m_hdbc, cur_);
        free(bound_data_char_ptr);
        bound_data_char_ptr = NULL;
        return;
    }

    sql_ret = SQLExecute(cur_);

    if (sql_ret == SQL_NO_DATA_FOUND)
    {
        log_debug("constructor SQLExecute NO_DATA_FOUND");
        free(bound_data_char_ptr);
        bound_data_char_ptr = NULL;
        return;
    }
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("constructor SQLExecute error %d", sql_ret);
        t->print_error(t->db_->m_henv, t->db_->m_hdbc, cur_);
        free(bound_data_char_ptr);
        bound_data_char_ptr = NULL;
        return;
    }

    valid_ = true;              //cursor is available

    log_debug("constructor exit; cursor (%p) is available.", cur_);
}

//----------------------------------------------------------------------------
ODBCDBIterator::~ODBCDBIterator()
{
    log_debug("destructor enter.");

    SQLRETURN sql_ret;
    valid_ = false;

    free(bound_data_char_ptr);
    bound_data_char_ptr = NULL;

    sql_ret = SQLFreeStmt(cur_, SQL_CLOSE);     //close from any prior use
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_crit("ERROR:  destructor - failed Statement Handle SQL_CLOSE");      //glr
    }

    if ( myTable->store_->serializeAll ) {
        myTable->store_->serialization_lock_.unlock();
    }

    myTable->iterator_lock_.unlock();
    log_debug("destructor exit.");
}

//----------------------------------------------------------------------------
int
ODBCDBIterator::next()
{
    log_debug("next - enter cur_ is (%p) thread(%08X)",
              cur_, (u_int32_t) pthread_self());
    ASSERT(valid_);
    ASSERT(myTable->iterator_lock_.is_locked_by_me());

    SQLRETURN sql_ret;

    memset(&key_, 0, sizeof(key_));
    memset(&data_, 0, sizeof(data_));
    memset(bound_key_string, 0, sizeof(bound_key_string));
    bound_key_int = 0;
    memset(bound_data_char_ptr, 0, DATA_MAX_SIZE);
    bound_data_size = 0;

    sql_ret = SQLFetch(cur_);

    if (sql_ret == SQL_NO_DATA_FOUND)
    {
        log_debug("next SQLFetch NO_DATA_FOUND");
        return DS_NOTFOUND;
    }
    if (!SQL_SUCCEEDED(sql_ret))
    {
        log_err("next SQLFetch error %d", sql_ret);
        valid_ = false;
        return DS_ERR;
    }

    if (bound_data_size == SQL_NULL_DATA)
    {
        log_err
            ("next SQLFetch SQL_NULL_DATA on data column - allow and continue");
    }

    if (strcmp(cur_table_name, "globals") == 0)
    {
        key_.data = (void *) bound_key_string;
        key_.size = strlen(bound_key_string);
        log_debug("next global key=%s", bound_key_string);
    } else {
        key_.data = &bound_key_int;
        key_.size = sizeof(bound_key_int);
        log_debug("next  bound key=%d", (unsigned int) bound_key_int);
        log_debug
            ("next  Key(reverse byte order) unsignedInt=%u int=%d plus size=%d",
             *((int *)key_.data), *((u_int32_t *) key_.data), key_.size);
    }
    data_.data = (void *) bound_data_char_ptr;
    data_.size = bound_data_size;
    log_debug
        ("next first 8-bytes of DATA=%x08 plus size=%d",
         *((u_int32_t*) data_.data), data_.size);

    log_debug("next - exit.");
    return 0;
}

//---------------------------------------------------------------------------t
int
ODBCDBIterator::get_key(SerializableObject * key)
{
    log_debug("get_key - enter.");
    ASSERT(key != NULL);
    ASSERT(myTable->iterator_lock_.is_locked_by_me());
    oasys::Unmarshal un(oasys::Serialize::CONTEXT_LOCAL,
                        static_cast < u_char * >(key_.data), key_.size);

    if (un.action(key) != 0)
    {
        log_err("get_key - error unmarshalling");
        return DS_ERR;
    }

    log_debug("get_key - exit.");
    return 0;
}

//----------------------------------------------------------------------------
int
ODBCDBIterator::raw_key(void **key, size_t * len)
{
    log_debug("raw_key - enter.");
    ASSERT(myTable->iterator_lock_.is_locked_by_me());
    if (!valid_)
        return DS_ERR;

    *key = key_.data;
    *len = key_.size;

    log_debug("raw_key - exit.");
    return 0;
}

//----------------------------------------------------------------------------
int
ODBCDBIterator::raw_data(void **data, size_t * len)
{
    log_debug("raw_data - enter.");
    ASSERT(myTable->iterator_lock_.is_locked_by_me());
    if (!valid_)
        return DS_ERR;

    *data = data_.data;
    *len = data_.size;

    log_debug("raw_data - exit.");
    return 0;
}

/*******************************************************************/
int
ODBCDBTable::print_error(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt)
{
    SQLCHAR buffer[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLCHAR sqlstate[SQL_SQLSTATE_SIZE + 1];
    SQLINTEGER sqlcode;
    SQLSMALLINT length;
    int i = 0;
    int first_sqlcode = 0;

    while (SQLError(henv, hdbc, hstmt, sqlstate, &sqlcode, buffer,
                    SQL_MAX_MESSAGE_LENGTH + 1, &length) == SQL_SUCCESS)
    {
        i++;
        log_debug("**** ODBCDBTable::print_error (lvl=%d) *****", i);
        log_debug("         SQLSTATE: %s", sqlstate);
        log_debug("Native Error Code: %d", (int) sqlcode);
        log_debug("          Message: %s", buffer);
        if (i == 1)
        {
            first_sqlcode = (int) sqlcode;
        }
    };
    return first_sqlcode;
}



}                               // namespace oasys

#endif // LIBODBC_ENABLED
