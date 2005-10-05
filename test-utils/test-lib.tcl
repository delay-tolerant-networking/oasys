
import "command-init.tcl"
import "tcl-utils.tcl"
import "dist-utils.tcl"
import "net-utils.tcl"
import "run-utils.tcl"
import "tell-utils.tcl"

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
    set testname     ""

    # Script actions to be performed after launching everything
    proc script { actions } {
	global test::run_actions
	set test::run_actions $actions
    }

    # Script to run after everything has shut down
    proc exit_script { actions } {
	global test::exit_actions
	set test::exit_actions $actions
    }

    # Run the script actions
    proc run_script {} {
	global test::run_actions
	uplevel \#0 $test::run_actions
    }

    # Run the exit script actions
    proc run_exit_script {} {
	global test::exit_actions
	if {$test::exit_actions != ""} {
	    uplevel \#0 $test::exit_actions
	}
    }

    # Set test name
    proc name { {name ""} } {
	global test::testname
	if {$name != ""} {
	    set test::testname $name
	}
	return $test::testname
    }
}
