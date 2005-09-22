
#
# File distribution utilities
#
namespace eval dist {

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

proc create {manifest basedir subst verbose} {
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
    
proc files {manifest_list host_list basedir targetdir subst {verbose 0}} {
    set distdir [dist::create $manifest_list $basedir $subst $verbose]

    if {$verbose} { puts "% copying files" }
    
    set i 0
    foreach host $host_list {
	if [net::is_localhost $host] {
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

# namespace eval dist
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

