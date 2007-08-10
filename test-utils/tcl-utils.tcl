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

#
# Rename the after proc to real_after so we can still run tcl events
# in the background during the 'after $N' syntax
#

rename after real_after

proc after {args} {
    if {[llength $args] == 1} {
        global after_vwait
        set delay [lindex $args 0]
        set after_vwait 0
        real_after $delay set after_vwait 1
        vwait after_vwait
    } else {
        eval real_after $args
    }
}

#
# Grab gethostbyname from Tclx
#
proc gethostbyname {host} {
    if {![catch {package require Tclx} err]} {
	return [lindex [host_info addresses $host] 0]
    }

    # XXX/demmer do something else
    error "no gethostbyname implementation available (install Tclx)"
}


#
# Try to load the tclgettimeofday implementation from the test import path
#
foreach dir $import::path {
    set library [file join $dir libtclgettimeofday[info sharedlibextension]]
    if {[file exists $library]} {
        load $library
        break
    }
}

#
# If we can't find it, then all we can do is have seconds
#
if {[info commands gettimeofday] == ""} {
    proc gettimeofday {} {
        return [clock seconds].000000
    }
}

#
# Simple proc for test output to match the loging output
#
proc testlog {args} {
    if {[llength $args] == 2} {
        set level [lindex $args 0]
        set msg   [lindex $args 1]
    } elseif {[llength $args] == 1} {
        set level always
        set msg   [lindex $args 0]
    } else {
        error "testlog <level?> message"
    }
    puts "\[test [gettimeofday] /test-script $level\] $msg"
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

proc struct->list { name } {
    set l {}
    foreach { key } [uplevel set "$name._meta_"] {
	lappend l $key [uplevel set "$name.$key"]
    }
    
    return $l
}

proc k->v {key alist {empty ""}} {
    foreach {k v} $alist {
	if [string equal $key $k] {
	    return $v
	}
    }

    return $empty
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

#
# arglist is the list of arguments given, varlist is a list of 
# {variable_name options "argument text" "argument text"}
#
proc parse_args { arglist varlist } {
    
}
