#!/usr/bin/tclsh
source "build/scripts/distfile.tcl"

proc dbg { msg } {
    global opt
    if {$opt(verbose) > 0} {
	puts $msg
    }
}

proc usage {} {
    puts "run.tcl \[options...\]"
    puts "    -g                   Run program with gdb"
    puts "    -h | -help | --help  Print help message"
    puts "    -l <log dir>         Set a different directory for logs"
    puts "    -p                   Pause after apps dies, applies only to"
    puts "                         xterm option"
    puts "    -r <run dir>         /tmp/run dir will be the remote test"
    puts "                         directory"
    puts "    -t <test script dir> Run a test instead of stock distribution"
    puts "    -v                   Verbose mode"
    puts "    -x                   Run tierd in an xterm"
    puts ""
    puts "Extended options:"
    puts "    --extra-gdbrc   <script>     Add an extra gdbrc to be run"
    puts "    --gdb-opts      <options...> Extra options to gdb"
    puts "    --net-script    <script>     Change networking script template"
    puts "    --opts          <options...> Extra options to tierd"
    puts "    --port-base     <port>       Base port for tierd"
    puts "    --remote-gdbrc  <script>     Change remote gdbrc template"
    puts "    --remote-script <script>     Change remote run script template"
    puts "    --strip                      Strip .exe files before transfer"
    exit
}

proc shift {} {
    global argv
    set argv [lrange $argv 1 end]
}

proc arg1 {} {
    global argv
    if {[llength $argv] == 0} {
	return ""
    } else {
	return [lindex $argv 0]
    }
}

proc get_num_hosts {} {
    global opt
    if {[string length $opt(test)] == 0} {
	dbg "% Using a single host"
	
	return 1
    } else {
	if {! [file exists $opt(test)/Hosts]} {
	    puts "!! Need $opt(test)/Hosts file to specify number of hosts"
	    exit 1
	}

	set f [open $opt(test)/Hosts "r"]
	set hosts [gets $f]
	close $f

	dbg "% Using $hosts hosts"

	return $hosts
    }
}

