#
# Tcl doesn't have a useful import mechanism so we just do a simple one
#

proc import {script} {
    global import_path

    if {$script == ""} {
	error "must specify a script to import"
    }

    # check for full paths
    if [file readable $script] {
	source $script
	return
    } 

    # otherwise we need the import_path
    if {![info exists import_path]} {
	error "cannot import without setting import_path"
    }

    foreach dir $import_path {
	set path [file join $dir $script]
	if {[file readable $path]} {
	    source $path
	    return
	}
    }

    error "can't find script $script in import_path $import_path"
}
