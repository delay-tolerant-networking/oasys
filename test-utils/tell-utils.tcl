
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
	do_until "connecting to $host:$port" $timeout {
	    if {![catch {tell::connect $host $port} err]} {
		return
	    }
	}
    }
    
    proc tell {host port cmd} {
	global tell::sockets

	
	if {![info exists sockets($host:$port)]} {
	    tell::connect $host $port
	}
	set sock $sockets($host:$port)
	
	regsub -all -- {\n} [string trim $cmd] {\\n} cmd
	puts $sock $cmd
	flush $sock
	
	set resultvec [gets $sock]
	set cmd_error [string index $resultvec 0]
	set result    [string range $resultvec 2 end]
	regsub -all -- {\\n} $result "\n" result
	
	if {$cmd_error == 0 || $result == ""} {
	    return $result
	} else {
	    set eol [string first "\n" $result]
	    set errorValue [string range $result 7 [expr $eol - 1]]
	    set errorInfo  [string range $result $eol end]
	    error $errorValue $errorInfo
	}
    }
}
