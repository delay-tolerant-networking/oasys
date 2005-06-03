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
#include "../debug/Debug.h"

namespace oasys {

/**
 * Simple singleton class that just contains the storage-specific
 * configuration variables.
 */
class StorageConfig {
public:
    std::string type_;		///< storage type [berkeleydb/mysql/postgres]
    bool        init_;		///< Create new databases on init
    bool        tidy_;		///< Prune out the database on init
    int         tidy_wait_;	///< Seconds to wait before tidying

    std::string dbname_;	///< Database name (filename in berkeley db)

    // Berkeley DB specific options
    std::string dbdir_;		///< Path to the database files
    std::string dberrlog_;	///< DB internal error log file
    int         dbflags_;       ///< Berkeley DB specific flags

    /**
     * Static initialization function. Must be called by the
     * application before initializing any storage systems.
     */
    static void init(
        const std::string& type,
        bool               init,
        bool               tidy,
        int                tidy_wait,    
        const std::string& dbname,
        const std::string& dbdir,
        const std::string& dberrlog,
        int                dbflags
        );
    
    /**
     * Singleton accessor.
     */
    static StorageConfig* instance()
    {
        if (!instance_) {
            PANIC("StorageConfig::init() must be called");
        }

        return instance_;
    }
    
protected:
    static StorageConfig* instance_;
};

} // namespace oasys

#endif /* _STORAGE_CONFIG_H_ */
