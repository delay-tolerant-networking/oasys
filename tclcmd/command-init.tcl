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
# Callback when there's data ready to be processed on stdin.
#
proc command_process {} {
    global command command_prompt

    # Grab the line, append it to the batched up command, and check if
    # it's complete
    if {[gets stdin line] == -1} {
	exit 0
    }

    append command $line
    if {![info complete $command]} {
	return
    }

    # trim and evaluate the command
    set command [string trim $command]
    if {[catch {uplevel \#0 $command} result]} {
	global errorInfo
	puts "error: $result\nwhile executing\n$errorInfo"
    } elseif {$result != ""} {
	puts $result
    }
    
    set command ""
    puts -nonewline $command_prompt
    flush stdout
}
    
# Run the command loop with the given prompt
proc command_loop {prompt} {
    global command_prompt forever
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

	puts -nonewline $command_prompt
	flush stdout
	
	fileevent stdin readable command_process

	after_forever
	vwait forever
    }
}

# Define a bgerror proc to print the error stack when errors occur in
# event handlers
proc bgerror {err} {
    global errorInfo
    puts "tcl error: $err\n$errorInfo"
}

