#!/usr/bin/tclsh

#
# Unit testing framework - (also see util/UnitTest.cc)
#
proc usage {} {
    puts {Usage: UnitTest.tcl [OPTION]... [TESTUNIT]...
Run the unit test suite.

-c Clobber and run all tests again.
-s Run in silent mode.
    }
}

# parse arguments
proc parse_args {} {
    global argv g_clobber g_error_only g_silent 
    
    foreach arg $argv {
	if [string equal $arg "-s"] {
	    set g_silent 1
	}
	if [string equal $arg "-c"] {
	    set g_clobber    1
	    set g_error_only 0
	}
	if [expr [string equal $arg "-h"] || [string equal $arg "--help"]] {
	    usage
	    exit 0
	}

    }
}

# Check and create test output directories
proc chkdirs {clobber} {
    if [expr [file exists output] && ! $clobber] {
	puts_reg "- no clobber set, not overwriting test results"
	return
    }

    puts_reg "- clobbering output/ directory"
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

# strip out and execute tcl code from CC file 
proc parse_cc_file {test_exe} {
    if [expr ! [file exists "$test_exe.cc"]] {
	error "couldn't find $test_exe.cc or test/$test_exe.cc"
    }

    set fd [open "$test_exe.cc"]
    set code [read $fd]

    if [expr ! [regexp "UnitTest.h" $code]] {
	return 0
    }
    
    while {1} {
	set matched [regexp {.*?DECLARE_TEST_TCL(.*?)endif(.*)$} $code match tcl_code rest]
	
	if {!$matched} {
	    break
	}
	
	eval $tcl_code
	set code $rest
    } 
    close $fd

    return 1
}

##############################################################################
#
# Main
#
set g_total   0
set g_passed  0
set g_failed  0

set g_clobber    0;			# clobber directories
set g_error_only 1;			# rerun only error cases
set g_silent     0;			# run silently
parse_args

chkdirs $g_clobber

set tests [glob "*-test"]

foreach test_exe $tests {
    if [expr ! [parse_cc_file $test_exe]] {
	continue
    }

    puts_reg -nonewline "* $test_exe: "
    flush stdout
    
    if [expr $g_error_only && [file exists output/$test_exe/PASSED]] {
	puts_reg "skipping (cached)"
	continue
    }
    
    if [file exists "output/$test_exe"] {
	exec "rm" "-r" "output/$test_exe"
    }
    file mkdir "output/$test_exe"

    if [catch {exec "./$test_exe" "-test" ">output/$test_exe/stdout" "2>output/$test_exe/stderr"} err ] {
 	puts "$err"
	incr g_failed
	incr g_total
    } else {
	source "output/$test_exe/stderr"
	lassign {test_suite unit_tests summary} $result
	set output [open "output/$test_exe/stdout" "r"]

	set all_clear 1
	foreach unit_test $unit_tests {
	    lassign {number name status} $unit_test

	    if [ string equal $status "P" ] {
		incr g_passed
	    } elseif [ string equal $status "F" ] {
		puts "$name failed, output in output/$test_exe/stdout"
		incr g_failed
		set all_clear 0
	    } elseif [ string equal $status "I" ] {
		seek $output 0
		set check_result [eval "check$name" $output]
		
		if [ expr $check_result < 0 ] {
		    puts "$name failed, output in output/$test_exe/stdout"
		    incr g_failed
		    set all_clear 0
		} else {
		    incr g_passed
		}
	    }
	    incr g_total
	}
	close $output

	if {$all_clear} {
	    puts_reg "passed"
	    set passed_summary [open "output/$test_exe/PASSED" "w"]
	    close $passed_summary
	}
    }
}

puts_reg "Total  : $g_total"
puts_reg "Passed : $g_passed"
puts_reg "Failed : $g_failed"
