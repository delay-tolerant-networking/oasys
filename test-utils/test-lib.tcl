
import "dist-utils.tcl"
import "net-utils.tcl"
import "run-utils.tcl"

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
	return $conf::conf($exec_name,$node)
    }
}

#
# Functions for running the test
# 
namespace eval test {
    set run_actions ""
    set testname    ""

    # Script actions to be performed after launching everything
    proc script { actions } {
	global test::run_actions
	set test::run_actions $actions
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
