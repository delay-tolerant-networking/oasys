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

#include <sys/time.h>
#include <tcl.h>

//----------------------------------------------------------------------
int
Tclgettimeofday(ClientData cd, Tcl_Interp* interp,
                int argc, CONST char *argv[])
{
    (void)cd; (void)argc; (void)argv;
    struct timeval tv;
    gettimeofday(&tv, 0);
    char buf[64];
    snprintf(buf, sizeof(buf), "%10lu.%06lu",
             (unsigned long)tv.tv_sec,
             (unsigned long)tv.tv_usec);
    Tcl_SetResult(interp, buf, TCL_VOLATILE);
    return TCL_OK;
}

//----------------------------------------------------------------------
int
Tclgettimeofday_Init(Tcl_Interp *interp)
{
    Tcl_CreateCommand(interp, "gettimeofday", Tclgettimeofday, NULL, NULL);
    return 0;
}

//----------------------------------------------------------------------
int
Tclgettimeofday_SafeInit(Tcl_Interp *interp)
{
    return Tclgettimeofday_Init(interp);
}

