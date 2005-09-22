#!/usr/bin/tclsh

namespace eval run {
    
proc dbg { msg } {
    global opt
    if {$opt(verbose) > 0} {
	puts $msg
    }
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

proc generate_script {id exec_opts_fcn} {
    global opt net::host test::testname

    set hostname $net::host($id)
    set rundir   $opt(rundir_prefix)-$hostname-$id

    array set exec_opts [$exec_opts_fcn $id]

    # runscript
    set script(exec_file)   $exec_opts(exec_file)
    set script(exec_opts)   $exec_opts(exec_opts)
    set script(gdb_opts)    $opt(gdbopts)
    set script(run_dir)     $rundir
    set script(run_id)      $exec_opts(exec_file)-$hostname-$id
    set script(gdb)         $opt(gdb)
    set script(local)       [net::is_localhost $hostname]
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

    set run_base "run-$exec_opts(exec_file)"
    
    write_script $id $rundir $run_base.sh  $runscript true
    write_script $id $rundir $run_base.gdb $gdbscript true
    
    set confname $exec_opts(confname)
    set conf     $exec_opts(conf)

    if {$confname != ""} {
	write_script $id $rundir $confname $conf false
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
    puts "    --crap                     Kill baby seals"
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
    
    if [string equal test_script "invalid"] {
	usage
	exit
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

#
# Run a given program on the specified list of nodes
#
proc run {nodelist exec_opts_fcn} {
    global opt manifest::manifest manifest::subst net::nodes

    array set exec_opts [$exec_opts_fcn 0]
    set exec_file $exec_opts(exec_file)
    
    puts "* Generating scripts for $exec_file on nodes $nodelist"
    foreach i $nodelist {
	generate_script $i $exec_opts_fcn
    }

    puts "* Running program $exec_file on nodes $nodelist"

    foreach i $nodelist {
	set hostname $net::host($i)
	set script "$opt(rundir_prefix)-$hostname-$i/run-$exec_file.sh"
	
	switch "[net::is_localhost $hostname] $opt(xterm)" {
	    "1 1" { 
		dbg "xterm -e $script &"
		exec xterm -e $script & 
	    }
	    "0 1" {
		dbg "% ssh $hostname $script"
		set screen_id [exec ssh $hostname $script]
		dbg "% $hostname $exec_file instance is PID $remote_pid"
		exec xterm -e ssh -t $hostname "screen -r $screen_id" &
	    }
	    "1 0" {
		dbg "% $script &"
		exec $script &
	    }
	    "0 0" {
		dbg "% ssh $hostname $script"
		set remote_pid [exec ssh $hostname $script]
		dbg "% $hostname $exec_file instance is PID $remote_pid"
	    }
	}
    }
}

}
