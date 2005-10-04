
#
# Send a tcl command to the console server running on host:port and
# return the result
#
proc tell {host port cmd} {
    return tell::tell $host $port $cmd
}

namespace eval tell {
    set tell_verbose 0

    proc verbose {{val ""}} {
	global tell::tell_verbose
	if {$val != ""} {
	    set tell_verbose $val
	}
	return $tell_verbose
    }
    
    proc connect {host port} {
	global tell::sockets tell::tell_verbose
	
	if {$tell_verbose} {
	    puts "tell:connect trying to connect to $host:$port"
	}
	
	set sock [socket $host $port]

	puts $sock "tell_encode"
	flush $sock
	set ret1 [gets $sock]; # the prompt
	set ret2 [gets $sock]; # the command response
	if {$ret2 != "tell_encode"} {
	    error "error in tell_encode handshake: $ret != 'tell_encode'"
	}

	set tell::sockets($host:$port) $sock

	if {$tell_verbose} {
	    puts "tell::connect success to $host:$port (sock $sock)"
	}
    }

    proc wait {host port {timeout 30000}} {
	do_until "connecting to $host:$port" $timeout {
	    if {![catch {tell::connect $host $port} err]} {
		return
	    }
	    after 1000
	}
    }
    
    proc tell {host port cmd} {
	global tell::sockets tell::tell_verbose

	if {![info exists sockets($host:$port)]} {
	    tell::connect $host $port
	}
	set sock $sockets($host:$port)
	
	if {$tell_verbose} {
	    puts "tell::tell: sending command \"$cmd\""
	}
	
	regsub -all -- {\n} [string trim $cmd] {\\n} cmd
	puts $sock $cmd
	flush $sock
	
	set resultvec [gets $sock]
	set cmd_error [string index $resultvec 0]
	set result    [string range $resultvec 2 end]
	regsub -all -- {\\n} $result "\n" result
	
	if {$cmd_error == 0 || $result == ""} {
	    if {$tell_verbose} {
		puts "tell::tell: result \"$result\""
	    }
	    
	    return $result
	} else {
	    set eol [string first "\n" $result]
	    set errorValue [string range $result 7 [expr $eol - 1]]
	    set errorInfo  [string range $result $eol end]
	    error $errorValue $errorInfo
	}
    }
}
