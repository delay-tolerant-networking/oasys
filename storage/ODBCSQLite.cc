/*
 *    Copyright 2004-2006 Intel Corporation
 *    Copyright 2011 Mitre Corporation
 *    Copyright 2011 Trinity Collage Dublin
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
//  For main configuration information relating to ODBC access to an SQLite
//  database see ODBCStore.cc.
//
//  This module filters out the non-standard parts of ODBC operation.
//  The main class defined here (ODBCDBSQLite) inherits the standard ODBC code
//  from the ODBCDBStore class and provides extra initialization code for
//  locating the file used to store the SQLite database and, when required,
//  initializing tha directory where it is to be stored.  If the database file
//  doesn't exist when the connection is opened, the file is created (provided
//  the directory exists). Note that this differs from the client-server case
//  wherethe database has to be created and a user set up before the database
//  can be accessed by ODBC.
//
//
//  Required Software on Linux box to run:
//   unixODBC
//   SQLite
//   SQLite ODBC libs (libsqlite3odbc-0.83.so -
//                     download from http://www.ch-werner.de/sqliteodbc/)
//
//  Ubuntu packages are available for Release 10.04 LTS for all these components
//  although the versions are marginally different across releases and distributions:
//   unixODBC   ==> 2.2.11-21
//   SQLite     ==> 3.3.6
//   SQLiteODBC ==> 0.80
//
//  Required OASYS storage files:
//    ODBCStore.h
//    ODBCSQLite.h
//    ODBCStore.cc
//    ODBCSQLite.cc
//  NOTE: need to modify init() to also CREATE TABLE for all VRL router schema tables!!!
//    SQLite_DTN_and_VRL_schema.ddl
//  NOTE: need to add CREATE TABLE for all VRL router schema tables!!!
//
//  Modified OASYS storage files:
//    Insertions into DurableStore.cc to instantiate ODBCDBSQLite as the storage
//    implementation:
//     #include "ODBCSQLite.h"
//
//     ...
//  
//      else if (config.type_ == "odbc-sqlite")
//      {
//          impl_ = new ODBCDBSQLite(logpath_);
//          log_debug("impl_ set to new ODBCDBSQLite");
//      }
//
//  Required OASYS test files:
//     sqlite-db-test.cc
//
//  When using ODBC the interpretation of the storage parameters is slightly
//  altered.
//  - The 'type' is set to 'odbc-sqlite' for a SQLite database accessed via ODBC.
//  - The 'dbname' storage parameter is used to identify the Data Source Name (DSN)
//    configured into the odbc.ini file. In the case of SQLite, the full path name
//    of the database file to be used is defined in the DSN specification, along
//    with necessary parameters especially the connection timeout.
//  - The 'dbdir' parameter is still needed because the startup/shutdown system
//    can, on request, store a 'clean shutdown' indicator by creating the
//    file .ds_clean in the directory specified in 'dbdir'.   Hence 'dbdir' is
//    still (just) relevant.
//  - The storage payloaddir *is* still used to store the payload files which
//    are not held in the database.
//
//  Modifications to ODBC configuration files (requires root access):
//    $ODBCSYSINI/odbcinst.ini:  add new section if MySQL ODBC lib *.so
//    is not already extant.  The default for ODBCSYSINI seems to have
//    altered  between version 2.2.x and version 2.3.x of unixODBC.
//    Older versions use /etc. Newer seem to use /etc/unixODBC.
//    You can check what is right for your system with the command
//      odbcinst -j -v
//    Additionally there are some GUI tools available to assist with
//    configuring unixODBC. These can be downloaded from
//       http://unixodbc-gui-qt.svn.sourceforge.net/
//    This appears not to be available as a precompiled package for
//    Debian/Ubuntu (and does not compile entirely cleanly as of
//    November 2011.)
//    BEWARE: There is also a user odbc.ini (default ~/.odbc.ini).
//    TODO: Scan this file as well as the system odbc.ini.
//    Note that unixODBC scans the user odbc.ini BEFORE the system one.
//    If you have a section in this that matches the section name in the
//    DTN2 configuration file dbname parameter, then unixODBC will try to
//    use that DSN rather than one with the same name in the system odbc.ini.
//    If they are different this could lead to confusion.
//
//  
//      [SQLite3]
//      Description  = SQLite3 ODBC Driver
//      Driver       = <installation directory for sqlite odbc driver>/libsqlite3odbc-0.83.so
//      Setup        = <installation directory for sqlite odbc driver>/libsqlite3odbc-0.83.so
//      UsageCount   = 1
//  
//    $ODBCSYSINI/odbc.ini:  add new DSN sections for DTN test/production DB as necessary
//
//      [<config DB name for DTN DB>]
//      Description   = SQLite database storage DSN for DTN2
//      Driver        = SQLite3
//      Database      = <fullPath to DB directory>/<SQLite database file name>
//      Timeout       = 100000
//      StepAPI       = No
//      NoWCHAR       = No
//
//  NOTE: should disable or not use configuration tidy/prune or any dynamic DB
//  creation due to data loss!!
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

#include "StorageConfig.h"
#include "ODBCSQLite.h"
#include "util/InitSequencer.h"

#if LIBODBC_ENABLED

namespace oasys
{
/******************************************************************************
 *
 * ODBCDBSQLite
 *
 *****************************************************************************/

