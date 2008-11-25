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

#ifdef HAVE_CONFIG_H
#  include <oasys-config.h>
#endif

#include "LogCommand.h"
#include "debug/Log.h"

namespace oasys {

LogCommand::LogCommand()
    : TclCommand("log")
{
    bind_var(new StringOpt("logfile", &Log::instance()->logfile_,
                           "file", "The pathname to the logfile."));

    bind_var(new StringOpt("debug_file", &Log::instance()->debug_path_,
                           "file", "The pathname to the log rules file."));

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
    (void)interp;
        
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
    
    logf(argv[1], level, "%s", argv[3]);

    return TCL_OK;
}

} // namespace oasys
