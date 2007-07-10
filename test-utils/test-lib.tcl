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


import "command-init.tcl"
import "tcl-utils.tcl"
import "dist-utils.tcl"
import "net-utils.tcl"
import "run-utils.tcl"
import "tell-utils.tcl"
import "status-utils.tcl"

#
# Manage each node's tcl configuration file
#
namespace eval conf {
    proc add { exec_name node text } {
	global conf::conf

	if {$node == "*"} {
	    foreach i [net::nodelist] {
		conf::add $exec_name $i $text
	    }
	} else {
	    append conf::conf($exec_name,$node) "$text\n"
	}
    }

    proc get { exec_name node } {
	global conf::conf

	if [info exist conf::conf($exec_name,$node)] {
	    return $conf::conf($exec_name,$node)
	} else {
	    return ""
	}
    }
}

#
# Functions for running the test
# 
namespace eval test {
    set run_actions  ""
    set exit_actions ""
    set error_actions  ""
    set cleanup_actions ""
    set testname     ""

    # Script actions to be performed after launching everything
    proc script { actions } {
	global test::run_actions
	set test::run_actions $actions
    }

    # Script actions to be run if there was an error in the test script
    proc error_script { actions } {
	global test::error_actions
	set test::error_actions $actions
    }

    # Script to run after the test is done
    proc exit_script { actions } {
	global test::exit_actions
	set test::exit_actions $actions
    }

    # Script to run after everything has shut down
    proc cleanup_script { actions } {
	global test::cleanup_actions
	set test::cleanup_actions $actions
    }

    # Run the script actions
    proc run_script {} {
	global test::run_actions opt
	if {$opt(dry_run)} {
	    return 0
	}

        if [catch {
            uplevel \#0 $test::run_actions
        } err] {
            global errorInfo
            puts "error in test [test::name]: $errorInfo"
            run_error_script
            return -1
        }
        return 0
    }

    # Run the exit script actions
    proc run_exit_script {} {
	global test::exit_actions opt
	if {$opt(dry_run)} {
	    return 0
	}
	if {$test::exit_actions != ""} {
            if [catch {
                uplevel \#0 $test::exit_actions
            } err] {
                global errorInfo
                puts "error: $errorInfo"
                return -1
            }
	}

        return 0
    }

    # Run the error script actions
    proc run_error_script {} {
	global test::error_actions opt
	if {$opt(dry_run)} {
	    return 0
	}

        if {$test::error_actions != ""} {
            if [catch {
                uplevel \#0 $test::error_actions
            } err] {
                global errorInfo
                puts "error: $errorInfo"
                return -1
            }
        }

        return 0
    }

    # Run the cleanup script actions
    proc run_cleanup_script {} {
	global test::cleanup_actions opt
	if {$opt(dry_run)} {
	    return 0
	}
	if {$test::cleanup_actions != ""} {
            if [catch {
                uplevel \#0 $test::cleanup_actions
            } err] {
                global errorInfo
                puts "error: $errorInfo"
                return -1
            }
	}
        return 0
    }

    # Set test name
    proc name { {name ""} } {
	global test::testname
	if {$name != ""} {
	    set test::testname $name
	}
	return $test::testname
    }

    # Parse args in a simple way
    proc get_args {vardesc} {
	global opt
	for {set i 0} {$i < [llength $opt(opts)]} {incr i} {
	    set var [string trimleft [lindex $opt(opts) $i] -]
	    set varname [k->v $var $vardesc]
	    
	    if {$varname == ""} {
		puts "ERROR: unrecognized option \"$var\""
		exit 1
	    }	    

	    set val [lindex $opt(opts) [incr i]]
	    uplevel set $var $val
	}

	foreach {var def} $vardesc {
	    if {! [uplevel info exists $var]} {
		uplevel set $var $def
	    }
	    puts "* option $var=[uplevel set $var]"
	}
    }

    # Identity function
    proc as_string {args} { 
	return $args
    }

    # Evaluates predicate and prints out the appropriate error message
    # if the evaluation is false.
    proc CHECK {pred {error_msg ""}} {
	set res [uplevel 1 $pred]
	set pred_str [uplevel 1 test::as_string $pred]

	if {$res} {
	    puts "CHECK: $pred_str passed"
	} else {
	    error "ERROR: $pred_str failed $error_msg"
	}
    }

    # Evaluates predicate and prints out the appropriate error message
    # if the evaluation is true.
    proc CHECK! {pred {error_msg ""}} {
	set res [uplevel 1 $pred]
	set pred_str [uplevel 1 test::as_string $pred]

	if {$res} {
	    error "ERROR: $pred_str failed $error_msg"
	} else {
	    puts "CHECK!: $pred_str passed"
	}
    }

    # Evaluates the predicate and checks for equality
    proc CHECK_EQ {pred val {error_msg ""}} {
	set res [uplevel 1 $pred]
	set pred_str [uplevel 1 test::as_string $pred]

	if {$res == $val} {
	    puts "CHECK_EQ: \"$pred_str\" == \"$val passed\""
	} else {
	    error "ERROR: $pred_str, \"$res\" != \"$val\" $error_msg"
	}
    }

    # Evaluates the predicate and checks for nequality
    proc CHECK_NEQ {pred val {error_msg ""}} {
	set res [uplevel 1 $pred]
	set pred_str [uplevel 1 test::as_string $pred]

	if {$res != $val} {
	    puts "CHECK_EQ: $pred_str, \"$res\" != \"$val\" passed"
	} else {
	    error "ERROR: \"$pred_str\" == \"$val\" $error_msg"
	}
    }

    # Store the time when the script was parsed in a global variable
    # so it can be used for the elapsed proc
    set starttime [clock seconds]
    
    # Return the number of seconds since the test started
    proc elapsed {} {
        global test::starttime
        
        return [expr [clock seconds] - $test::starttime]
    }
    
    # Return the number of seconds since the last interval call
    proc interval_elapsed {} {
        global test::intervaltime
        
        if {![info exists intervaltime]} {
            set intervaltime 0
        }
        set last $intervaltime
        set intervaltime [clock seconds]
        return [expr $intervaltime - $last]
    }
    
}
