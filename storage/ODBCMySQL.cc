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
//  For main configuration information relating to ODBC access to an MySQL
//  database see ODBCStore.cc.
//
//  This module filters out the non-standard parts of ODBC operation.
//  The class defined here (ODBCDBMySQL) inherits the standard ODBC code
//  from the ODBCDBStore class and provides extra initialization code for
//  checking that there is a odbc.ini section for the selected ODBC
//  DSN (Data Source Name).
//
//  For MySQL (and other client server databases) it is necessary to create the
//  database, setup a suitable user with a known password for accessing this
//  database and install the entry in odbc.ini before ODBC can access the database.
//  This is unlike (say) SQLite where the database file is automatically created
//  on first access provided that the necessary directory exists.
//
//  Required Software on Linux box to run:
//   unixODBC  (unixODBC-2.2.11-7.1 - this conforms to ODBC v3.80)
//   MySQL   (MySQL v5.1. client and server)
//   MySQL Connector/ODBC (v5.1, conforms to ODBC v3.51)
//  Ubuntu packages are available for Release 10.04 LTS for all these components
//
//
//  Modified OASYS storage files:
//    Insertions into DurableStore.cc to instantiate ODBCDBMySQL as the storage
//    implementation:
//    #include "ODBCMySQL.h"
//
//      ...
//  
//    #ifdef LIBODBC_ENABLED
//      ...
//      else if (config.type_ == "odbc-mysql")
//      {
//          impl_ = new ODBCDBMySQL(logpath_);
//          log_debug("impl_ set to new ODBCDBMySQL");
//      }
//     ...
//    #endif
//
//
// TODO: Add test stuff for MySQL
//  Required OASYS test files:
//     mysql-db-test.cc
//
//  When using ODBC the interpretation of the storage parameters is slightly
//  altered.
//  - The 'type' is set to 'odbc-mysql' for a MySQL database accessed via ODBC.
//  - The 'dbname' storage parameter is used to identify the Data Source Name (DSN)
//    configured into the odbc.ini file. In the case of MySQL, the database to be
//    used is defined in the DSN specification, along with necessary parameters
//    to allow access to that database via the MySQL client interface.
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
//      [ODBC Drivers]
//      MyODBC          = Installed
//
//      [MyODBC]
//      Driver          = <installation directory>/lib/libmyodbc5.so
//      Description     = Driver for connecting to MySQL database server
//      Threading       = 0
//  
//    $ODBCSYSINI/odbc.ini:  add new sections for DTN2 DSN(s).  A newly created
//      MySQL server always contains an empty 'test' database.  It is necessary
//      (or at least desirable) to create a user (e.g., dtn_user) that will be
//      used to access the DTN2 database(s).  The user needs to be created and
//      given a password and relevant privileges for the database (ALL privileges
//      is probably OK). If the user is created before the database, it may be
//      necessary to explicitly grant privileges for the database even if the user
//      was given privileges for all databases previously, using the mysql command
//       line interface, thus:
//        mysql -u root -p
//        Password: <Root database user password>
//         SQL> CREATE USER "dtn_user"@"localhost" IDENTIFIED BY "<password>";
//         SQL> CREATE DATABASE dtn_test;
//         SQL> GRANT ALL ON dtn_test.* TO "dtn_user"@"localhost" IDENTIFIED BY "<password>";
//
//      If using a version of the MySQL database before 5.1.6 and defining TRIGGERS
//      on the database, it is also necessary to give the use the SUPER privilege, thus:
//         SQL> GRANT SUPER ON dtn_test.* TO "dtn_user"@"localhost" IDENTIFIED BY "<password>";
//
//      Note that it is advisable to make database names from the character set
//      [0-9a-zA-Z$_].  Using other characters (especially - and /) requires the
//      database name to be quoted using ` (accent grave/backtick). ODBC may or
//      may not cope with such names.
//      The possible values of the OPTION parameter are defined in
//      Section 20.1.4.2 "Connector/ODBC Connection Parameters" of the MySQL
//      Reference Manual (version 5.1).

//      [ODBC Data Sources]
//      myodbc5           = MyODBC 3.51 Driver DSN test
//      dtn-test-mysql    = MySQL DTN test database
//
//      [myodbc5]
//      Driver       = MyODBC
//      Description  = Connector/ODBC 3.51 Driver DSN
//      SERVER       = localhost
//      PORT         =
//      USER         = dtn_user
//      Password     = <the MySQL password set up for dtn_user>
//      Database     = test
//      OPTION       = 3
//      SOCKET       =

