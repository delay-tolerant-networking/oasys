#!/usr/bin/tclsh

#
# Unit testing framework - (also see util/UnitTest.cc)
#  
# Creates a directory output/*-testname for each test run.
#

# Check and create test output directories
proc chkdirs {clobber} {
    if [expr [file exists output] && ! $clobber] {
	puts "- no clobber set, not overwriting test results"
	exit -1
    }

#    puts "- clobbering output/ directory"
    exec "rm" "-rf" "output/"
    exec "mkdir" "output/"
}

# assign a list of variables to the elements in the list
proc lassign {varlist list} {
    foreach var $varlist list_val $list {
	uplevel "set $var \"$list_val\""
    }
}

# log messages which can be suppressed with --silent
proc puts_reg {args} {
    global g_silent
    if {! $g_silent} {
	eval puts $args
    }
}

# parse arguments
proc parse_args {} {
    global argv g_silent
    
    foreach arg $argv {
	if [string equal $arg "--silent"] {
	    set g_silent 1
	}
    }
}

#
# Main
#
chkdirs 1

set g_total  0
set g_passed 0
set g_failed 0
set g_silent 0

parse_args

# set tests [glob "*-test"]
set tests {sample-test timer-test serialize-test}

foreach test_exe $tests {
    if {[file exists "$test_exe.cc"]} {
    } elseif {[file exists "test/$test_exe.cc"]} {
	cd "test"
    } else {
	error "couldn't find $test_exe.cc or test/$test_exe.cc"
    }

    file mkdir "output/$test_exe"
    set fd [open "$test_exe.cc"]

    set code [read $fd]

    while {1} {
	set matched [regexp {.*?DECLARE_TEST_TCL(.*?)endif(.*)$} $code match tcl_code rest]
	
	if {!$matched} {
	    break
	}
	
	eval $tcl_code
	set code $rest
    } 

    close $fd

    puts_reg -nonewline "* $test_exe: "
    flush stdout
    
    if [catch {exec "./$test_exe" "-test" ">output/$test_exe/stdout" "2>output/$test_exe/stderr"} err ] {
 	puts "$err"
	set g_failed [expr $g_failed + 1 ]
	set g_total  [expr $g_total + 1  ]
    } else {
	source "output/$test_exe/stderr"
	lassign {test_suite unit_tests summary} $result
	set output [open "output/$test_exe/stdout" "r"]

	set all_clear 1
	foreach unit_test $unit_tests {
	    lassign {number name status} $unit_test

	    if [ string equal $status "P" ] {
		set g_passed [expr $g_passed + 1 ]
	    } elseif [ string equal $status "F" ] {
		puts "$name failed, output in output/$test_exe/stdout"
		set g_failed [expr $g_failed + 1 ]
		set all_clear 0
	    } elseif [ string equal $status "I" ] {
		seek $output 0
		set check_result [eval "check$name" $output]
		
		if [ expr $check_result < 0 ] {
		    puts "$name failed, output in output/$test_exe/stdout"
		    set g_failed [expr $g_failed + 1 ]
		    set all_clear 0
		} else {
		    set g_passed [expr $g_passed + 1 ]
		}
	    }
	    set g_total  [expr $g_total + 1 ]
	}
	close $output

	if {$all_clear} {
	    puts_reg "passed"
	}
    }
}

puts_reg "Total  : $g_total"
puts_reg "Passed : $g_passed"
puts_reg "Failed : $g_failed"
