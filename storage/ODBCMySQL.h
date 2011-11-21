/*
 *    Copyright 2004-2006 Intel Corporation
 *    Copyright 2011 Mitre Corporation
 *    Copyright 2011 Trinity College Dublin
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


#ifndef __ODBC_MYSQL_STORE_H__
#define __ODBC_MYSQL_STORE_H__

#ifndef OASYS_CONFIG_STATE
#error "MUST INCLUDE oasys-config.h before including this file"
#endif

#if LIBODBC_ENABLED

#include <stdio.h>              //glr
#include <stdlib.h>             //glr
#include <string.h>             //glr
#include <sys/time.h>           //glr

#include <map>

//glr includes for standard ODBC headers typically in /usr/include
#include <sql.h>                //glr
#include <sqlext.h>             //glr
#include <sqltypes.h>           //glr
#include <sqlucode.h>           //glr

#include "../debug/Logger.h"

#include "StorageConfig.h"
#include "ODBCStore.h"

#define DATA_MAX_SIZE (1 * 1024 * 1024) //1M - increase this and table create for larger buffers


namespace oasys
{


/**
 * Interface for the generic datastore
 */
class ODBCDBMySQL: public ODBCDBStore
{

  public:
      ODBCDBMySQL (const char *logpath);

    // Can't copy or =, don't implement these
      ODBCDBMySQL & operator= (const ODBCDBMySQL &);
      ODBCDBMySQL (const ODBCDBMySQL &);

     ~ODBCDBMySQL ();

    //! @{ Virtual from DurableStoreImpl
    //! Initialize ODBCDBMySQL
    int init (const StorageConfig & cfg);

    /// @}

  private:

    int parseOdbcIniMySQL(const char *dsnName, char *databaseName,
    		char *userName, char *password, char *schemaPath);

};


};                              // namespace oasys

#endif // LIBODBC_ENABLED

#endif //__ODBC_MYSQL_STORE_H__
