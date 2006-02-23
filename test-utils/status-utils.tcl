#
# Various status for the running test processes
#

proc help {} {
    puts "commands: help status xterm"
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

proc xterm {args} {
    for {set i 0} {$i < $::net::nodes} {incr i} {
	set dir $::dist::distdirs($i)
	set hostname $::net::host($i)
	
	# ssh will barf trying to lock the xauthority XXX/bowei --
	# this still has some mysterious problems and interactions
	# with the command prompt
	after 100
	
	eval "exec ssh -X $hostname $args \"cd $dir; xterm\" &"
    }
}

namespace eval status {
    proc get_distdirs {} {
	global ::dist::distdirs
	return [array get ::dist::distdirs]
    }
}