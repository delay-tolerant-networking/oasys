
#
# File distribution utilities
#
namespace eval dist {

#
# A handler proc to call for cleaning up a run directory for use in
# case there's a mounted directory or something that can't just be
# cleaned with rm -rf. 
#
set cleanup_handler ""

#
# Return the proper run directory for the given hostname / test id
#
proc get_rundir {hostname id} {
    global opt
    
    if {$opt(local_rundir) && $hostname == "localhost"} {
	set cwd [pwd]
	# XXX/demmer cygwin hack
	regsub -- {C:} $cwd {/cygdrive/c} cwd
	return "$cwd/run-$id"
    } else {
	return $opt(rundir_prefix)-$id
    }
}


#
# Generate a distribution set. 
#
# XXX/bowei -- optimize for duplicate files by making the last #
# duplicate win. Right now there is an extra copy op, but the behavior
# is the same
# 
# @param manifest_list List of manifest files
# @param basedir       Basedir from which the files are taken
# @param subst         A list of mappings to be done in the 
#                      manifests, e.g. { exe stripped }
# @param strip         Strip ELF executables to save transmission time
# @param verbose       Print out what is happening
#
# Manifest file format list of tuples:
#
# { F local-filename remote-filename}
# { D remote-directory }
#

proc create {manifest basedir subst strip verbose} {
    global env opt
    # Somehow these /tmp/distrun.tcl dirs seem to leak if they're
    # uniquified by pid, so instead we just use the username

    if {$opt(dry_run)} {
	return
    }
    
    set tmpdir "/tmp/distrun.tcl-$env(USER)"

    if [file exists $tmpdir] {
	puts "WARNING: $tmpdir leaked from previous test!!"
	exec rm -rf $tmpdir
    }
    
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

		if { $strip && [file executable $tmpdir/$dst] } {
		    # This should be safe, strip bails if it's not an exe
		    if {$verbose} { 
			puts "% stripping $tmpdir/$dst"
		    }
		    catch { exec strip "$tmpdir/$dst" }
		}
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
# @param node_list     List of destination node ids
# @param basedir       Basedir from which the files are taken
# @param subst         A list of mappings to be done in the 
#                      manifests, e.g. { exe stripped }
# @param strip         Strip executables in the distribution
# @param verbose       Print out what is happening
#
    
proc files {manifest_list node_list basedir subst strip {verbose 0}} {
    global ::dist::distdirs ::dist::cleanup_handler opt
    global net::host

    set distdir [dist::create $manifest_list $basedir $subst $strip $verbose]
    set dist::distdirs(-1) $distdir

    if {$opt(dry_run)} {
	return
    }

    if {$verbose} {
	puts "% copying files" 
    }
	    
    foreach id $node_list {
	set hostname $net::host($id)
	set targetdir [get_rundir $hostname $id]
	set dist::distdirs($id) $targetdir

	if {$verbose} { puts "% $distdir -> $hostname:$targetdir" }

	if {$dist::cleanup_handler != ""} {
	    
	    if {$verbose} { puts "% calling cleanup_handler $cleanup_handler" }
	    $dist::cleanup_handler $hostname $targetdir
	} else {
	    if {$verbose} { puts "% no cleanup handler set" }

	}
	
	run::run_cmd $hostname rm -rf $targetdir
	
	if [net::is_localhost $hostname] {
	    exec cp -r $distdir $targetdir
	} else {
	    exec scp -C -r $distdir $hostname:$targetdir
	}
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
    
    proc rfile { dst } {
	global manifest::manifest

	set src ""
	foreach m $manifest::manifest {
	    if {[lindex $m 0] == "F" && [lindex $m 2] == $dst} {
		set src [lindex $m 1]
	    }
	}

	return $src
    }
}

