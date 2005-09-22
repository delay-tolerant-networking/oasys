#
# Generate a distribution set
# 
# @param manifest_list List of manifest files
# @param basedir       Basedir from which the files are taken
# @param subst         A list of mappings to be done in the 
#                      manifests, e.g. { exe stripped }
# @param verbose       Print out what is happening
#
# Manifest file format list of tuples:
#
# { F local-filename remote-filename}
# { D remote-directory }
#
proc create-dist {manifest basedir subst verbose} {
    set tmpdir "/tmp/distrun.tcl-[pid]"
    exec mkdir $tmpdir

    if {$verbose} { puts "% constructing file list" }
    foreach entry $manifest {
	set type [lindex $entry 0]
	switch $type {
	    "F" {
		set src [lindex $entry 1]
		foreach {k v} $subst { regsub $k $src $v src }
		set dst [lindex $entry 2]
		foreach {k v} $subst { regsub $k $dst $v dst }
		set dir [file dirname $dst]
		if [string length $dir] {
		    exec mkdir -p "$tmpdir/$dir"
		}
		if {$verbose} {
		    puts "% $basedir/$src -> $tmpdir/$dst"
		}
		exec cp "$basedir/$src" "$tmpdir/$dst"
	    }
	    
	    "D" {
		set new_dir [lindex $entry 1]
		foreach {k v} $subst { regsub $k $dst $v dst }
		if {$verbose} { puts "% making $tmpdir/$new_dir" }
		exec mkdir -p "$tmpdir/$new_dir"
	    }
	    
	    default { puts "! garbage in manifest $manifest_file" }
	}
    }

    return $tmpdir
}   

#
# Distribute files to localhost/remote hosts
#
# @param manifest_list List of manifest files
# @param host_list     List of destination hosts
# @param basedir       Basedir from which the files are taken
# @param targetdir     Target directory
# @param subst         A list of mappings to be done in the 
#                      manifests, e.g. { exe stripped }
# @param verbose       Print out what is happening
#
proc distfiles {manifest_list host_list basedir targetdir subst {verbose 0}} {
    set distdir [create-dist $manifest_list $basedir $subst $verbose]

    if {$verbose} { puts "% copying files" }
    
    set i 0
    foreach host $host_list {
	if [is-localhost $host] {
	    if {$verbose} { puts "% $distdir -> $host:$targetdir-$host-$i" }
	    exec cp -r $distdir $targetdir-$host-$i
	} else {
	    if {$verbose} { puts "% $distdir -> $host:$targetdir-$host-$i" }
	    exec scp -C -r $distdir $host:$targetdir-$host-$i
	}
	incr i
    }

    if {$verbose} { puts "% removing $distdir" }
    exec rm -rf $distdir
}

# 
# @return 1 if host is localhost, o.w. 0 
#
proc is-localhost { host } {
    if {[string equal $host "localhost"]    ||
	[regexp {172\.16\.\d+\.\d+}  $host] ||
	[regexp {192\.168\.\d+\.\d+} $host] ||
	[regexp {10\.\d+\.\d+\.\d+}  $host]} {
	return 1
    }

    return 0
}

#
# Manage global manifest list
#
namespace eval manifest {
    set manifest {}
    set subst    {}

    proc file { src dst } { 
	global manifest::manifest
	lappend manifest::manifest [list F "$src" "$dst"] 
    }
    proc dir { dir } { 
	global manifest::manifest
	lappend manifest::manifest [list D "$dir"] 
    }
    proc set { manifest } {
	global manifest::manifest
	eval lappend manifest::manifest $manifest
    }
}

#
# Manage network topology
#
namespace eval net {
    set nodes 0
    
    # Network definitions
    proc node { node_id hostname new_portbase {new_extra {} } } {
	global net::host net::portbase net::extra
	global net::nodes

	set  net::host($node_id)      $hostname
	set  net::portbase($node_id)  $new_portbase
	set  net::extra($node_id)     $new_extra
	incr net::nodes
    }
}

#
# Manage each node's tcl configuration file
#
namespace eval conf {
    proc add { node text } {
	global conf::conf
	append conf::conf($node) $text
    }
    proc get { node } {
	global conf::conf
	return $conf::conf($node)
    }
}

#
# Functions for running the test
# 
namespace eval test {
    set run_actions ""
    
    # Script actions to be performed after launching everything
    proc decl { actions } {
	global test::run_actions
	set test::run_actions $actions
    }
}

