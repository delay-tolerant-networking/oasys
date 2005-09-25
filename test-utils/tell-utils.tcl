
#
# Send a tcl command to the console server running on host:port and
# return the result
#
proc tell {host port cmd} {
    return tell::tell $host $port $cmd
}

namespace eval tell {
    proc connect {host port} {
	global tell::sockets

	set sock [socket $host $port]
	puts $sock "tell_encode"
	flush $sock
	set ret1 [gets $sock]; # the prompt
	set ret2 [gets $sock]; # the command response
	if {$ret2 != "tell_encode"} {
	    error "error in tell_encode handshake: $ret != 'tell_encode'"
	}

	set tell::sockets($host:$port) $sock
    }

    proc wait {host port {timeout 30000}} {
	set start [clock clicks -milliseconds]
	while {[clock clicks -milliseconds] < $start + $timeout} {
	    if [catch {tell::connect $host $port} err] {
		continue
	    }

	    return
	}
	
	error "timeout connecting to $host:$port"
    }
    
    proc tell {host port cmd} {
	global tell::sockets
	
	if {![info exists sockets($host:$port)]} {
	    tell::connect $host $port
	}

	set sock $sockets($host:$port)
	puts $sock $cmd
	flush $sock
	set result [gets $sock]

	regsub -all -- {\\n} $result "\n" result
	return $result
    }
}
