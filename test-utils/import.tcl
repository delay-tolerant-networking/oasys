#
#    Copyright 2005-2006 Intel Corporation
# 
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
# 
#        http://www.apache.org/licenses/LICENSE-2.0
# 
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
# Tcl doesn't have a useful import mechanism so we just do a simple one
#

namespace eval import {
    set scripts {}
}

proc import_find {script} {
    global import::path

    # check for full paths
    if [file readable $script] {
	return $script
    }
    
    # otherwise we need the import::path
    if {![info exists import::path]} {
	error "cannot import without setting import::path"
    }

    foreach dir $import::path {
	set my_path [file join $dir $script]
	if {[file readable $my_path]} {
	    return $my_path
	}
    }
    
    error "can't find script $script in import::path $import::path"
}
    
proc import {script} {
    global import::scripts

    if {$script == ""} {
	error "must specify a script to import"
    }

    if {[lsearch -exact $import::scripts $script] != -1} {
	return
    }

    lappend $import::scripts $script
    uplevel \#0 source [import_find $script]
}
