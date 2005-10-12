#!/usr/bin/tclsh
package require Tclx
if {[info procs log] != ""} {
    rename log tclx_log
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

proc arg1 { l } {
    return [lindex $l 0]
}

proc usage {} {
    puts "Options:"
    puts "    -h | -help | --help  Print help message"
    puts "    -g | -gdb  | --gdb   Run program with gdb"
    puts "    -x | -xterm          Run each instance in an xterm"
    puts "    -d | -daemon         Daemon mode (no command loop)"
    puts "    -geom | -geometry    Specify the xterm geometry (implies -x)"
    puts "    -net  | --net <file> Select the net file"
    puts "    -id   | --id  <id>   Id which is used to select a port slice"
    puts "    -l <log dir>         Set a different directory for logs"
    puts "    -n <# nodes>         Number of nodes, unless overridden by test"
    puts "    -p                   Pause after apps dies, applies only to"
    puts "                         xterm option"
    puts "    -r <rundir>          /tmp/<rundir> will be the test directory"
    puts "    -v                   Verbose mode"
    puts ""
    puts "Extended options:"
    puts "    --extra-gdbrc <script>     Add an extra gdbrc to be run"
    puts "    --gdb-opts    <options...> Extra options to gdb"
    puts "    --opts        <options...> Extra options to the program"
    puts "    --gdbrc       <tmpl>       Change remote gdbrc template"
    puts "    --script      <tmpl>       Change remote run script template"
    puts "    --no-logs                  Don't collect logs/cores"
    puts "    --leave-crap               Leave all crap /tmp dir"
    puts "    --strip                    Strip execs before copying"
}

proc init {args test_script} {
    global opt
    
    set opt(conf_id)       0
    set opt(daemon)        0
    set opt(gdb)           0
    set opt(leave_crap)    0
    set opt(logdir)        "."
    set opt(net)           ""
    set opt(no_logs)       0
    set opt(pause)         0
    set opt(rundir_prefix) "/tmp/run-[pid]"
    set opt(strip)         0
    set opt(verbose)       0
    set opt(xterm)         0

    set opt(gdb_extra)     ""
    set opt(gdbopts)       ""
    set opt(opts)          ""
    set opt(geometry)      ""
    set opt(gdb_tmpl)      [import_find gdbrc.template]
    set opt(script_tmpl)   [import_find script.template]

    set num_nodes_override 0
    
    # Load default options if they exist -- You can override options
    # if you want to with command line arguments.
    if [file readable "~/.debug_opts"] {
	set f [open "~/.debug_opts" "r"]
	
	set new_args [eval list [read -nonewline $f]]
	set args [concat $new_args $args]
	close $f
    }

    switch -- $test_script {
	invalid -
	-h      -
	-help   -
	--help  { usage; exit }
    }
    
    # parse options
    while {[llength $args] > 0} {
	switch -- [lindex $args 0] {
	    -h            -
	    -help         -
	    --help        { usage; exit }
	    -g            -
	    -gdb          -
	    --gdb         { set opt(gdb) 1 }
	    -x            -
	    -xterm        -
	    --xterm       { set opt(xterm) 1 }
	    -d            -
	    -daemon       -
	    --daemon      { set opt(daemon) 1 }
	    -geom         -
	    -geometry     -
	    --geometry    { shift args; set opt(xterm) 1; \
		            set opt(geometry) [arg1 $args] }
	    -l            { shift args; set opt(logdir) [arg1 $args] }
	    -p            { set opt(pause) 1}
	    -r            { shift args; set opt(rundir_prefix) [arg1] }
	    -v            { set opt(verbose) 1}
	    -n            { shift args; set num_nodes_override [arg1 $args] }
	    -net          -
	    --net         { shift args; set opt(net)         [arg1 $args] }
	    -id           -
	    --id          { shift args; set opt(conf_id)     [arg1 $args] }
	    --extra-gdbrc { shift args; set opt(gdb_extra)   [arg1 $args] }
	    --gdb-opts    { shift args; set opt(gdbopts)     [arg1 $args] }
	    --opts        { shift args; set opt(opts)        [arg1 $args] }
	    --gdb-tmpl    { shift args; set opt(gdb_tmpl)    [arg1 $args] }
	    --script-tmpl { shift args; set opt(script_tmpl) [arg1 $args] }
	    --no-logs     { set opt(no_logs) 1 }
	    --leave-crap  { set opt(leave_crap) 1}
	    --strip       { set opt(strip) 1 }
	    default       { puts "illegal option [arg1 $args]"; usage; exit }
	}
	shift args
    }

    puts "* Reading net definition file $opt(net)"
    import $opt(net)
    
    if {$num_nodes_override != 0} {
	puts "* Setting num_nodes to $num_nodes_override"
	net::num_nodes $num_nodes_override
    }
    
    puts "* Reading test script $test_script"
    source $test_script

    puts "* Distributing files"
    dist::files $manifest::manifest [net::hostlist] [pwd] \
	$opt(rundir_prefix) $manifest::subst $opt(strip) $opt(verbose)
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

proc generate_script {id exec_file exec_opts confname conf} {
    global opt net::host test::testname

    set hostname $net::host($id)
    set rundir   $opt(rundir_prefix)-$hostname-$id

    # runscript
    set script(exec_file)   $exec_file
    set script(exec_opts)   $exec_opts
    set script(gdb_opts)    $opt(gdbopts)
    set script(run_dir)     $rundir
    set script(run_id)      $exec_file-$hostname-$id
    set script(gdb)         $opt(gdb)
    set script(local)       [net::is_localhost $hostname]
    set script(pause_after) $opt(pause)
    set script(verbose)     $opt(verbose)
    set script(xterm)       $opt(xterm)
    set script(geometry)    $opt(geometry)

    # run script
    set runscript [process_template $opt(script_tmpl) script]    
    dbg "% runscript = \n$runscript"

    # debug script
    set gdb(exec_opts)      $exec_opts
    set gdb(gdb_extra)      $opt(gdb_extra)
    set gdb(gdb_test_extra) [conf::get gdb $id]

    set gdbscript [process_template $opt(gdb_tmpl) gdb]    
    dbg "% gdbscript = \n$gdbscript"

    set run_base "run-$exec_file"
    
    write_script $id $rundir $run_base.sh  $runscript true
    write_script $id $rundir $run_base.gdb $gdbscript true

    if {$confname != ""} {
	write_script $id $rundir $confname $conf false
    }
}

proc run_cmd {hostname args} {
    if [net::is_localhost $hostname] {
	dbg "% [join $args]"
	return [eval exec $args]
    } else {
	dbg "% ssh $hostname [join $args]"
	return [eval exec ssh $hostname $args]
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
proc run {id exec_file exec_opts confname conf} {
    global opt manifest::manifest manifest::subst 
    global run::pids run::dirs run::xterm

    set hostname $net::host($id)

    dbg "* Generating scripts for $exec_file for $hostname:$id"
    generate_script $id $exec_file $exec_opts $confname $conf

    set script "$opt(rundir_prefix)-$hostname-$id/run-$exec_file.sh"
    set run::dirs($id) "$opt(rundir_prefix)-$hostname-$id"

    set geometry $opt(geometry)

    puts "* Running $exec_file on $hostname:$id"
    
    # NB: When running in an xterm, the PID collected is the PID of
    # the local xterm instance, not the remote process instance
    switch "[net::is_localhost $hostname] $opt(xterm)" {
	"1 1" {
	    set exec_pid [run_cmd localhost xterm -title "$hostname-$id" \
		    -geometry $geometry -e $script &]
	    lappend run::pids($id) $exec_pid
	    set run::xterm($id) 1
	    dbg "% $hostname:$id $exec_file instance is PID $exec_pid"
	}
	"0 1" {
	    set exec_pid [run_cmd localhost  xterm -title "$hostname - $id" \
		    -geometry $geometry -e ssh -t $hostname $script &]
	    lappend run::pids($id) $exec_pid
	    set run::xterm($id) 1
	    dbg "% $hostname:$id $exec_file instance is PID $exec_pid"
	}
	"1 0" {
	    set exec_pid [run_cmd localhost sh $script &]
	    lappend run::pids($id)  $exec_pid
	    set run::xterm($id) 0
	    dbg "% $hostname:$id $exec_file instance is PID $exec_pid"
	}
	"0 0" {
	    set exec_pid [run_cmd $hostname sh $script &]
	    lappend run::pids($id) $exec_pid
	    set run::xterm($id) 0
	    dbg "% $hostname:$id $exec_file instance is PID $exec_pid"
	}
    }
}

#
# Wait for all running test programs to exit
#
proc wait_for_programs {{timeout 10000}} {
    global net::host run::pids run::xterm
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
		if {[llength run::pids($i)] == 0} {
		    continue
		}

		# all xterm pid actions are local
		if {$run::xterm($i)} {
		    set kill_hostname localhost
		} else {
		    set kill_hostname $hostname
		}

		foreach pid $run::pids($i) {
		    if {![catch {run_cmd $kill_hostname ps h -p $pid}]} {
			dbg "% $hostname:$i pid $pid still alive"
			if {$signal != "none"} {
			    puts "* ERROR: pid $pid on host $hostname:$i still alive"
			    puts "* ERROR: sending $signal signal"
			    catch {run_cmd $kill_hostname kill -s $signal $pid} err
			}
			lappend livepids $pid
		    }
		}

		if {[llength $livepids] == 0} {
		    puts "    $hostname:$i all processes finished"
		} else {
		    dbg "* $hostname:$i [llength livepids] pids still alive ($livepids)"
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

#
# Collect the logs/cores left by all of the executing processes
# 
proc collect_logs {} {
    global opt net::host run::dirs

    if {$opt(no_logs)} { return }

    puts "* Collecting logs/cores into $opt(logdir)"
    if {! [file isdirectory $opt(logdir)]} {
	if [file exists $opt(logdir)] {
	    puts "$opt(logdir) exists, putting logs into $opt(logdir)-logs"
	    set opt(logdir) $opt(logdir)-logs
	}
	
	exec mkdir -p $opt(logdir)
    }

    for {set i 0} {$i < [net::num_nodes]} {incr i} {
	set cores {}
	set logs  {}
	
	set hostname $net::host($i)
	set dir      $run::dirs($i)
	
	catch { lappend cores [split [run_cmd $hostname sh << "ls -1 $dir/*core*"]] }
	catch { lappend logs  [split [run_cmd $hostname sh << "ls -1 $dir/*.out"]]}
	catch { lappend logs  [split [run_cmd $hostname sh << "ls -1 $dir/*.err"]]}
	
	dbg "* $hostname:$i cores = $cores"
	dbg "* $hostname:$i logs = $logs"

	foreach c $cores {
	    if [net::is_localhost $hostname] {
		set cp "cp "
	    } else {
		set cp "scp $hostname:"
	    }

	    set clocal $opt(logdir)/$i-$hostname-[file tail $c]
	    puts "error: found core file $c (copying to $clocal)"
	    eval exec $cp$c $clocal
	}

	foreach l $logs {
	    set contents [run_cmd $hostname cat $l]
	    set contents [string trim $contents]
	    if {[string length $contents] != 0} {
		puts "***"
		puts "*** $hostname:$i [file tail $l]:"
		puts "***"
		puts $contents
	    }
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