proc get_hosts { hosts hostfile } {
    if {! [file exists "~/$hostfile"] } {
	puts "! No ~/$hostfile found, stopping..."
	puts "! You need ~/$hostfile to run the tests"
	exit
    } else {
	set f [open "~/$hostfile" "r"]
	set hostlist {}
	for {set i 0} {$i < $hosts} {incr i} {
	    set host [gets $f]
	    if [string equal $host ""] {
		error "Not enough remote hosts to run test."
	    }
	    lappend hostlist $host
	}
    }
    
    dbg "% hostlist is \"$hostlist\""

    return $hostlist
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

proc generate-run-script {host host_id} {
    global opt globvar

    # run-script.sh

    if {$opt(local)} {
	set script(rundir) $opt(rundir)-$host
    } else {
	set script(rundir) $opt(rundir)
    }
    set script(pause_after) $opt(pause)
    set script(local)       $opt(local)
    set script(gdb)         $opt(gdb)
    set script(gdbopts)     $opt(gdbopts)
    set script(xterm)       $opt(xterm)
    set script(tierid)      $host_id
    set script(tier_opts)   $opt(extraopts)
    set script(test_id)     "tier-run-[pid]"

    set runscript [process_template $opt(script_tmpl) script]    
    dbg "% runscript = \n$runscript"

    # run-script.gdb
    if {! $opt(xterm)} {
	set gdb(daemon) "-d"
    } else {
	set gdb(daemon) ""
    }
    set gdb(id)   $host_id
    set gdb(opts) $opt(extraopts)

    set gdbscript [process_template $opt(gdbrc_tmpl) gdb]
    dbg "% gdbscript = \n$gdbscript"

    # tcl/network.tcl

    if { $opt(local) } {
	set net(local_iface) $host
    } else {
	set net(local_iface) "0.0.0.0"
    }
    set net(port_base)   $opt(port_base)
    set network_setup [process_template $opt(net_tmpl) net]
    dbg "% network_setup = \n$network_setup"

    if {$opt(local)} {
	exec cat > $opt(rundir)-$host/run-script.sh  << "$runscript"
	dbg "% wrote run-script for $host in $opt(rundir)-$host"
	exec cat > $opt(rundir)-$host/run-script.gdb << "$gdbscript"
	dbg "% wrote gdbscript for $host in $opt(rundir)-$host"
	exec cat > $opt(rundir)-$host/tcl/network.tcl << "$network_setup"
	dbg "% wrote network.tcl for $host in $opt(rundir)-$host"
	exec chmod +x $opt(rundir)-$host/run-script.sh
	dbg "% made run script for $host exec"
	exec cat > $opt(rundir)-$host/Hosts << "$globvar(hostlist)\n"
	dbg "% put hostlist to $opt(rundir)-$host/Hosts"
    } else {
	# put the scripts on the remote machines
	exec ssh $host "cat > $opt(rundir)/run-script.sh"  << "$runscript"
	dbg "% transferred run-script to $host"
	exec ssh $host "cat > $opt(rundir)/run-script.gdb" << "$gdbscript"
	dbg "% transferred gdbscript to $host"
	exec ssh $host "cat > $opt(rundir)/tcl/network.tcl" << "$network_setup"
	dbg "% transferred network.tcl to $host"
	exec ssh $host "chmod +x $opt(rundir)/run-script.sh"
	dbg "% made run script exec on $host"
	
	exec ssh $host "cat > $opt(rundir)/Hosts" << $globvar(hostlist)
	dbg "% put hostlist to $opt(rundir)/Hosts"
    }
}

proc sweep-logs-core {host rundir} {
    global opt
    if {! [file isdirectory $opt(logdir)] } {
	dbg "% making logdir $opt(logdir)"
	exec mkdir $opt(logdir)
    }

    dbg "% sweeping $host for logs and core"
    catch {
	exec scp $host:$rundir/tierd.log $opt(logdir)/tierd-$host.log
	exec scp $host:$rundir/tierd.err $opt(logdir)/tierd-$host.err
	
	set corefile [exec ssh $host "find $opt(rundir) -name \"*core*\""]
	if {[string length $corefile] > 0} {
	    dbg "$host:$corefile"
	    exec scp $host:$corefile $opt(logdir)/tierd-$host.core
	}
    }
}

# Parse arguments to script
set opt(extraopts)  ""
set opt(gdb)        0
set opt(gdbopts)    ""
set opt(local)      0
set opt(logdir)     "."
set opt(pause)      0
set opt(port_base)  12000
set opt(rundir)     "/tmp/tier-run-[pid]"
set opt(strip)      0
set opt(test)       ""
set opt(verbose)    0
set opt(xterm)      0

set opt(extra_gdbrc)  ""
set opt(gdbrc_tmpl)   "build/scripts/gdbrc.template"
set opt(net_tmpl)     "build/scripts/network.tcl.template"
set opt(script_tmpl)  "build/scripts/script.template"

while {[llength $argv] > 0} {
    switch -- [arg1] {
	-g      { set opt(gdb) 1 }
	-h      -
	-help   -
	--help  { usage }
	-l      { shift; set opt(logdir)    [arg1] }
	-p      { set opt(pause) 1 }
	-r      { shift; set opt(rundir) [arg1] }
	-t      { shift; set opt(test)   [arg1] }
	-v      { set opt(verbose) 1 }
	-x      { set opt(xterm) 1 }
	--opts          { shift; set opt(extraopts)   [arg1] }
	--gdb-opts      { shift; set opt(gdbopts)     [arg1] }
	--net-script    { shift; set opt(net_tmpl)    [arg1] }
	--remote-gdbrc  { shift; set opt(gdbrc_tmpl)  [arg1] }
	--remote-script { shift; set opt(script_tmpl) [arg1] }
	--extra-gdbrc   { shift; set opt(extra_gdbrc) [arg1] }
	--port-base     { shift; set opt(port_base)   [arg1] }
	--strip         { set opt(strip) 1 }
	--local         { set opt(local) 1 }
	default { puts "Illegal option [arg1]"; usage }
    }
    shift
}

# Clean up options that don't make sense
if {! $opt(xterm) && $opt(pause)} {
    set opt(pause) ""
    dbg "% correcting pause without xterm"
}

# Set up global variables
set globvar(manifest_list) {}
set globvar(remote_pids)   {}
lappend globvar(manifest_list) "tcl/Manifest"
if {[string length $opt(test)] > 0} {
    lappend globvar(manifest_list) "$opt(test)/Manifest"
} else {
    lappend globvar(manifest_list) "build/bin/Manifest"
}

if {$opt(local)} {
    set globvar(hostlist) [get_hosts [get_num_hosts] ".debug_iface" ]
} else {
    set globvar(hostlist) [get_hosts [get_num_hosts] ".debug_machines" ]
}

# Set up files set
puts "* Distributing files"
if {$opt(strip)} {
    dbg "% using stripped exe"
    set subst { %exe-suffix% stripped }
} else {
    dbg "% using normal (non-stripped) exe"
    set subst { %exe-suffix% exe }
}

if {$opt(local)} {
    distfile-local $globvar(manifest_list) $globvar(hostlist) \
	$opt(rundir) $subst $opt(verbose)
} else {
    distfile $globvar(manifest_list) $globvar(hostlist) \
	$opt(rundir) $subst $opt(verbose)
}

for {set i 0} {$i<[llength $globvar(hostlist)]} {incr i} {
    generate-run-script [lindex $globvar(hostlist) $i] $i
}


# Run tierd
puts "* Running tierd"
if {$opt(local)} {
    foreach host $globvar(hostlist) {
	dbg "% running $opt(rundir)-$host/run-script.sh"
	
	if {$opt(xterm)} {
	    puts "* Starting terminals"
	    dbg "% xterm -e $opt(rundir)-$host/run-script.sh &"
	    exec xterm -e $opt(rundir)-$host/run-script.sh &
	} else {
	    dbg "% $opt(rundir)-$host/run-script.sh &"
	    exec $opt(rundir)-$host/run-script.sh &
	}
    }
} else {
    foreach host $globvar(hostlist) {
	dbg "% running $opt(rundir)/run-script.sh on $host"
	set remote_pid [exec ssh $host $opt(rundir)/run-script.sh]
	dbg "% $host tierd instance is PID $remote_pid"
	lappend globvar(remote_pids) $remote_pid
    }

    if {$opt(xterm)} {
	puts "* Starting terminals"
	foreach host $globvar(hostlist) screen_pid $globvar(remote_pids) {
	    exec xterm -e ssh -t $host "screen -r $screen_pid" &
	}
    }
}

# Execute post action script
puts "* Executing test-hook.sh"
set i 0
foreach host $globvar(hostlist) {
    dbg "% running test hook on $host"
    if {$opt(local)} {
	catch { exec $opt(rundir)-$host/bin/test-hook.sh $i & }
    } else {
	catch { exec ssh $host $opt(rundir)/bin/test-hook.sh $i & }
    }
    incr i
}

if {! $opt(xterm) && ! $opt(local)} {
    # watch and sweep the log/core/err files back to this host
    puts "* Waiting for processes"
    set deadhosts 0
    set swept {}
    while {$deadhosts < [llength $globvar(hostlist)]} {
	foreach host $globvar(hostlist) remote_pid $globvar(remote_pids) {
	    set ps [exec ssh $host "ps aux"]
	    if {([string first $remote_pid $ps] == -1) &&
		([lsearch $swept $host] == -1)} {
		puts "* $host died"
		incr deadhosts
		sweep-logs-core $host $opt(rundir)
		lappend swept $host
	    }
	}
    }   
    puts "* Done, logs are in $opt(logdir)"
}
