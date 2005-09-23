#!/usr/bin/tclsh

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
    puts "    -net <file>          Select the net file"
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
    puts "    --crap                     Pollute the /tmp dir"
}

proc init {args test_script} {
    global opt
    
    set opt(gdb)           0
    set opt(logdir)        "."
    set opt(pause)         0
    set opt(rundir_prefix) "/tmp/run-[pid]"
    set opt(verbose)       0
    set opt(xterm)         0
    set opt(crap)          0
    set opt(net)           ""

    set opt(gdb_extra)     ""
    set opt(gdbopts)       ""
    set opt(opts)          ""
    set opt(gdb_tmpl)      [import_find gdbrc.template]
    set opt(script_tmpl)   [import_find script.template]

    set num_nodes_override 0
    
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
	    --gdb         {set opt(gdb) 1}
	    -x            -
	    -xterm        -
	    --xterm       {set opt(xterm) 1}
	    -l            {shift args; set opt(logdir) [arg1 $args] }
	    -p            {set opt(pause) 1}
	    -r            {shift args; set opt(rundir_prefix) [arg1] }
	    -v            {set opt(verbose) 1}
	    -n            {shift args; set num_nodes_override [arg1 $args] }
	    -net          -
	    --net         {shift args; set opt(net)         [arg1 $args] }
	    --extra-gdbrc {shift args; set opt(gdb_extra)   [arg1 $args] }
	    --gdb-opts    {shift args; set opt(gdbopts)     [arg1 $args] }
	    --opts        {shift args; set opt(opts)        [arg1 $args] }
	    --gdb-tmpl    {shift args; set opt(gdb_tmpl)    [arg1 $args] }
	    --script-tmpl {shift args; set opt(script_tmpl) [arg1 $args] }
	    --crap        {set opt(crap) 1}
	    default       {puts "illegal option [arg1 $args]"; usage; exit }
	}
	shift args
    }

    puts "* Reading net definition file $opt(net)"
    import $opt(net)
    
    puts "* Reading test script $test_script"
    source $test_script

    if {$num_nodes_override != 0} {
	puts "* Setting num_nodes to $num_nodes_override"
	net::num_nodes $num_nodes_override
    }
    
    puts "* Distributing files"
    dist::files $manifest::manifest [net::hostlist] [pwd] \
	    $opt(rundir_prefix) $manifest::subst $opt(verbose)
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

    # run script
    set runscript [process_template $opt(script_tmpl) script]    
    dbg "% runscript = \n$runscript"

    # debug script
    set gdb(exec_opts)      $exec_opts
    set gdb(gdb_extra)      $opt(gdb_extra)
    set gdbscript [process_template $opt(gdb_tmpl) gdb]    
    dbg "% gdbscript = \n$gdbscript"

    set run_base "run-$exec_file"
    
    write_script $id $rundir $run_base.sh  $runscript true
    write_script $id $rundir $run_base.gdb $gdbscript true

    if {$confname != ""} {
	write_script $id $rundir $confname $conf false
    }
}

proc write_script {id dir filename contents do_chmod} {
    global net::host
    set hostname $net::host($id)

    set path [file join $dir $filename]
    
    if [net::is_localhost $hostname] {
	exec cat > $path << $contents
	if {$do_chmod} {
	    exec chmod +x $path
	}
	
    } else {
	exec ssh $hostname "cat > $path" << $contents
	if {$do_chmod} {
	    exec ssh $hostname "chmod +x $path"
	}
    }


    dbg "% wrote $hostname:$id:$path"
}

#
# Run a given program on the specified list of nodes
#
proc run {id exec_file exec_opts confname conf} {
    global opt manifest::manifest manifest::subst 
    global run::pids run::dirs run::xterm

    set hostname $net::host($id)

    puts "* Generating scripts for $exec_file for $hostname:$id"
    generate_script $id $exec_file $exec_opts $confname $conf

    set script "$opt(rundir_prefix)-$hostname-$id/run-$exec_file.sh"
    set run::dirs($id) "$opt(rundir_prefix)-$hostname-$id"

    # NB: When running in an xterm, the PID collected is the PID of
    # the local xterm instance, not the remote process instance
    switch "[net::is_localhost $hostname] $opt(xterm)" {
	"1 1" { 
	    dbg "xterm -title \"$hostname-$id\" -e $script &"
	    set exec_pid [exec xterm -title "$hostname - $id" -e $script &]
	    set run::pids($id) $exec_pid
	    set run::xterm($id) 1
	    dbg "% $hostname:$id $exec_file instance is PID $exec_pid"
	}
	"0 1" {
	    dbg "% xterm -title \"$hostname-$id\" -e ssh -t $hostname $script"
	    set exec_pid [exec xterm -title "$hostname - $id" \
			      -e ssh -t $hostname $script &]
	    set run::pids($id) $exec_pid
	    set run::xterm($id) 1
	    dbg "% $hostname:$id $exec_file instance is PID $exec_pid"
	}
	"1 0" {
	    dbg "% $script &"
	    set exec_pid [exec $script &]
	    set run::pids($id)  $exec_pid
	    set run::xterm($id) 0
	    dbg "% $hostname:$id $exec_file instance is PID $exec_pid"
	}
	"0 0" {
	    dbg "% ssh $hostname $script"
	    set exec_pid [exec ssh $hostname $script]
	    set run::pids($id) $exec_pid
	    set run::xterm($id) 0

	    dbg "% $hostname:$id $exec_file instance is PID $exec_pid"
	}
    }
}

