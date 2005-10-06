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

#include "HelpCommand.h"

#include "memory/Memory.h"
#include "util/StringBuffer.h"

namespace oasys {

HelpCommand::HelpCommand()
    : TclCommand("help")
{
}

int
HelpCommand::exec(int argc, const char** argv, Tcl_Interp* interp)
{
    const TclCommandList *cmdlist = NULL;
    TclCommandList::const_iterator iter;
    
    cmdlist = TclCommandInterp::instance()->commands();

    if (argc == 1) {
        StringBuffer buf;
        int len = 0;

        buf.append("For help on a particular command, type \"help <cmd>\".\n");
        buf.append("The registered commands are: \n\t");
                   
        for (iter = cmdlist->begin(); iter != cmdlist->end(); iter++) {
            if (len > 60) {
                buf.appendf("\n\t");
                len = 0;
            }

            len += buf.appendf("%s ", (*iter)->name());
        }

        set_result(buf.c_str());

        return TCL_OK;
    } else if (argc == 2) {
        for (iter = cmdlist->begin(); iter != cmdlist->end(); iter++) {
            if (!strcmp((*iter)->name(), argv[1])) {
                const char *help = (*iter)->help_string();
                const char *binfo = "";

                if (!help || (help && help[0] == '\0')) {
                    help = "(no help, sorry)";
                }

                StringBuffer buf;
                if ((*iter)->hasBindings()) {
                    buf.appendf("%s info\n\t%s", (*iter)->name(),
                                "Lists settable parameters.\n");
                    binfo = buf.c_str();
                }

                resultf("%s%s", binfo, help);
                
                return TCL_OK;
            }
        }

        resultf("no registered command '%s'", argv[1]);
        return TCL_ERROR;
        
    } else {
        wrong_num_args(argc, argv, 2, 3, 3);
        return TCL_ERROR;
    }
}

const char*
HelpCommand::help_string()
{
    return("help <cmd?>");
}

} // namespace oasys
