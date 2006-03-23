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

#include "LogCommand.h"
#include "debug/Log.h"

namespace oasys {

LogCommand::LogCommand()
    : TclCommand("log")
{
    bind_s("logfile", &Log::instance()->logfile_,
           "The pathname to the logfile.");

    add_to_help("<path> <level> <string>", 
                "Log message string with path, level");
    add_to_help("prefix <prefix>", "Set logging prefix");
    add_to_help("rotate", "Rotate the log file");
    add_to_help("dump_rules", "Show log filter rules");
    add_to_help("reparse", "Reparse the rules file");
}

int
LogCommand::exec(int argc, const char** argv, Tcl_Interp* interp)
{
    // log prefix <string>
    if (argc == 3 && !strcmp(argv[1], "prefix")) {
        Log::instance()->set_prefix(argv[2]);
        logf("/log", LOG_DEBUG, "set logging prefix to '%s'", argv[2]);
        return TCL_OK;
    }

    // log rotate
    if (argc == 2 && !strcmp(argv[1], "rotate")) {
        Log::instance()->rotate();
        return TCL_OK;
    }
    
    // dump rules
    if (argc == 2 && !strcmp(argv[1], "dump_rules")) {
        StringBuffer buf;
        Log::instance()->dump_rules(&buf);
        set_result(buf.c_str());
        return TCL_OK;
    }
    
    // log reparse_debug_file
    if (argc == 2 && 
        (strcmp(argv[1], "reparse_debug_file") == 0 ||
         strcmp(argv[1], "reparse") == 0))
    {
        Log::instance()->parse_debug_file();
        return TCL_OK;
    }
    
    // log path level string
    if (argc != 4) {
        wrong_num_args(argc, argv, 1, 4, 4);
        return TCL_ERROR;
    }

    log_level_t level = str2level(argv[2]);
    if (level == LOG_INVALID) {
        resultf("invalid log level %s", argv[2]);
        return TCL_ERROR;
    }
    
    logf(argv[1], level, argv[3]);

    return TCL_OK;
}

} // namespace oasys