//      [dtn-test-mysql]
//      Driver       = MyODBC
//      Description  = Connector/ODBC 3.51 Driver DSN
//      SERVER       = localhost
//      PORT         =
//      USER         = dtn_user
//      Password     = <password>
//      Database     = dtn_test
//      OPTION       = 3
//      SOCKET       =
//
//    After much investigation and experiment, it appears that the
//    keywords and the section names in both odbc.ini and odbcinst.ini
//    should be case insensitive.  unixODBC uses strcasecmp to check for
//    equality of section names and property names. However the values
//    of properties *are* case sensitive and spaces other than round
//    the '=' are preserved.
//
//    The odbc.ini parsing code now ignores all blank lines and comment lines
//    starting with ';' or '#'.  Section and property names are case insensitive.
//    Property lines may have trailing comments starting with ; or #.
//
//  
//##########################################################


#ifdef HAVE_CONFIG_H
#include <oasys-config.h>
#endif

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#include <debug/DebugUtils.h>

#include <util/StringBuffer.h>
#include <util/Pointers.h>
#include <util/ScratchBuffer.h>

#include "StorageConfig.h"
#include "ODBCMySQL.h"
#include "util/InitSequencer.h"

#if LIBODBC_ENABLED

namespace oasys
{
/******************************************************************************
 *
 * ODBCDBMySQL
 *
 *****************************************************************************/

//----------------------------------------------------------------------------
ODBCDBMySQL::ODBCDBMySQL(const char *logpath):
ODBCDBStore("ODBCDBMySQL", logpath)
{
    logpath_appendf("/ODBCDBMySQL");
    log_debug("constructor enter/exit.");
}

//----------------------------------------------------------------------------
ODBCDBMySQL::~ODBCDBMySQL()
{
    log_debug("ODBCMySQL destructor enter/exit.");
    // All the action (clean shutdown of database) takes place in ODBCDBStore destructor
}

//----------------------------------------------------------------------------

//  For MySQL this function serves to check that there is a DSN name of <dbanme>
//  in the odbc.ini file, that it contains a resonably sensible database name
//  (a warning is given if the name is not made up of characters from the set
//  [0-9a-zA-Z$_], which otherwise could lead to issues with quoting being required),
//  to find the name of a SchemaCreationFile for auxiliary database content
//  if one is specified.

int
ODBCDBMySQL::parseOdbcIniMySQL(const char *dsnName, char *databaseName,
		char *userName, char *password, char*schemaPath)
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
    databaseName[0] = '\0';
    userName[0] ='\0';
    password[0] = '\0';
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
            // Extract the property value - name of database used
        	// Not sure if comments are allowed at the end of lines but
        	// stop at ; or # to be sure, as these are not good characters
        	// in database names.
        	// Note that for MySQL this is only used when adding in extra schema file
        	// which uses mysql directly and needs the database name.
        	// ODBC is only interested in the section name and we have
        	// to pre-create the database before ODBC can be used.
            tok = strtok(NULL, " \t\n=;#");
            if ( tok == NULL ) {
            	log_crit("Empty Database property in DSN '%s' in file '%s'.",
            			dsnName, odbciniFilename );
            	break;
            }
            log_debug("Found database (%s) in odbc.ini file (%s).",
                      tok, odbciniFilename);
            strcpy(databaseName, tok);
            // Check for a reasonably sensible name
            bool sensible = true;
            while ( (c = *tok++) != '\0' ) {
            	if ( ! ( isalnum(c) || ( c == '$' ) || ( c == '_') ) ) {
            		sensible = false;
            	}
            }
            if( ! sensible ) {
            	log_warn("Database name '%s' in DSN '%s' uses characters not from set [0-9a-zA-Z$_] - may need quoting.",
            			databaseName, dsnName);
            }
        }
        // In the right section look for User property - again not case sensitive
		else if ( foundDSN && ( strcasecmp(tok, "User") == 0 ) ) {
			// See Database extraction above...
			tok = strtok(NULL, " \t\n=;#");
			log_debug("Found User (%s) for DSN (%s) in odbc.ini file (%s).",
					  tok, dsnName, odbciniFilename);
			strcpy(userName, tok);
		}
        // In the right section look for Password property - again not case sensitive
		else if ( foundDSN && ( strcasecmp(tok, "Password") == 0 ) ) {
			// See Database extraction above...
			tok = strtok(NULL, " \t\n=;#");
			log_debug("Found Password (redacted!) for DSN (%s) in odbc.ini file (%s).",
					  dsnName, odbciniFilename);
			strcpy(password, tok);
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
    if (foundDSN  && (databaseName[0] != '\0') && (userName[0] != '\0') && (password[0] != '\0') ) {
    	return(0);
    }

    if ( ! foundDSN ) {
    	log_crit("DSN '%s' not found in odbc.ini file (%s).  Aborting.",
    			dsnName, odbciniFilename );
    } else {
    	if ( databaseName[0] == '\0' ) {
    	log_crit("Database property is missing or empty in DSN '%s' in odbc.ini file (%s), Aborting.",
    			dsnName, odbciniFilename );
    	}
        if ( userName[0] == '\0' ){
        	log_crit("Database property is missing or empty in DSN '%s' in odbc.ini file (%s), Aborting.",
        			dsnName, odbciniFilename );
        }
        if (password[0] == '\0' ) {
        	log_crit("Database property is missing or empty in DSN '%s' in odbc.ini file (%s), Aborting.",
        			dsnName, odbciniFilename );
        }
    }

    return (-1);
}

