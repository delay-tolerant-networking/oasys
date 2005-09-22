#!/usr/bin/tclsh
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
    puts "    --crap                     Kill baby seals"
}

proc hostlist {} {
    global net::host net::nodes
    set hosts {}

    for {set i 0} {$i<$net::nodes} {incr i} {
	lappend hosts $net::host($i)
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

proc generate-run-script {id exec_opts_fcn} {
    global opt net::host conf::conf test::name

    set hostname $net::host($id)
    set rundir   "$opt(rundir_prefix)-$hostname-$id"
    set confname $test::name

    array set exec_opts [$exec_opts_fcn $id]

    # runscript
    set script(exec_file)   $exec_opts(exec_file)
    set script(exec_opts)   $exec_opts(exec_opts)
    set script(gdb_opts)    $opt(gdbopts)
    set script(run_dir)     $rundir
    set script(run_id)      "$exec_opts(exec_file)-$hostname-$id"
    set script(gdb)         $opt(gdb)
    set script(local)       [is-localhost $hostname]
    set script(pause_after) $opt(pause)
    set script(xterm)       $opt(xterm)

    # run script
    set runscript [process_template $opt(script_tmpl) script]    
    dbg "% runscript = \n$runscript"

    # debug script
    set gdb(exec_opts)      $exec_opts(exec_opts)
    set gdb(gdb_extra)      $opt(gdb_extra)
    set gdbscript [process_template $opt(gdb_tmpl) gdb]    
    dbg "% gdbscript = \n$gdbscript"
    
    # tcl script
    if [info exist conf::conf($id)] {
	set tclscript [conf::get $id]
    } else {
	set tclscript ""
    }
    dbg "% tclscript = \n$tclscript"
    
    if [is-localhost $hostname] {
	exec cat > $rundir/run-test.sh  << "$runscript"
	dbg "% wrote $hostname:$id:$rundir/run-test.sh"

	exec cat > $rundir/run-test.gdb << "$gdbscript"
	dbg "% wrote $hostname:$id:$rundir/run-test.gdb"

	exec cat > $rundir/$confname.tcl << "$tclscript"
	dbg "% wrote $hostname:$id:$rundir/$confname.conf"

	exec chmod +x $rundir/run-test.sh
	dbg "% chmod +x $hostname:$id:$rundir/run-test.sh"
    } else {
	exec ssh $hostname "cat > $rundir/run-test.sh" << "$runscript"
	dbg "% wrote $hostname:$id:$rundir/run-test.sh"

	exec ssh $hostname "cat > $rundir/run-test.gdb" << "$gdbscript"
	dbg "% wrote $hostname:$id:$rundir/run-test.gdb"

	exec ssh $hostname "cat > $rundir/$confname.conf" << "$tclscript"
	dbg "% wrote $hostname:$id:$rundir/$confname.conf"

	exec ssh $hostname "chmod +x $rundir/run-test.sh"
	dbg "% chmod +x $hostname:$id:$rundir/run-test.sh"
    }
}

proc shift { l } {
    upvar $l xx
    set xx [lrange $xx 1 end]
}

proc arg1 { l } {
    return [lindex $l 0]
}

proc run {args tcl_script netdef_script basedir tmpl_dir exec_opts_fcn} {
    global opt manifest::manifest manifest::subst net::nodes

    set opt(gdb)           0
    set opt(logdir)        "."
    set opt(pause)         0
    set opt(rundir_prefix) "/tmp/run-[pid]"
    set opt(verbose)       0
    set opt(xterm)         0
    set opt(crap)          0

    set opt(gdb_extra)   ""
    set opt(gdbopts)     ""
    set opt(opts)        ""
    set opt(gdb_tmpl)    "$tmpl_dir/gdbrc.template"
    set opt(script_tmpl) "$tmpl_dir/script.template"
    
    # parse options
    while {[llength $args] > 0} {
	switch -- [lindex $args 0] {
	    -h     -
	    -help  -
	    --help { usage; exit }
	    -g     {set opt(gdb) 1}
	    -l     {shift args; set opt(logdir) [arg1 $args] }
	    -p     {set opt(pause) 1}
	    -r     {shift args; set opt(rundir_prefix) [arg1] }
	    -v     {set opt(verbose) 1}
	    -x     {set opt(xterm) 1}
	    --extra-gdbrc {shift args; set opt(gdb_extra)   [arg1 $args] }
	    --gdb-opts    {shift args; set opt(gdbopts)     [arg1 $args] }
	    --opts        {shift args; set opt(opts)        [arg1 $args] }
	    --gdb-tmpl    {shift args; set opt(gdb_tmpl)    [arg1 $args] }
	    --script-tmpl {shift args; set opt(script_tmpl) [arg1 $args] }
	    --crap        {set opt(crap) 1}
	    default       {puts "Illegal option [arg1 $args]"; usage }
	}
	shift args
    }
    
    puts "* Reading configuration scripts"
    source $netdef_script
    source $tcl_script

    puts "* Distributing files"
    distfiles $manifest::manifest [hostlist] $basedir $opt(rundir_prefix) \
	$manifest::subst $opt(verbose)
    
    puts "* Generating scripts"
    for {set i 0} {$i < $net::nodes} {incr i} {
	generate-run-script $i $exec_opts_fcn
    }

    puts "* Running program"
    for {set i 0} {$i < $net::nodes} {incr i} {
	set hostname $net::host($i)
	
	switch "[is-localhost $hostname] $opt(xterm)" {
	    "1 1" { 
		dbg "xterm -e $opt(rundir_prefix)-$hostname-$i/run-script.sh &"
		exec xterm -e $opt(rundir_prefix)-$hostname-$i/run-script.sh & 
	    }
	    "0 1" {
		dbg "% ssh $hostname $opt(rundir_prefix)-$hostname-$i/run-script.sh"
		set screen_id \
		    [exec ssh $hostname $opt(rundir_prefix)-$hostname-$i/run-script.sh]
		dbg "% $hostname tierd instance is PID $remote_pid"
		exec xterm -e ssh -t $hostname "screen -r $screen_id" &
	    }
	    "1 0" {
		dbg "% $opt(rundir_prefix)-$hostname-$i/run-script.sh &"
		exec $opt(rundir_prefix)-$hostname-$i/run-script.sh &
	    }
	    "0 0" {
		dbg "% ssh $hostname $opt(rundir_prefix)-$hostname-$i/run-script.sh"
		set remote_pid \
		    [exec ssh $hostname $opt(rundir_prefix)-$hostname-$i/run-script.sh]
		dbg "% $hostname tierd instance is PID $remote_pid"
	    }
	}
    }
}