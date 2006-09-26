#
# Various status for the running test processes
#

proc help {} {
    puts "commands: help status xterm"
}

proc status {} {
    foreach id [net::nodelist] {
	set dir $::dist::distdirs($id)
	set hostname $::net::host($id)

	if [run::check_pid $hostname $::run::pids($id)] {
	    set status "running"
	} else {
	    set status "dead"
	}
	puts "host $id $hostname $dir $status"
    }
}

proc xterm {args} {
    foreach id [net::nodelist] {
	set dir $::dist::distdirs($id)
	set hostname $::net::host($id)
	
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
