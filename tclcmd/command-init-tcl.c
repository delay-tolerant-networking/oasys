static const char* INIT_COMMAND = 
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
"    after 1000000 after_forever\n"
"}\n"
"\n"
"#\n"
"# Run the event loop and no command line interpreter\n"
"#\n"
"proc event_loop {} {\n"
"    global forever\n"
"    after_forever\n"
"    vwait forever\n"
"}\n"
"\n"
"#\n"
"# Callback when there's data ready to be processed.\n"
"#\n"
"proc command_process {input output} {\n"
"    global command command_prompt command_info\n"
"\n"
"    # Grab the line, append it to the batched up command, and check if\n"
"    # it's complete\n"
"    if {[gets $input line] == -1} {\n"
"	if {\"$input\" == \"stdin\"} {\n"
"	    exit 0\n"
"	} else {\n"
"	    log /command debug \"closed connection $command_info($input)\"\n"
"	    catch {close $input}\n"
"	    return\n"
"	}\n"
"    }\n"
"\n"
"    append command($input) $line\n"
"    if {![info complete $command($input)]} {\n"
"	return\n"
"    }\n"
"\n"
"    # trim and evaluate the command\n"
"    set command($input) [string trim $command($input)]\n"
"    if {[catch {uplevel \\#0 $command($input)} result]} {\n"
"	global errorInfo\n"
"	puts $output \"error: $result\\nwhile executing\\n$errorInfo\"\n"
"    } elseif {$result != \"\"} {\n"
"	puts $output $result\n"
"    }\n"
"    \n"
"    set command($input) \"\"\n"
"    puts -nonewline $output $command_prompt\n"
"    flush $output\n"
"}\n"
"\n"
"#\n"
"# Run the simple (i.e. no tclreadline) command loop\n"
"#\n"
"proc simple_command_loop {prompt} {\n"
"    global command_prompt forever\n"
"    set command_prompt \"${prompt}% \"\n"
"    \n"
"    puts -nonewline $command_prompt\n"
"    flush stdout\n"
"    \n"
"    fileevent stdin readable \"command_process stdin stdout\"\n"
"    \n"
"    after_forever\n"
"    vwait forever\n"
"}\n"
"\n"
"#\n"
"# Run the command loop with the given prompt\n"
"#\n"
"proc command_loop {prompt} {\n"
"    global command_prompt no_tclreadline\n"
"\n"
"    if [info exists no_tclreadline] {\n"
"	simple_command_loop $prompt\n"
"	return\n"
"    }\n"
"    \n"
"    set command_prompt \"${prompt}% \"\n"
"\n"
"    if [catch {\n"
"	package require tclreadline\n"
"	namespace eval tclreadline {\n"
"	    proc prompt1 {} {\n"
"		global command_prompt\n"
"		return $command_prompt\n"
"	    }\n"
"	}\n"
"\n"
"	tclreadline::Loop\n"
"    } err] {\n"
"	log /tcl INFO \"can't load tclreadline: $err\"\n"
"	log /tcl INFO \"fall back to simple command loop\"\n"
"	simple_command_loop $prompt\n"
"    }\n"
"}\n"
"\n"
"#\n"
"# Proc that's called when a new command connection arrives\n"
"#\n"
"proc command_connection {chan host port} {\n"
"    global command_info command_prompt\n"
"\n"
"    set command_info($chan) \"$host:$port\"\n"
"    log /command debug \"new command connection $chan from $host:$port\"\n"
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
"    set command_prompt \"${prompt}% \"\n"
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
