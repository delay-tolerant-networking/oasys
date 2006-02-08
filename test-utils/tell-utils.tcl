
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

	set sock "none"
	if [catch {
	    set sock [socket $host $port]
	} err] {
	    global errorInfo
	    if {$tell_verbose} {
		puts "tell::connect error connecting to $host:$port (sock $sock)"
	    }
	    error $err $errorInfo
	}

	puts $sock "tell_encode"
	flush $sock
	set ret1 [gets $sock]; # the prompt
	set ret2 [gets $sock]; # the command response
	if {$ret2 != "tell_encode"} {
	    error "error in tell_encode handshake: $ret2 != 'tell_encode'"
	}

	fconfigure $sock -blocking 0
	fileevent $sock readable "tell::response $sock $host $port"
	set sockets($host:$port) $sock

	if {$tell_verbose} {
	    puts "tell::connect success to $host:$port (sock $sock)"
	}
    }

    proc wait {host port {timeout 30000}} {
	global tell::sockets tell::tell_verbose
	
	if {[info exists sockets($host:$port)]} {
	    return
	}

	if {$tell_verbose} {
	    puts "tell::wait: no socket to $host:$port, connecting"
	}
	
	do_until "connecting to $host:$port" $timeout {
	    if {![catch {tell::connect $host $port} err]} {
		if {$tell_verbose} {
		    puts "tell::wait: connected to $host:$port (sock $sockets($host:$port))"
		}
		return
	    }
	    after 1000
	}
    }

    proc tell {host port args} {
	global tell::sockets tell::results tell::tell_verbose tell::timeout

	if {![info exists sockets($host:$port)]} {
	    if {$tell_verbose} {
		puts "tell::tell: no socket to $host:$port, connecting"
	    }
	    tell::connect $host $port
	}
	set sock $sockets($host:$port)

	if {[llength $args] == 1} {
	    set cmd [lindex $args 0]
	} else {
	    set cmd $args
	}
	
	if {$tell_verbose} {
	    puts "tell::tell: sending command \"$cmd\" to $host:$port"
	}
	
	regsub -all -- {\n} [string trim $cmd] {\\n} cmd
	puts $sock $cmd
	flush $sock

	# set up a timeout in case there's no response
	set timer [after $tell::timeout "tell::timeout $sock $host $port"]
	
	# wait for a response
	set tell::results($host:$port) ""
	while {$tell::results($host:$port) == ""} {
	    vwait tell::results($host:$port)
	}

	# cancel the timer (if it exists)
	catch {after cancel $timer} err
	
	if {$tell_verbose} {
	    puts "tell::tell: got result '$tell::results($host:$port)' from $host:$port"
	}
	set cmd_error [string index $tell::results($host:$port) 0]
	set result    [string range $tell::results($host:$port) 2 end]
	regsub -all -- {\\n} $result "\n" result

	# special case a command of "shutdown" by closing the tell
	# socket proactively since we expect the other side to shut
	# down as well
	if {[lindex $cmd 0] == "shutdown"} {
	    close_socket $host $port
	}

	if {$cmd_error == 0 || $result == ""} {
	    if {$tell_verbose} {
		puts "tell::tell: result \"$result\""
	    }
	    
	    return $result
	} else {
	    set eol [string first "\n" $result]
	    set info  [string range $result $eol end]
	    if {$result == ""} {
		set result "(no additional information)"
	    }
	    puts "tell error '$result' '$info'"
	    error $result $info
	}
    }

    proc close_socket {host port} {
	global tell::sockets tell::tell_verbose

	if {$tell_verbose} {
	    puts "tell:close_socket $host:$port"
	}
	
	catch {
	    fileevent $tell::sockets($host:$port) readable ""
	}

	catch {
	    close $tell::sockets($host:$port)
	}
	
	unset tell::sockets($host:$port)
    }
    
    proc response {sock host port} {
	global tell::sockets tell::results tell::tell_verbose

	if {![info exists sockets($host:$port)] ||\
		$sock != $sockets($host:$port)} {
	    puts "XXX WARNING: bogus fileevent for stale socket $sock ($host:$port)"
	    return
	}

	set cmd_response [gets $sock]

	if {($cmd_response == "") && [eof $sock]} {
	    puts "warning: eof in tell command to $host:$port"

	    tell::close_socket $host $port
	    set results($host:$port) "1 error: eof while waiting for tell response"
	    return
	}
	
	if {$cmd_response == ""} {
	    # short read, means there's not a full line but we should
	    # get another callback when more data arrives
	    puts " XXX ERROR: short read in tell_response"
	    return;
	}

	if {$tell_verbose} {
	    puts "tell::response $host:$port '$cmd_response'"
	}
	
	set results($host:$port) $cmd_response
    }

    proc timeout {sock host port} {
	global tell::sockets tell::results tell::tell_verbose

	if {![info exists sockets($host:$port)] ||\
		$sock != $sockets($host:$port)} {
	    puts "XXX WARNING: bogus timeout for stale socket $sock ($host:$port)"
	    return
	}
	
	puts "error: timeout in tell command to $host:$port ($sock)"
	
	tell::close_socket $host $port
	set tell::results($host:$port) "1 error: timeout while waiting for tell response"
    }
}
