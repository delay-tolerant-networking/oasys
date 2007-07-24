static const char* INIT_COMMAND = 
"#\n"
"#    Copyright 2004-2006 Intel Corporation\n"
"# \n"
"#    Licensed under the Apache License, Version 2.0 (the \"License\");\n"
"#    you may not use this file except in compliance with the License.\n"
"#    You may obtain a copy of the License at\n"
"# \n"
"#        http://www.apache.org/licenses/LICENSE-2.0\n"
"# \n"
"#    Unless required by applicable law or agreed to in writing, software\n"
"#    distributed under the License is distributed on an \"AS IS\" BASIS,\n"
"#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
"#    See the License for the specific language governing permissions and\n"
"#    limitations under the License.\n"
"#\n"
"\n"
"#\n"
"# This file is converted into a big C string during the build\n"
"# process and evaluated in the command interpreter at startup\n"
"# time.\n"
"#\n"
"\n"
"#\n"
"# For the vwait in event_loop to work, we need to make sure there's at\n"
"# least one event outstanding at all times, otherwise 'vwait forever'\n"
"# doesn't work\n"
"#\n"
"proc after_forever {} {\n"
"    global forever_timer\n"
"    set forever_timer [after 1000000 after_forever]\n"
"}\n"
"\n"
"#\n"
"# Run the event loop and no command line interpreter\n"
"#\n"
"proc event_loop {} {\n"
"    global event_loop_wait\n"
"    after_forever\n"
"\n"
"    set event_loop_wait 1\n"
"    while {$event_loop_wait} {\n"
"	after 1000 { \n"
"	    global event_loop_wait\n"
"	    if {$event_loop_wait} { set event_loop_wait 1 }\n"
"	}\n"
"	vwait event_loop_wait\n"
"    }\n"
"    command_log notice \"exiting event loop\"\n"
"}\n"
"\n"
"proc do_nothing {} {\n"
"}\n"
"\n"
"#\n"
"# Kill the event loop\n"
"#\n"
"proc exit_event_loop {} {\n"
"    global forever_timer event_loop_wait stdin\n"
"    command_log notice \"kicking event loop to exit\"\n"
"    set event_loop_wait 0\n"
"    if [catch {\n"
"	::tclreadline::readline eof\n"
"    } err] {\n"
"    }\n"
"    after 0 do_nothing\n"
"}\n"
"\n"
"#\n"
"# Global var containing the command log path\n"
"#\n"
"set command_logpath \"/command\"\n"
"\n"
"#\n"
"# Wrapper proc to handle the fact that we may or may not have a log\n"
"# procedure defined\n"
"#\n"
"proc command_log {level string} {\n"
"    global command_logpath\n"
"    if {[info commands log] != \"\"} {\n"
"        log $command_logpath $level $string\n"
"    } else {\n"
"        # hard-coded level filter\n"
"        switch -- $level {\n"
"            notice - warn - error - crit {\n"
"                puts $string\n"
"            }\n"
"        }\n"
"    }\n"
"}\n"
"\n"
"#\n"
"# Callback when there's data ready to be processed.\n"
"#\n"
"proc command_process {input output} {\n"
"    global command command_prompt command_info tell_encode event_loop_wait\n"
"\n"
"    # Grab the line, and check for eof\n"
"    if {[gets $input line] == -1} {\n"
"	if {\"$input\" == \"stdin\"} {\n"
"	    set event_loop_wait 1\n"
"	    return\n"
"	} else {\n"
"	    command_log debug \"closed connection $command_info($input)\"\n"
"	    fileevent $input readable \"\"\n"
"	    catch {close $input}\n"
"	    return\n"
"	}\n"
"    }\n"
"\n"
"    # handle exit from a socket connection\n"
"    if {($input != \"stdin\") && ($line == \"exit\")} {\n"
"	command_log notice \"connection $command_info($input) exiting\"\n"
"	fileevent $input readable \"\"\n"
"	catch {close $input}\n"
"	return\n"
"    }\n"
"    \n"
"    # handle tell_encode / no_tell_encode commands\n"
"    if {$line == \"tell_encode\"} {\n"
"	set tell_encode($output) 1\n"
"	puts $output \"\\ntell_encode\"\n"
"	flush $output\n"
"	return\n"
"    } elseif {$line == \"no_tell_encode\"} {\n"
"	set tell_encode($output) 0\n"
"	puts $output \"\\nno_tell_encode\"\n"
"	flush $output\n"
"	return\n"
"    }\n"
"\n"
"    if {$tell_encode($output)} {\n"
"	# if we're in tell encoding mode, decode the message\n"
"\n"
"	if {$command($input) != \"\"} {\n"
"	    error \"unexpected partial command '$command($input)' in tell mode\"\n"
"	}\n"
"	regsub -all -- {\\\\n} $line \"\\n\" command($input)\n"
"    } else {\n"
"	# otherwise, append the line to the batched up command, and\n"
"	# check if it's complete\n"
"	\n"
"	append command($input) $line\n"
"	if {![info complete $command($input)]} {\n"
"	    return\n"
"	}\n"
"    }\n"
"    \n"
"    # trim and evaluate the command\n"
"    set command($input) [string trim $command($input)]\n"
"    set cmd_error 0\n"
"\n"
"    command_log debug \"executing command $command($input)\"\n"
"    if {[catch {uplevel \\#0 $command($input)} result]} {\n"
"        command_log debug \"error result is $result\"\n"
"	if {$result == \"exit_command\"} {\n"
"	    if {$input == \"stdin\"} {\n"
"		set event_loop_wait 1\n"
"		return\n"
"	    } else {\n"
"		real_exit\n"
"	    }\n"
"	}\n"
"	global errorInfo\n"
"	set result \"error: $result\\nwhile executing\\n$errorInfo\"\n"
"	set cmd_error 1\n"
"    }\n"
"    command_log debug \"result is $result\"\n"
"    set command($input) \"\"\n"
"\n"
"    if {$tell_encode($output)} {\n"
"	regsub -all -- {\\n} $result {\\\\n} result\n"
"	puts $output \"$cmd_error $result\"\n"
"    } else {\n"
"	puts $output $result\n"
"    }    \n"
"    \n"
"    if {! $tell_encode($output)} {\n"
"	puts -nonewline $output $command_prompt\n"
"    }\n"
"    flush $output\n"
"}\n"
"\n"
"#\n"
"# Run the simple (i.e. no tclreadline) command loop\n"
"#\n"
"proc simple_command_loop {prompt} {\n"
"    global command command_prompt forever tell_encode\n"
"    set command_prompt \"$prompt\"\n"
"    \n"
"    puts -nonewline $command_prompt\n"
"    flush stdout\n"
"\n"
"    set command(stdin)      \"\"\n"
"    set tell_encode(stdout) 0\n"
"    set event_loop_wait        0\n"
"    fileevent stdin readable \"command_process stdin stdout\"\n"
"\n"
"    vwait event_loop_wait\n"
"\n"
"    command_log notice \"exiting simple command loop\"\n"
"}\n"
"\n"
"#\n"
"# Run the command loop with the given prompt\n"
"#\n"
"proc command_loop {prompt} {\n"
"    global command_prompt event_loop_wait\n"
"    \n"
"    set command_prompt \"$prompt\"\n"
"    set event_loop_wait 0\n"
"\n"
"    # Handle the behavior that we want for the 'exit' proc -- when running\n"
"    # as the console loop (either tclreadline or not), we just want it to\n"
"    # exit the loop so the caller knows to clean up properly. To implement\n"
"    # that, we error with the special string \"exit_command\" which is\n"
"    # caught by callers who DTRT with it.\n"
"    rename exit real_exit\n"
"    proc exit {} {\n"
"	error \"exit_command\"\n"
"    }\n"
"\n"
"    if [catch {\n"
"	package require tclreadline\n"
"	tclreadline::readline initialize \"\"\n"
"	tclreadline::readline eofchar \"error exit_command\"\n"
"	tclreadline_loop\n"
"	\n"
"    } err] {\n"
"	command_log info \"can't load tclreadline: $err\"\n"
"	command_log info \"fall back to simple command loop\"\n"
"	simple_command_loop $prompt\n"
"    }\n"
"    puts \"\"\n"
"\n"
"    # fix up the exit proc\n"
"    rename exit \"\"\n"
"    rename real_exit exit\n"
"}\n"
"\n"
"#\n"
"#\n"
"proc tclreadline_completer {text start end line} {\n"
"    global event_loop_wait\n"
"    if {$event_loop_wait == 1} {\n"
"	error \"exit_command\"\n"
"    }\n"
"    return \"\"\n"
"}\n"
"\n"
"#\n"
"# Custom main loop for tclreadline (allows us to exit on eof)\n"
"# Copied from tclreadline's internal Loop method\n"
"#\n"
"proc tclreadline_loop {} {\n"
"    global event_loop_wait\n"
"    \n"
"    tclreadline::readline builtincompleter 0\n"
"    tclreadline::readline customcompleter tclreadline_completer\n"
"    \n"
"    uplevel \\#0 {\n"
"	while {1} {\n"
"	    if {$event_loop_wait == 1} {\n"
"		return\n"
"	    }\n"
"	    \n"
"	    if [info exists tcl_prompt2] {\n"
"		set prompt2 $tcl_prompt2\n"
"	    } else {\n"
"		set prompt2 \">\"\n"
"	    }\n"
"\n"
"	    if {[catch {\n"
"		set LINE [::tclreadline::readline read $command_prompt]\n"
"		while {![::tclreadline::readline complete $LINE]} {\n"
"		    append LINE \"\\n\"\n"
"		    append LINE [tclreadline::readline read ${prompt2}]\n"
"		}\n"
"		\n"
"	    } errorMsg]} {\n"
"		if {$errorMsg == \"exit_command\"} {\n"
"		    break\n"
"		}\n"
"		puts stderr \"tclreadline_loop error. $errorMsg\"\n"
"		continue\n"
"	    }\n"
"\n"
"	    # Magnus Eriksson <magnus.eriksson@netinsight.se> proposed\n"
"	    # to add the line also to tclsh's history.\n"
"	    #\n"
"	    # I decided to add only lines which are different from\n"
"	    # the previous one to the history. This is different\n"
"	    # from tcsh's behaviour, but I found it quite convenient\n"
"	    # while using mshell on os9.\n"
"	    #\n"
"	    if {[string length $LINE] && [history event 0] != $LINE} {\n"
"		history add $LINE\n"
"	    }\n"
"\n"
"	    if [catch {\n"
"		\n"
"		set result [eval $LINE]\n"
"		if {$result != \"\"} {\n"
"		    puts $result\n"
"		}\n"
"		set result \"\"\n"
"	    } ::tclreadline::errorMsg] {\n"
"		if {$::tclreadline::errorMsg == \"exit_command\"} {\n"
"		    break\n"
"		}\n"
"		puts stderr $::tclreadline::errorMsg\n"
"		puts stderr [list while evaluating $LINE]\n"
"	    }\n"
"	}\n"
"    }\n"
"}\n"
"\n"
"\n"
"#\n"
"# Proc that's called when a new command connection arrives\n"
"#\n"
"proc command_connection {chan host port} {\n"
"    global command command_info command_prompt tell_encode\n"
"\n"
"    set command_info($chan) \"$host:$port\"\n"
"    set command($chan)      \"\"\n"
"    set tell_encode($chan)  0\n"
"    command_log debug \"new command connection $chan from $host:$port\"\n"
"    fileevent $chan readable \"command_process $chan $chan\"\n"
"\n"
"    puts -nonewline $chan $command_prompt\n"
"    flush $chan\n"
"}\n"
"\n"
"#\n"
"# Run a command server on the given addr:port\n"
"#\n"
"proc command_server {prompt addr port} {\n"
"    global command_prompt\n"
"    set command_prompt \"$prompt\"\n"
"    socket -server command_connection -myaddr $addr $port \n"
"}\n"
"\n"
"#\n"
"# Define a bgerror proc to print the error stack when errors occur in\n"
"# event handlers\n"
"#\n"
"proc bgerror {err} {\n"
"    global errorInfo\n"
"    puts \"tcl error: $err\\n$errorInfo\"\n"
"}\n"
"\n"
;
