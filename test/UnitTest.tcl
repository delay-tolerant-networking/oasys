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

proc lassign {varlist list} {
    foreach var $varlist list_val $list {
	uplevel "set $var \"$list_val\""
    }
}

#
# Main
#
chkdirs 1

set g_total  0
set g_passed 0
set g_failed 0

# set tests [glob "*-test"]
set tests {sample-test}

foreach test_exe $tests {
    exec   "mkdir" "output/$test_exe"
    source "$test_exe.cc"
    
    if [catch {exec "./$test_exe" ">output/$test_exe/stdout" "2>output/$test_exe/stderr"} err ] {
 	puts "$test_exe: $err"
	set g_failed [expr $g_failed + 1 ]
	set g_total  [expr $g_total + 1  ]
    } else {
	source "output/$test_exe/stderr"
	lassign {test_suite unit_tests summary} $result
	set output [open "output/$test_exe/stdout" "r"]

	foreach unit_test $unit_tests {
	    lassign {number name status} $unit_test

	    if [ string equal $status "P" ] {
		set g_passed [expr $g_passed + 1 ]
	    } elseif [ string equal $status "F" ] {
		puts "* $test_exe: $name failed, output in output/$test_exe/stdout"
		set g_failed [expr $g_failed + 1 ]
	    } elseif [ string equal $status "I" ] {
		seek $output 0
		set check_result [eval "check$name" $output]
		
		if [ expr $check_result < 0 ] {
		    puts "* $test_exe: $name failed, output in output/$test_exe/stdout"
		    set g_failed [expr $g_failed + 1 ]
		} else {
		    set g_passed [expr $g_passed + 1 ]
		}
	    }
	    set g_total  [expr $g_total + 1 ]
	}
	close $output
    }
}

puts "Total  : $g_total"
puts "Passed : $g_passed"
puts "Failed : $g_failed"