#
# Wait for all running test programs to exit
#
proc wait_for_programs {} {
    global net::host run::pids run::xterm
     
    set num_alive 1

    puts "* Waiting for programs"
    while {$num_alive > 0} {
	set num_alive 0

	for {set i 0} {$i < [net::num_nodes]} {incr i} {
	    set hostname $net::host($i)
	    set pid      $run::pids($i)

	    # zero pid means the process died
	    if { (! [info exist $run::pids($i)]) || 
		 $run::pids($i) == 0} {
		continue
	    }
	    
	    if {[net::is_localhost $hostname] || $run::xterm($i)} {
		dbg "% ps h -p $pid"
		if [catch { exec ps h -p $pid }] {
		    puts "    $hostname:$i finished"
		    set run::pids($i) 0 
		} else {
		    dbg "% $hostname still alive"
		    incr num_alive
		}
	    } else {
		dbg "% ssh $hostname ps h -p $pid"
		if [catch { exec ssh $hostname ps h -p $pid }] {
		    puts "    $hostname:$i finished"
		    set run::pids($i) 0 
		} else {
		    dbg "% $hostname:$i still alive"
		    incr num_alive
		}
	    }
	}
	after 500
    }

    puts "* All programs done"
}

#
# Collect the logs/cores left by all of the executing processes
# 
proc collect_logs {} {
    global opt net::host run::dirs

    puts "* Collecting logs/cores into $opt(logdir)"
    if {! [file isdirectory $opt(logdir)]} {
	if [file exists $opt(logdir)] {
	    puts "$opt(logdir) exists, putting logs into $opt(logdir)-logs"
	    set opt(logdir) $opt(logdir)-logs
	}
	
	exec mkdir -p $opt(logdir)
    }

    for {set i 0} {$i < [net::num_nodes]} {incr i} {
	set hostname $net::host($i)
	set dir      $run::dirs($i)
	
	if [net::is_localhost $hostname] {
	    set logs ""
	    dbg "exec sh << \"ls -1 $dir/*core* $dir/*.out $dir/*.err\""
	    catch { append logs [exec sh << "ls -1 $dir/*core*"]}
	    append logs " "
	    catch { append logs [exec sh << "ls -1 $dir/*.out"]}
	    append logs " "
	    catch { append logs [exec sh << "ls -1 $dir/*.err"]}

	    dbg "% logs = $logs"
	    foreach l $logs {
		dbg "% exec cp $l $opt(logdir)/$i-$hostname-[file tail $l]"
		exec cp $l $opt(logdir)/$i-$hostname-[file tail $l]
	    }
	} else {
	    set logs ""
	    dbg "exec sh << \"ls -1 $dir/*core* $dir/*.out $dir/*.err\""
	    catch { append logs [exec ssh $hostname sh << "ls -1 $dir/*core*"]}
	    append logs " "
	    catch { append logs [exec ssh $hostname sh << "ls -1 $dir/*.out"]}
	    append logs " "
	    catch { append logs [exec ssh $hostname sh << "ls -1 $dir/*.err"]}

	    dbg "% logs = $logs"
	    foreach l $logs {
		dbg "% exec scp $hostname:$l $opt(logdir)/$i-$hostname-[file tail $l]"
		exec scp $hostname:$l $opt(logdir)/$i-$hostname-[file tail $l]
	    }
	}
    }
}

#
# Cleanup the directories created by the test
#
proc cleanup {} {
    global opt net::host run::dirs

    if {$opt(crap)} { return }

    puts "* Getting rid of run files"
    for {set i 0} {$i < [net::num_nodes]} {incr i} {
	set hostname $net::host($i)
	set dir      $run::dirs($i)

	dbg "% removing $hostname:$i:$dir"
	if [net::is_localhost $hostname] {
	    exec rm -r $dirs($i)
	    dbg "exec rm -r $dirs($i)"
	} else {
	    exec ssh $hostname rm -r $dirs($i)
	    dbg "exec ssh $hostname rm -r $dirs($i)"
	}
    }
}

# namespace run
}
