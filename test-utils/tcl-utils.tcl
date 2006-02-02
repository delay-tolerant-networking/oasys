#
# Miscellaneous tcl utilities
#

proc do_until {what timeout script} {
    upvar do_what    do_what
    upvar do_timeout do_timeout
    upvar do_script  do_script
    upvar do_start   do_start

    set do_what      $what
    set do_timeout   $timeout
    set do_script    $script
    set do_start     [clock clicks -milliseconds]
    
    uplevel 1 {
	while {1} {
	    if {[clock clicks -milliseconds] > $do_start + $do_timeout} {
		error "timeout in '$do_what'"
	    }
	    eval $do_script
	}
    }
}

proc gethostbyname {host} {
    if {![catch {package require Tclx} err]} {
	return [lindex [host_info addresses $host] 0]
    }

    # XXX/demmer do something else
    error "no gethostbyname implementation available (install Tclx)"
}

# Cute implementation of structs. Convert to a list to pass between
# procs
proc struct { name structlist } {
    set $name._meta_ {}

    foreach { key value } $structlist {
	uplevel set $name.$key $value
	uplevel lappend $name._meta_ $key
    }
}

proc struct2list { name } {
    set l {}
    foreach { key } [uplevel set "$name._meta_"] {
	lappend l $key [uplevel set "$name.$key"]
    }
    
    return $l
}

# unit test for struct
if 0 {
    struct s {
	a 1 
	b 2
    }
    
    puts ${s._meta_}
    puts [struct2list s]
}
