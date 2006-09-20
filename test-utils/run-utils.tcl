#!/usr/bin/tclsh

# Try to set up a signal handler for Control-C, which requires the
# Tclx package
if [catch {
    package require Tclx
    if {[info procs log] != ""} {
	rename log tclx_log
    }

    # Trap SIGINT
    signal trap SIGINT { 
	set opt(leave_crap) 0
	puts "* Caught SIGINT, cleaning up"
	run::cleanup
	if {[info commands real_exit] != ""} {
	    real_exit
	} else {
	    exit
	}
    }
} err] {
    puts "* error setting up signal handler: $err"
}

namespace eval run {

proc dbg { msg } {
    global opt
    if {$opt(verbose) > 0} {
	puts $msg
    }
}

proc shift { l } {
    upvar $l xx
    set xx [lrange $xx 1 end]
}

proc arg0 { l } {
    return [lindex $l 0]
}

proc usage {} {
    puts "Options:"
    puts "    -h | -help | --help  Print help message"
    puts "    -g | -gdb  | --gdb   Run program with gdb"
    puts "    -vg | --valgrind     Run program with valgrind's memcheck"
    puts "    -x | -xterm          Run each instance in an xterm"
    puts "    -d | -daemon         Daemon mode (no command loop)"
    puts "    -geom | -geometry    Specify the xterm geometry (implies -x)"
    puts "    -net  | --net <file> Select the net file"
    puts "    -id   | --id  <id>   Id which is used to select a port slice"
    puts "    -l <log dir>         Set a different directory for logs"
    puts "    -n <# nodes>         Number of nodes, unless overridden by test"
    puts "    -p                   Pause after apps die, applies only to"
    puts "                         xterm option"
    puts "    -r <rundir>          /tmp/<rundir> will be the test directory"
    puts "    -v                   Verbose mode"
    puts ""
    puts "Extended options:"
    puts "    --extra-gdbrc <script>     Add an extra gdbrc to be run"
    puts "    --gdb-opts    <options...> Extra options to gdb"
    puts "    --extra-exe   <options...> Add extra options to the command line"
    puts "                               of a specified exe: exe_name=options...;"
    puts "    --alt-gdb     <alt gdb>    Use an alternative debugger"
    puts "    --opts        <options...> Extra options to the program"
    puts "    --gdbrc       <tmpl>       Change remote gdbrc template"
    puts "    --script      <tmpl>       Change remote run script template"
    puts "    --no-logs                  Don't collect logs/cores"
    puts "    --local-rundir		 Use ./run-0 ./run-1 etc instead of /tmp"
    puts "    --leave-crap               Leave all crap in /tmp dir"
    puts "    --strip                    Strip execs before copying"
    puts "    --valgrind-suppressions    Generate valgrind error suppressions"
    puts "    --base-test-dir <dir>      Base Directory for test scripts"
    puts "    --seed <seed>              Use the specified random seed"
    puts "    --dry-run                  Don't really distribute files and run"
}

proc init {argv} {
    global opt

    set opt(conf_id)       0
    set opt(daemon)        0
    set opt(gdb)           0
    set opt(valgrind)      0
    set opt(valgrind_suppressions) 0
    set opt(leave_crap)    0
    set opt(logdir)        ""
    set opt(net)           ""
    set opt(no_logs)       0
    set opt(pause)         0
    set opt(rundir_prefix) "/tmp/run-[pid]"
    set opt(local_rundir)  0
    set opt(strip)         0
    set opt(verbose)       0
    set opt(xterm)         0
    set opt(dry_run)       0
    
    set opt(gdb_extra)     ""
    set opt(exe_extra)     ""
    set opt(gdb_exec)      "gdb"
    set opt(gdbopts)       ""
    set opt(opts)          ""
    set opt(geometry)      ""
    set opt(gdb_tmpl)      [import_find gdbrc.template]
    set opt(script_tmpl)   [import_find script.template]
    set opt(base_test_dir) ""
    set opt(seed)          [clock seconds]

    set num_nodes_override 0
    set test_script ""
    
    # Load default options if they exist -- You can override options
    # if you want to with command line arguments.
    if [file readable "~/.debug_opts"] {
	set f [open "~/.debug_opts" "r"]
	
	set new_argv [read -nonewline $f]
	set argv [concat $new_argv $argv]
	close $f
    }

    # parse options
    while {[llength $argv] > 0} {
	switch -- [arg0 $argv] {
	    -h            -
	    -help         -
	    --help        { usage; exit }
	    -g            -
	    -gdb          -
	    --gdb         { set opt(gdb) 1 }
	    -vg           -
	    -valgrind     -
	    --valgrind    { set opt(valgrind) 1 }
	    -x            -
	    -xterm        -
	    --xterm       { set opt(xterm) 1 }
	    -d            -
	    -daemon       -
	    --daemon      { set opt(daemon) 1 }
	    -geom         -
	    -geometry     -
	    --geometry    { shift argv; set opt(geometry) [arg0 $argv] }
	    -l            { shift argv; set opt(logdir) [arg0 $argv] }
	    -p            { set opt(pause) 1}
	    -r            { shift argv; set opt(rundir_prefix) [arg0 $argv] }
	    --local-rundir { set opt(local_rundir) 1}
	    -v            { set opt(verbose) 1}
	    -n            { shift argv; set num_nodes_override [arg0 $argv] }
	    -net          -
	    --net         { shift argv; set opt(net)         [arg0 $argv] }
	    -id           -
	    --id          { shift argv; set opt(conf_id)     [arg0 $argv] }
	    --extra-gdbrc { shift argv; set opt(gdb_extra)   [arg0 $argv] }
	    --extra-exe   { shift argv; set opt(exe_extra)   [arg0 $argv] }
	    --gdb-opts    { shift argv; set opt(gdbopts)     [arg0 $argv] }
	    --alt-gdb     { shift argv; set opt(gdb_exec)    [arg0 $argv] }
	    --opts        { shift argv; set opt(opts)        [arg0 $argv] }
	    --gdb-tmpl    { shift argv; set opt(gdb_tmpl)    [arg0 $argv] }
	    --script-tmpl { shift argv; set opt(script_tmpl) [arg0 $argv] }
	    --base-test-dir { shift argv; set opt(base_test_dir) [arg0 $argv] }
	    --seed 	  { shift argv; set opt(seed) [arg0 $argv] }
	    --no-logs     { set opt(no_logs) 1 }
	    --leave-crap  { set opt(leave_crap) 1}
	    --strip       { set opt(strip) 1 }
	    --valgrind-suppressions { set opt(valgrind_suppressions) 1}
	    --dry-run     { set opt(dry_run) 1 }
	    
	    default  {
		if {([string index [arg0 $argv] 0] != "-") && \
			($test_script == "") } {
		    set test_script [arg0 $argv]
		} else {
		    puts "illegal option \"[arg0 $argv]\""; usage; exit
		}
	    }
	}
	shift argv
    }

    if {$opt(valgrind) && $opt(gdb)} {
	puts "ERROR: cannot use both valgrind option and gdb option";
	exit 1
    }

    puts "* Reading net definition file $opt(net)"
    import $opt(net)
    
    if {$num_nodes_override != 0} {
	puts "* Setting num_nodes to $num_nodes_override"
	net::num_nodes $num_nodes_override
    }

    #
    # Check and output the random number seed
    #
    puts "* Using random number generator seed $opt(seed)"
    expr srand($opt(seed))

    #
    # Check if the test script is really in the base_test_dir
    # 
    if {! [file readable $test_script]} {
	set script2 [file join $opt(base_test_dir) $test_script]
	if {[file readable $script2]} {
	    set test_script $script2
	} else {
	    error "can't read test script file $test_script"
	}
    }
    
    puts "* Reading test script $test_script"
    uplevel \#0 source $test_script

    if { $opt(dry_run) } {
	puts "* Distributing files"
    } else {
	puts "* Generating script files"
    }

    dist::files $manifest::manifest [net::hostlist] [pwd] \
	$manifest::subst $opt(strip) $opt(verbose)
}

proc process_template {template var_array} {
    upvar $var_array ar

    dbg "% processing template \"$template\" with \"$var_array\""

    set tmpl [open $template "r"]
    set script [read -nonewline $tmpl]
    close $tmpl
    
    foreach {k v} [array get ar] {
	regsub -all "%$k%" $script $v script
    }

    return "$script\n"
}

proc generate_script {id exec_file exec_opts confname conf exec_env} {
    global opt net::host test::testname

    set hostname $net::host($id)

    set rundir [dist::get_rundir $hostname $id]
    
    # Handle extra options
    set name [file tail $exec_file]
    if {$opt(exe_extra) != ""} {
	dbg "% extra exe options $opt(exe_extra), exe name $name"
	regexp "$name=\(.*\);" $opt(exe_extra) dummy the_opts
	if [info exists the_opts] {
	    dbg "% extra options for $name = \"$the_opts\""
	    append exec_opts $the_opts
	}
    }
    
    # runscript
    set script(exec_file)   $exec_file
    set script(exec_name)   [file tail $exec_file]
    set script(exec_opts)   $exec_opts
    set script(gdb_opts)    $opt(gdbopts)
    set script(run_dir)     $rundir
    set script(run_id)      $script(exec_name)-$hostname-$id
    set script(gdb_exec)    $opt(gdb_exec)
    set script(gdb)         $opt(gdb)
    set script(valgrind)    $opt(valgrind)
    set script(local)       [net::is_localhost $hostname]
    set script(pause_after) $opt(pause)
    set script(verbose)     $opt(verbose)
    set script(xterm)       $opt(xterm)
    set script(geometry)    $opt(geometry)

    if {$opt(valgrind_suppressions)} {
	set script(valgrind_opts) "--gen-suppressions=all"
    } else {
	set script(valgrind_opts) ""
    }

    # environmental variables:
    set env_commands ""
    foreach {var val} $exec_env {
	set env_commands "$env_commands\nexport $var=$val"
    }
    set script(exec_env)    $env_commands

    # run script
    set runscript [process_template $opt(script_tmpl) script]    
    dbg "% runscript = \n$runscript"

    # debug script
    set gdb(exec_opts)      $exec_opts
    set gdb(gdb_extra)      $opt(gdb_extra)
    set gdb(gdb_test_extra) [conf::get gdb $id]

    set gdbscript [process_template $opt(gdb_tmpl) gdb]    
    dbg "% gdbscript = \n$gdbscript"

    set run_base "run-$script(exec_name)"
    
    write_script $id $rundir $run_base.sh  $runscript true
    write_script $id $rundir $run_base.gdb $gdbscript true

    if {$confname != ""} {
	write_script $id $rundir $confname $conf false
    }
}

proc run_cmd {hostname args} {
    if [net::is_localhost $hostname] {
	dbg "% [join $args]"
	return [eval exec $args < /dev/null]
    } else {
	dbg "% ssh $hostname [join $args]"
	return [eval exec ssh $hostname $args < /dev/null]
    }
}

proc write_script {id dir filename contents do_chmod} {
    global net::host
    set hostname $net::host($id)
    set path [file join $dir $filename]

    if [net::is_localhost $hostname] {
	dbg "% cat > $path << $contents"
	exec cat > $path << $contents
    } else {
	dbg "% ssh $hostname cat > $path << $contents"
	exec ssh $hostname "cat > $path" << $contents
    }
    run_cmd $hostname chmod +x $path
}

#
# Run a given program on the specified list of nodes
#
proc run {id exec_file exec_opts confname conf exec_env} {
    global opt manifest::manifest manifest::subst 
    global run::pids run::dirs run::xterm

    set hostname $net::host($id)

    set exec_name [file tail $exec_file]

    dbg "* Generating scripts for $exec_name for $hostname:$id"
    generate_script $id $exec_file $exec_opts $confname $conf $exec_env

    set run::dirs($id) [dist::get_rundir $hostname $id]
    set script "$run::dirs($id)/run-$exec_name.sh"

    if {$opt(geometry) != ""} {
	set geometry "-geometry $opt(geometry)"
    } else {
	set geometry ""
    }

    puts "* Running $exec_name on $hostname:$id"

    # XXX/demmer get rid of the run::xterm useless vars
    
    switch "[net::is_localhost $hostname] $opt(xterm)" {
	"1 1" {
	    set cmd "xterm -title \"$hostname-$id $exec_name\" $geometry \
		    -e $script"
	    set run::xterm($id) 1
	}
	"0 1" {
	    set cmd "xterm -title \"$hostname-$id $exec_name\" $geometry \
		    -e ssh -t $hostname $script"
	    set run::xterm($id) 1
	}
	"1 0" {
	    set cmd "sh $script < /dev/null"
	    set run::xterm($id) 0
	}
	"0 0" {
	    set cmd "ssh $hostname sh $script < /dev/null"
	    set run::xterm($id) 0
	}
    }

    dbg "% $hostname:$id exec $exec_name -- cmd '$cmd'"
    eval exec $cmd &

    do_until "getting exec pid" 10000 {
	if {![catch {
	    set exec_pid [run_cmd $hostname cat $run::dirs($id)/$exec_name.pid]
	} err]} {
	    return
	}
	after 500
    }

    dbg "% $hostname:$id exec $exec_name -- pid $exec_pid'"
    
    run_cmd $hostname /bin/rm -f $run::dirs($id)/$exec_name.pid
    
    lappend run::pids($id) $exec_pid
    return $exec_pid
}

#
# Add the pid for a given program that damonizes itself to the
# one contained in the given run directory file
#
proc add_pid {id pid_filename} {
    global run::dirs run::pids

    set hostname $net::host($id)
    set pid [run_cmd $hostname cat $run::dirs($id)/$pid_filename]
    lappend run::pids($id) $pid
    return $pid
}

#
# Check if the given pid is alive on the specified hostname
#
proc check_pid {hostname pid} {
    global net::host tcl_platform

    set procs ""

    if {$tcl_platform(os) == "Linux"} {
	# under linux, ps -h suppresses the header line, and returns
	# an error code (as checked in the catch block) if the process
	# can't be found, but if the command succeeds, we can immediately
	# return since there must be only one line of output
	if [catch {
	    set procs [run_cmd $hostname ps h -p $pid]
	} err] {
	    return 0
	}

	return 1

    } elseif {$tcl_platform(platform) == "windows"} {
	# on windows we have to get the whole list and scan it below
	set procs [run_cmd $hostname ps -s]

    } else {
	# on other unix systems, we can run BSD style ps and parse its
	# output below to skip the header line
	if [catch {
	    set procs [run_cmd $hostname ps -p $pid]
	} err] {
	    return 0
	}
    }

    # now check the list to try to find the pid
    foreach pidline [split $procs "\n"] {
	set pid2 [lindex $pidline 0]
	if {$pid2 == $pid} {
	    return 1
	}
    }
	
    return 0

}

#
# Wait for the PID to exit
#
proc wait_for_pid_exit {id pid {timeout 30000}} {
    global net::host
    do_until "waiting for PID $pid to exit" $timeout {
	if {![check_pid $net::host($id) $pid]} {
	    return
	}
	after 1000
    }
}

#
# Kill the specified pid on the host with the given signal
#
proc kill_pid {id pid signal} {
    global net::host
    return [catch {run_cmd $net::host($id) kill -s $signal $pid} err]
}

#
# Wait for all running test programs to exit
#
proc wait_for_programs {{timeout 5000}} {
    global net::host run::pids
    set num_alive 0

    puts "* Waiting for spawned programs to exit"

    foreach signal {none TERM KILL} {
	set start [clock clicks -milliseconds]
	
	while {[clock clicks -milliseconds] < $start + $timeout} {
	    set num_alive 0
	    
	    for {set i 0} {$i < [net::num_nodes]} {incr i} {
		set hostname $net::host($i)
		set livepids {}

		# check if all pids have died
		if {![info exists run::pids($i)] || \
			[llength run::pids($i)] == 0} {
		    continue
		}

		dbg "% pids on $hostname:$i: $run::pids($i)"
		foreach pid $run::pids($i) {
		    if {[check_pid $hostname $pid]} {
			dbg "% $hostname:$i pid $pid still alive"
			if {$signal != "none"} {
			    puts "* ERROR: pid $pid on host $hostname:$i\
				    still alive"
			    puts "* ERROR: sending $signal signal"
			    kill_pid $i $pid $signal
			}
			lappend livepids $pid
		    } else {
			dbg "% $hostname:$i pid $pid exited"
		    }
		}

		if {[llength $livepids] == 0} {
		    puts "    $hostname:$i -- all processes finished"
		} else {
		    puts "    $hostname:$i -- [llength livepids] pid(s)\
			    still alive ($livepids)"
		    set run::pids($i) $livepids
		    incr num_alive [llength $livepids]
		}
	    }
	    
	    if {$num_alive == 0} {
		puts "* All programs done"
		return
	    }

	    after 1000
	}
    }

    error "$num_alive pids still exist after sending all kill signals"
}

proc get_files {varname hostname dir pattern} {
    upvar $varname var

    set files ""
    if {$hostname == "localhost"} {
	set files [glob -nocomplain $dir/$pattern]
    } else {
	catch {
	    set files [run_cmd $hostname ls -1 $dir/$pattern]
	}
    }

    if {$files != ""} {
	set var [concat $var [split $files]]
    }
}

#
# Collect the logs/cores left by all of the executing processes
# 
proc collect_logs {} {
    global opt net::host run::dirs

    if {$opt(no_logs)} { return }

    puts "* Collecting logs/cores into $opt(logdir)"
    if {! [file isdirectory $opt(logdir)]} {
	set opt(logdir) ""
    }

    for {set i 0} {$i < [net::num_nodes]} {incr i} {
	set cores {}
	set logs  {}

	if {![info exists run::dirs($i)]} {
	    continue
	}
	
	set hostname $net::host($i)
	set dir      $run::dirs($i)

	get_files cores $hostname $dir "*core*"
	get_files logs  $hostname $dir "*.out"
	get_files logs  $hostname $dir "*.err"
	
	dbg "* $hostname:$i cores = $cores"
	dbg "* $hostname:$i logs = $logs"

	foreach l $logs {
	    set contents [run_cmd $hostname cat $l]
	    set contents [string trim $contents]
	    
	    if {[string length $contents] != 0} {
		set exec_name [manifest::rfile [file rootname [file tail $l]]]
		if {$exec_name == ""} {
		    puts "warning: no reverse manifest for [file rootname $l]"
		} else {
		    set expand [import_find expand-stacktrace.pl]
		    set contents [run_cmd $hostname cat $l | \
			    $expand -o $exec_name]
		}

		dbg "% got $hostname:$i [file tail $l]:"
		if {$opt(logdir) == ""} {
		    puts "***"
		    puts "*** $hostname:$i [file tail $l]:"
		    puts "***"

		    puts $contents
		} else {
		    set outf [open "$i-$hostname-[file tail $l].log" "w"]
		    puts $outf $contents
		    close $outf
		}
	    }
	}
	
	foreach c $cores {
	    if [net::is_localhost $hostname] {
		set cp "cp "
	    } else {
		set cp "scp $hostname:"
	    }
	    
	    if {$opt(logdir) == ""} {
		set clocal $i-$hostname-[file tail $c]
	    } else {
		set clocal $opt(logdir)/$i-$hostname-[file tail $c]
	    }

	    puts "error: found core file $c (copying to $clocal)"
	    eval exec $cp$c $clocal
	}
    }
}

#
# Cleanup the directories created by the test
#
proc cleanup {} {
    global opt net::host run::dirs dist::distdirs

    if {$opt(leave_crap)} { return }

    puts "* Getting rid of run files"
    for {set i 0} {$i < [net::num_nodes]} {incr i} {
	set hostname $net::host($i)

	if {$opt(local_rundir) && $hostname == "localhost"} {
	    continue
	}
	
	if [info exist run::dirs($i)] {
	    dbg "% removing $hostname:$i:$run::dirs($i)"
	    catch { run_cmd $hostname /bin/rm -r $run::dirs($i) }
	}
	if [info exist dist::distdirs($i)] {
	    dbg "% removing $hostname:$i:$dist::distdirs($i)"
	    catch { run_cmd $hostname /bin/rm -r $dist::distdirs($i) }
	}
    }

    if [info exist dist::distdirs(-1)] {
	dbg "% removing distdir"
	catch { run_cmd $hostname /bin/rm -r $dist::distdirs(-1) }
    }
}

# namespace run
}
