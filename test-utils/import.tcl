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
