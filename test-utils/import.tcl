#
# Tcl doesn't have a useful import mechanism so we just do a simple one
#

proc import_find {script} {
    global import_path

    # check for full paths
    if [file readable $script] {
	return $script
    }
    
    # otherwise we need the import_path
    if {![info exists import_path]} {
	error "cannot import without setting import_path"
    }

    foreach dir $import_path {
	set path [file join $dir $script]
	if {[file readable $path]} {
	    return $path
	}
    }
    
    error "can't find script $script in import_path $import_path"
}
    
proc import {script} {
    if {$script == ""} {
	error "must specify a script to import"
    }
    
    source [import_find $script]
}