int
ODBCDBMySQL::init(const StorageConfig & cfg)
{
    int ret;

    log_debug("init entry.");
    dbenv_ = &base_dbenv_;      //glr

    std::string dbdir = cfg.dbdir_;

    db_name_ = cfg.dbname_;
    sharefile_ = cfg.db_sharefile_;

    bool force_schema_creation = false;

    char database_name[500] = "";
    char database_schema_fullpath[500] = "";
    char database_username[100] = "";
    char database_password[100] = "";

    // For ODBC, the cfg.dbname_ refers to the Data Source Name (DSN) as defined
    // in the odbc.ini file (i.e. the name inside square brackets, e.g. [DTN],
    // starting a section in the file).
    // This code parses the odbc.ini file looking for the given DSN,
    // then checks that there is a non-empty 'Database =' entry in the odbc.ini file.
    // It also extracts the extension SchemaCreationFile entry if there is one.
    //
    // Note that for odbc-based storage methods, the cfg.dbdir_ variable is only
    // used to store the clean exit recording file (.ds_clean - see DurableStore.cc).
    //
    // Parse the odbc.ini file to find the database name, etc.
    ret = parseOdbcIniMySQL(db_name_.c_str(), database_name, database_username,
    		database_password, database_schema_fullpath);

    if ( ret!=0 ) {
        return DS_ERR;
    }

    // Check that the directory cfg.dbdir_ used for .ds_clean file exists and create it if
    // it does not and we are initializing database (i.e., if cfg.init_ is true)
    bool db_dir_exists;

    int err = check_db_dir(cfg.dbdir_.c_str(), &db_dir_exists);
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

	// For MySQL the only way to test that the database exists is to connect to it.
    // It is not possible to connect to a database that doesn't exist.
    if ( ! sharefile_) {
        log_warn("For MySQL databases, specifying separate files ('storage db_sharefile = no') is ignored.");
    }
    log_info("init connecting to db name=%s", db_name_.c_str() );

    ret = connect_to_database( cfg );
    if ( ret != DS_OK ) {
    	return ret;
    }

    if (cfg.tidy_)
    {
        log_crit("init WARNING:  tidy/prune will delete all tables from selected database (%s).",
        		database_name );

        // TODO: Iterate through existing tables deleting them (globals and META_DATA last).
        // force_schema_creation = true;
    }

    if (cfg.init_) {
    	force_schema_creation = true;
    }

    if (force_schema_creation)
    {
        log_crit
            ("WARNING: Initialization of database %s requested: CREATE new DB Schema for DTN (globals, bundles & registrations; links & prophet unused/uncreated) and VRL",
             database_name );

        if ( strlen(database_schema_fullpath)>0 ) {
        	char schema_creation_string[1024];
            log_info
                ("init: sourcing odbc schema creation file (%s) in addition to regular DTN tables",
                 database_schema_fullpath);
            sprintf(schema_creation_string, "mysql %s -u %s -p%s < %s",
        		    database_name, database_username, database_password, database_schema_fullpath);

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

}                               // namespace oasys

#endif // LIBODBC_ENABLED
