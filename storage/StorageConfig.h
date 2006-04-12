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
#ifndef _OASYS_STORAGE_CONFIG_H_
#define _OASYS_STORAGE_CONFIG_H_

#include <string>
#include "../debug/DebugUtils.h"

namespace oasys {

/*!
 * Simple singleton class that just contains the storage-specific
 * configuration variables. An instance of this configuration is given
 * to Durable storage system to initialize it.
 */
struct StorageConfig {
    // General options that must be set (in the constructor)
    std::string cmd_;		///< tcl command name for this instance
    std::string type_;		///< storage type [berkeleydb/mysql/postgres]
    std::string dbname_;	///< Database name (filename in berkeley db)
    std::string dbdir_;		///< Path to the database files

    // Other general options
    bool        init_;		///< Create new databases on init
    bool        tidy_;		///< Prune out the database on init
    int         tidy_wait_;	///< Seconds to wait before tidying
    bool        leave_clean_file_;///< Leave a .ds_clean file on clean shutdown

    // Berkeley DB Specific options
    bool        db_mpool_;      ///< Use DB mpool (default true)
    bool        db_log_;        ///< Use DB log subsystem
    bool        db_txn_;        ///< Use DB transaction
    int         db_max_tx_;     ///< Max # of active transactions (0 for default)
    int		db_max_locks_;	///< Max # of active locks (0 for default)
    int		db_max_lockers_;///< Max # of active locking threads (0 for default)
    int		db_max_lockedobjs_;///< Max # of active locked objects (0 for default)
    int		db_max_logregion_; ///< Logging region max (0 for default)
    int         db_lockdetect_; ///< Frequency in msecs to check for deadlocks
                                ///< (locking disabled if zero)
    bool	db_sharefile_;	///< Share a single DB file (and a lock)

    StorageConfig(
        const std::string& cmd,
        const std::string& type,
        const std::string& dbname,
        const std::string& dbdir)
        
      : cmd_(cmd),
        type_(type),
        dbname_(dbname),
        dbdir_(dbdir),
        
        init_(false),
        tidy_(false),
        tidy_wait_(3),
        leave_clean_file_(true),
        db_mpool_(true),
        db_log_(true),
        db_txn_(true),
        db_max_tx_(0),
        db_max_locks_(0),
        db_max_lockers_(0),
        db_max_lockedobjs_(0),
        db_max_logregion_(0),
        db_lockdetect_(5000),
        db_sharefile_(false)
    {}
};

} // namespace oasys

#endif /* _STORAGE_CONFIG_H_ */