//----------------------------------------------------------------------------
ODBCDBSQLite::ODBCDBSQLite(const char *logpath):
ODBCDBStore("ODBCDBSQLite", logpath)
{
    logpath_appendf("/ODBCDBSQLite");
    log_debug("constructor enter/exit.");
}

//----------------------------------------------------------------------------
ODBCDBSQLite::~ODBCDBSQLite()
{
    log_debug("ODBCDBSQLite destructor enter/exit.");
    // All the action (clean shutdown of database) takes place in ODBCDBStore destructor
}

//----------------------------------------------------------------------------


int
ODBCDBSQLite::parseOdbcIniSQLite(const char *dsnName, char *fullPath, char*schemaPath)
{
    FILE *f;
    char odbciniFilename[1000];
    char *oneLine;
    char *tok;
    char c;
    size_t lineLen;

    char odbcDsnIdentifier[500];
    char section_start = '[';
    char section_end = ']';
    bool foundDSN = false;

    sprintf(odbcDsnIdentifier, "%c%s%c", section_start, dsnName, section_end);
    fullPath[0] = '\0';
    schemaPath[0] = '\0';

    // TODO: (Elwyn) Some installations may use /etc/unixODBC to house the odbc.ini file.
    // Note that ODBCSYSINI is the name of the directory where the system odbc.ini
    // and odbcinst.ini are found (not the name of either of these files).
    // We really ought to also check in the user's ODBCINI (defaults to ~/.odbc.ini).
    if ( ( tok = getenv("ODBCSYSINI") ) == NULL ) {
        sprintf(odbciniFilename, "/etc/%s", "odbc.ini");
    } else {
        sprintf(odbciniFilename, "%s/%s", tok, "odbc.ini");
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
        tok = strtok(oneLine, "= \t\n");
    	// Ignore blank lines and comment lines.
        if ( ( tok==NULL ) || ( (c = tok[0]) == ';' ) || ( c == '#') ) {
        	continue;
        }

        // Stop at beginning of next section
        if ( foundDSN && ( c == section_start ) ) {
        	break;
        }

        // Look for interesting section start - names are not case sensitive
        if ( !foundDSN && ( strcasecmp(tok, odbcDsnIdentifier) == 0 ) )
        {
            log_debug("Found DSN %s", odbcDsnIdentifier);
            foundDSN = true;
            continue;
        // In the right section look for Database property - again not case sensitive
        } else if ( foundDSN && ( strcasecmp(tok, "Database") == 0 ) ) {
            // Extract the property value - name of database file used
        	// Not sure if comments are allowed at the end of lines but
        	// stop at ; or # to be sure, as these are not good characters
        	// in database names.
        	// For SQLite this path name is needed both to ensure that the
        	// directory containing the file is present when initializing
        	// the database so that SQLite3 can craete the file on first access,
        	// and also to allow sqlite3 to be used directly to read in the
        	// additional schema file, if any.
            tok = strtok(NULL, " \t\n=;#");
            if ( tok == NULL ) {
            	log_crit("Empty Database property in DSN '%s' in file '%s'.",
            			dsnName, odbciniFilename );
            	break;
            }
            log_debug("Found database path name (%s) in odbc.ini file (%s).",
                      tok, odbciniFilename);
            strcpy(fullPath, tok);
        }
        // In the right section look for SchemaCreationFile property - again not case sensitive
		else if ( foundDSN && ( strcasecmp(tok, "SchemaCreationFile") == 0 ) ) {
			// See Database extraction above...
			tok = strtok(NULL, " \t\n=;#");
			log_debug("Found schema creation file (%s) for DSN (%s) in odbc.ini file (%s).",
					  tok, dsnName, odbciniFilename);
			strcpy(schemaPath, tok);
		}
    }
    free(oneLine);
    fclose(f);

    // All is well if we found the section requested and it has a non-empty Database value
    if (foundDSN  && (fullPath[0] != '\0') ) {
    	return(0);
    }

    if ( ! foundDSN ) {
    	log_crit("DSN '%s' not found in odbc.ini file (%s).  Aborting.",
    			dsnName, odbciniFilename );
    } else {
    	log_crit("Database property is missing or empty in DSN '%s' in odbc.ini file (%s), Aborting.",
    			dsnName, odbciniFilename );
    }

    return (-1);
}

