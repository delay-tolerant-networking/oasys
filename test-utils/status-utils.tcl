#
# Various status for the running test processes
#

proc help {} {
    puts "commands: help status"
}

proc status {} {
    for {set i 0} {$i < $::net::nodes} {incr i} {
	set dir $::dist::distdirs($i)
	set hostname $::net::host($i)

	if [run::check_pid $hostname $::run::pids($i)] {
	    set status "running"
	} else {
	    set status "dead"
	}
	puts "host $i $hostname $dir $status"
    }
}

namespace eval status {
    proc get_distdirs {} {
	global ::dist::distdirs
	return [array get ::dist::distdirs]
    }
}