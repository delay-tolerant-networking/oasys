#
# This file is converted into a big C string during the build
# process and evaluated in the command interpreter at startup
# time.
#

#
# For the vwait in event_loop to work, we need to make sure there's at
# least one event outstanding at all times, otherwise 'vwait forever'
# doesn't work
#
proc after_forever {} {
    after 1000000 after_forever
}

#
# Run the event loop and no command line interpreter
#
proc event_loop {} {
    global forever
    after_forever
    vwait forever
}

#
# Callback when there's data ready to be processed.
#
proc command_process {input output} {
    global command command_prompt command_info tell_encode

    # Grab the line, append it to the batched up command, and check if
    # it's complete
    if {[gets $input line] == -1} {
	if {"$input" == "stdin"} {
	    exit 0
	} else {
	    log /command debug "closed connection $command_info($input)"
	    catch {close $input}
	    return
	}
    }

    if {$line == "tell_encode"} {
	set tell_encode($output) 1
	puts $output "\ntell_encode"
	flush $output
	return
    } elseif {$line == "no_tell_encode"} {
	set tell_encode($output) 0
	puts $output "\nno_tell_encode"
	flush $output
	return
    }

    append command($input) $line
    if {![info complete $command($input)]} {
	return
    }
    
    # trim and evaluate the command
    set command($input) [string trim $command($input)]
    if {[catch {uplevel \#0 $command($input)} result]} {
	global errorInfo
	set result "error: $result\nwhile executing\n$errorInfo"
    }
    set command($input) ""

    if {$tell_encode($output)} {
	regsub -all -- {\n} $result {\\n} result
    }
    puts $output $result
    
    if {! $tell_encode($output)} {
	puts -nonewline $output $command_prompt
    }
    flush $output
}

#
# Run the simple (i.e. no tclreadline) command loop
#
proc simple_command_loop {prompt} {
    global command_prompt forever tell_encode
    set command_prompt "${prompt}% "
    
    puts -nonewline $command_prompt
    flush stdout
    
    set tell_encode(stdin)  0
    fileevent stdin readable "command_process stdin stdout"
    
    after_forever
    vwait forever
}

#
# Run the command loop with the given prompt
#
proc command_loop {prompt} {
    global command_prompt no_tclreadline

    if [info exists no_tclreadline] {
	simple_command_loop $prompt
	return
    }
    
    set command_prompt "${prompt}% "

    if [catch {
	package require tclreadline
	namespace eval tclreadline {
	    proc prompt1 {} {
		global command_prompt
		return $command_prompt
	    }
	}

	tclreadline::Loop
    } err] {
	log /tcl INFO "can't load tclreadline: $err"
	log /tcl INFO "fall back to simple command loop"
	simple_command_loop $prompt
    }
}

#
# Proc that's called when a new command connection arrives
#
proc command_connection {chan host port} {
    global command_info command_prompt tell_encode

    set command_info($chan) "$host:$port"
    set tell_encode($chan)  0
    log /command debug "new command connection $chan from $host:$port"
    fileevent $chan readable "command_process $chan $chan"

    puts -nonewline $chan $command_prompt
    flush $chan
}

#
# Run a command server on the given addr:port
#
proc command_server {prompt addr port} {
    global command_prompt
    set command_prompt "${prompt}% "
    socket -server command_connection -myaddr $addr $port 
}

#
# Define a bgerror proc to print the error stack when errors occur in
# event handlers
#
proc bgerror {err} {
    global errorInfo
    puts "tcl error: $err\n$errorInfo"
}