int
ODBCDBSQLite::init(const StorageConfig & cfg)
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
    // Note that for odbc-based storage methods, the cfg.dbdir_ variable is only
    // used to store the clean exit recording file (.ds_clean - see DurableStore.cc).
    //
    // Parse the odbc.ini file to find the database name; copy the full path to
    // the database file into database_fullpath
    //sprintf(sys_command, "%s%s%s",
    //        section_start, cfg.dbname_.c_str(), section_end);
    ret = parseOdbcIniSQLite(cfg.dbname_.c_str(), database_fullpath, database_schema_fullpath);

    if ( ret!=0 ) {
        return DS_ERR;
    }
    char *c;
    strcpy(database_dir, database_fullpath);
    c = rindex(database_dir, '/');
    if ( c != NULL )
    {
        *c = '\0';
        dbdir = database_dir;
    }
    else
    {
    	dbdir = "";
    }

    // XXX/bowei need to expose options if needed later
    if (cfg.tidy_)
    {
        log_crit("init WARNING:  tidy/prune DB dir %s will delete all DB files, i.e., all DTN and VRL DB tables/data",  //glr
                 dbdir.c_str());
        prune_db_dir(dbdir.c_str(), cfg.tidy_wait_);
    }

    bool force_schema_creation = false;

    // Check that the directory used for SQLite files exists and create it if
    // it does not and we are initializing database (i.e., if cfg.init_ is true)
    bool db_dir_exists;
    int err;

    err = check_db_dir(dbdir.c_str(), &db_dir_exists);
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

    // Check that the directory cfg.dbdir_ used for .ds_clean file exists and create it if
    // it does not and we are initializing database (i.e., if cfg.init_ is true)
    err = check_db_dir(cfg.dbdir_.c_str(), &db_dir_exists);
    if (err != 0)
    {
        log_crit("init  ERROR:  CHECK failed on DB dir %s so exit!",     //glr
                 cfg.dbdir_.c_str());
        return DS_ERR;
    }
    if (!db_dir_exists)
    {
        log_crit("init  WARNING: directory for .ds_clean %s does not exist.",    //glr
                 cfg.dbdir_.c_str());
        if (cfg.init_)
        {
            if (create_db_dir(cfg.dbdir_.c_str()) != 0)
            {
                return DS_ERR;
            }
            force_schema_creation = true;
            log_info("Directory %s for .ds_clean created,", cfg.dbdir_.c_str());
        } else {
            log_crit
                ("init  directory for .ds_clean %s does not exist and not told to create!",
                 cfg.dbdir_.c_str());
            return DS_ERR;
        }
    }
    else{
    	log_debug("init: directory for .ds_clean %s exists.", cfg.dbdir_.c_str());
    }

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

	// NOTE: It would be possible to use multiple files (e.g., one for each table
	// in SQLite3 by using the ATTACH DATABASE mechanism.  It isn't clear that this
	// would help performance and it significantly increases the setup complexity.
	// Currently one common file is used and the db_sharefile parameter is ignored.
    log_info("init initializing db name=%s (db_sharefile requested %s - currently ignored), dir=%s",
             db_name_.c_str(), sharefile_ ? "shared" : "not shared",
             dbdir.c_str());

    // Setup all the ODBC environment and connect to the database.
    ret = connect_to_database( cfg );
    if ( ret != DS_OK ) {
    	return ret;
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

        // Create auxiliary tables - main storage tables are created automatically
        // when ODBCDBTable instances are created for specific tables with DS_CREATE true.
        ret = create_aux_tables();
        if ( ret != DS_OK ) {
        	return ret;
        }
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


//----------------------------------------------------------------------------

}                               // namespace oasys

#endif // LIBODBC_ENABLED
