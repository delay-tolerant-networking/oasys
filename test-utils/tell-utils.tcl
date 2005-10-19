
#
# Send a tcl command to the console server running on host:port and
# return the result
#
proc tell {host port cmd} {
    return tell::tell $host $port $cmd
}

namespace eval tell {
    set tell_verbose 0
    set timeout 30000

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
	    error "error in tell_encode handshake: $ret2 != 'tell_encode'"
	}

	fconfigure $sock -blocking 0
	fileevent $sock readable "tell::response $host $port"
	set tell::sockets($host:$port) $sock

	if {$tell_verbose} {
	    puts "tell::connect success to $host:$port (sock $sock)"
	}
    }

    proc wait {host port {timeout 30000}} {
	global tell::sockets
	if {[info exists sockets($host:$port)]} {
	    return
	}
	
	do_until "connecting to $host:$port" $timeout {
	    if {![catch {tell::connect $host $port} err]} {
		return
	    }
	    after 1000
	}
    }

    proc tell {host port args} {
	global tell::sockets tell::results tell::tell_verbose tell::timeout

	if {![info exists sockets($host:$port)]} {
	    tell::connect $host $port
	}
	set sock $sockets($host:$port)

	if {[llength $args] == 1} {
	    set cmd [lindex $args 0]
	} else {
	    set cmd $args
	}
	
	if {$tell_verbose} {
	    puts "tell::tell: sending command \"$cmd\""
	}
	
	regsub -all -- {\n} [string trim $cmd] {\\n} cmd
	puts $sock $cmd
	flush $sock

	# set up a timeout in case there's no response
	set timer [after $tell::timeout "tell::timeout $host $port"]
	
	# wait for a response
	set tell::results($host:$port) ""
	while {$tell::results($host:$port) == ""} {
	    vwait tell::results($host:$port)
	}

	# cancel the timer (if it exists)
	catch {after cancel $timer}
	
	if {$tell_verbose} {
	    puts "tell::tell: got result $tell::results($host:$port)"
	}
	
	set cmd_error [string index $tell::results($host:$port) 0]
	set result    [string range $tell::results($host:$port) 2 end]
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

    proc close_socket {host port} {
	global tell::sockets tell::tell_verbose

	if {$tell_verbose} {
	    puts "tell:close_socket $host:$port"
	}
	
	catch {
	    fileevent $tell::sockets($host:$port) readable ""
	    close $tell::sockets($host:$port)
	}
	
	unset tell::sockets($host:$port)
    }
    
    proc response {host port} {
	global tell::sockets tell::results tell::tell_verbose
	set sock $tell::sockets($host:$port)

	set cmd_response [gets $sock]

	if {[eof $sock]} {
	    if {$tell_verbose} {
		puts "tell:eof $host:$port"
	    }

	    tell::close_socket $host $port
	    set tell::results($host:$port) \
		    "1 eof while waiting for tell response\n"
	    return
	}

	if {$cmd_response == ""} {
	    # short read, means there's not a full line but we should
	    # get another callback when more data arrives
	    puts " XXX ERROR: short read in tell_response"
	    return;
	}

	if {$tell_verbose} {
	    puts "tell:response $host:$port $cmd_response"
	}
	
	set tell::results($host:$port) $cmd_response
    }

    proc timeout {host port} {
	global tell::results tell::tell_verbose

	if {$tell_verbose} {
	    puts "tell:timeout $host:$port"
	}
	
	tell::close_socket $host $port
	set tell::results($host:$port) \
		"1 error: timeout while waiting for tell response\n"
    }
}
