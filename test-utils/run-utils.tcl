#!/usr/bin/tclsh
source "distfile.tcl"
source "resolve-path.tcl"

proc dbg { msg } {
    global opt
    if {$opt(verbose) > 0} {
	puts $msg
    }
}

proc usage {} {
    puts "Options:"
    puts "    -g                   Run program with gdb"
    puts "    -h | -help | --help  Print help message"
    puts "    -l <log dir>         Set a different directory for logs"
    puts "    -n <# nodes>         Number of nodes, unless overridden by test"
    puts "    -p                   Pause after apps dies, applies only to"
    puts "                         xterm option"
    puts "    -r <run dir>         /tmp/run dir will be the test directory"
    puts "    -v                   Verbose mode"
    puts "    -x                   Run each instance in an xterm"
    puts ""
    puts "Extended options:"
    puts "    --extra-gdbrc <script>     Add an extra gdbrc to be run"
    puts "    --gdb-opts    <options...> Extra options to gdb"
    puts "    --opts        <options...> Extra options to the program"
    puts "    --gdbrc       <tmpl>       Change remote gdbrc template"
    puts "    --script      <tmpl>       Change remote run script template"
}

proc hostlist { netdef } {
    set hosts {}
    foreach entry $netdef {
	array set net $entry
	lappend hosts $net(host)
    }

    return $hosts
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

proc generate-run-script {netdef netentry_id exec_opts_fcn} {
    global opt
    
    set netentry [lindex $netdef $netentry_id]

    array set net $netentry
    array set exec_opts [$exec_opts_fcn $netentry]

    set rundir "$opt(rundir_prefix)-$host-$netentry_id"

    set script(exec_file)   $exec_opts(exec_file)
    set script(exec_opts)   $exec_opts(exec_opts)
    set script(gdb_opts)    $opt(gdbopts)
    set script(run_dir)     $rundir
    set script(run_id)      "$exec_opts(exec_file)-$host-$netentry_id"
    set script(gdb)         $opt(gdb)
    set script(local)       [is-localhost $net(host)]
    set script(pause_after) $opt(pause)
    set script(xterm)       $opt(xterm)

    set runscript [process_template $opt(script_tmpl) script]    
    dbg "% runscript = \n$runscript"

    set gdb(exec_opts)      $exec_opts(exec_opts)
    set gdb(gdb_extra)      $opt(gdb_extra) # XXX/bowei - finish me
    set gdbscript [process_template $opt(gdb_tmpl) gdb]    
    dbg "% gdbscript = \n$gdbscript"
    
    if [is-localhost $net(host)] {
	exec cat > $rundir/run-script.sh  << "$runscript"
	dbg "% wrote $net(host):$netentry_id:$rundir/run-script.sh"
	exec cat > $rundir/run-script.gdb << "$gdbscript"
	dbg "% wrote $net(host):$netentry_id:$rundir/run-script.gdb"
	exec cat > $rundir/network.tcl << "$netentry"
	dbg "% wrote $net(host):$netentry_id:$rundir/network.tcl"
	exec chmod +x $rundir/run-script.sh
	dbg "% chmod +x $net(host):$netentry_id:$rundir/run-script.sh"
    } else {
	exec ssh $net(host) "cat > $rundir/run-script.sh" << "$runscript"
	dbg "% wrote $net(host):$netentry_id:$rundir/run-script.sh"
	exec ssh $net(host) "cat > $rundir/run-script.gdb" << "$gdbscript"
	dbg "% wrote $net(host):$netentry_id:$rundir/run-script.gdb"
	exec ssh $net(host) "cat > $rundir/network.tcl" << "$netentry"
	dbg "% wrote $net(host):$netentry_id:$rundir/network.tcl"
	exec ssh $net(host) "chmod +x $rundir/run-script.sh"
	dbg "% chmod +x $net(host):$netentry_id:$rundir/run-script.sh"
    }
}

proc run {args manifest_list manifest_subst $basedir netdef exec_opts_fcn} {
    global opt
    
    set opt(gdb)           0
    set opt(logdir)        "."
    set opt(nodes)         0
    set opt(pause)         0
    set opt(rundir_prefix) "/tmp/run-[pid]"
    set opt(verbose)       0
    set opt(xterm)         0
    
    set opt(gdb_extra)   ""
    set opt(gdbopts)     ""
    set opt(opts)        ""
    set opt(gdb_tmpl)    "gdbrc.template"
    set opt(script_tmpl) "script.template"
    
    # parse options
    while {[llength $args] > 0} {
	if {[llength $args] > 1} {
	    set arg1 [lindex $args 1]
	} else {
	    set arg1 ""
	}
	
	switch -- [lindex $args 0] {
	    -h     -
	    -help  -
	    --help { usage; exit }
	    -g     {set opt(gdb) 1}
	    -l     {shift; set opt(logdir) $arg1 }
	    -n     {shift; set opt(nodes)  $arg1 }
	    -p     {set opt(pause) 1}
	    -r     {shift; set opt(rundir_prefix) $arg1 }
	    -v     {set opt(verbose) 1}
	    -x     {set opt(xterm) 1}
	    --extra-gdbrc {shift; set opt(gdb_extra)  $arg1 }
	    --gdb-opts    {shift; set opt(gdbopts)    $arg1 }
	    --opts        {shift; set opt(opts)       $arg1 }
	    --gdb-tmpl    {shift; set opt(gdb_tmpl)    $arg1 }
	    --script-tmpl {shift; set opt(script_tmpl) $arg1 }
	    default       { puts "Illegal option $arg1"; usage }
	}
	shift
    }
    
    puts "* Distributing files"
    distfile $manifest_list [hostlist $netdef] $basedir $opt(rundir_prefix) \
	$manifest_subst $opt(verbose)
    for {set i 0} {$i < [llength $netdef]} {incr i} {
	generate-run-script $netdef $i $exec_opts_fcn
    }

    puts "* Running program"
    for {set i 0} {$i < [llength $netdef]} {incr i} {
	array set net [lindex $netdef $i]
	switch "[is-localhost $net(host)] $opt(xterm)" {
	    "1 1" { 
		dbg "xterm -e $opt(rundir_prefix)-$host-$i/run-script.sh &"
		exec xterm -e $opt(rundir_prefix)-$host-$i/run-script.sh & 
	    }
	    "0 1" {
		dbg "% ssh $host $opt(rundir_prefix)-$host-$i/run-script.sh"
		set screen_id \
		    [exec ssh $host $opt(rundir_prefix)-$host-$i/run-script.sh]
		dbg "% $host tierd instance is PID $remote_pid"
		exec xterm -e ssh -t $host "screen -r $screen_id" &
	    }
	    "1 0" {
		dbg "% $opt(rundir_prefix)-$host-$i/run-script.sh &"
		exec $opt(rundir_prefix)-$host-$i/run-script.sh &
	    }
	    "0 0" {
		dbg "% ssh $host $opt(rundir_prefix)-$host-$i/run-script.sh"
		set remote_pid \
		    [exec ssh $host $opt(rundir_prefix)-$host-$i/run-script.sh]
		dbg "% $host tierd instance is PID $remote_pid"
	    }
	}
    }
}