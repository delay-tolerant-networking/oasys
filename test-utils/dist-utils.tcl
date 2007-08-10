#
#    Copyright 2005-2006 Intel Corporation
# 
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
# 
#        http://www.apache.org/licenses/LICENSE-2.0
# 
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#


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
# @param subst         A list of mappings to be done in the 
#                      manifests, e.g. { exe stripped }
#
# Manifest file format list of tuples:
#
# { F local-filename remote-filename}
# { D remote-directory }
#

proc create {manifest subst} {
    global env opt
    # Somehow these /tmp/distrun.tcl dirs seem to leak if they're
    # uniquified by pid, so instead we just use the username

    if {$opt(dry_run)} {
	return
    }
    
    set tmpdir "/tmp/distrun.tcl-$env(USER)"

    if [file exists $tmpdir] {
	testlog "WARNING: $tmpdir leaked from previous test!!"
	exec rm -rf $tmpdir
    }
    
    exec mkdir $tmpdir

    dbg "% constructing file list"
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
                dbg "% $src -> $tmpdir/$dst"
		exec cp "$src" "$tmpdir/$dst"

		if { $opt(strip) && [file executable $tmpdir/$dst] } {
		    # This should be safe, strip bails if it's not an exe
                    dbg "% stripping $tmpdir/$dst"
		    catch { exec strip "$tmpdir/$dst" }
		}
	    }
	    
	    "D" {
		set new_dir [lindex $entry 1]
		foreach {k v} $subst { regsub $k $dst $v dst }
		dbg "% making $tmpdir/$new_dir"
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
# @param subst         A list of mappings to be done in the 
#                      manifests, e.g. { exe stripped }
#
    
proc files {manifest_list node_list subst} {
    global ::dist::distdirs ::dist::cleanup_handler opt
    global net::host

    set distdir [dist::create $manifest_list $subst]

    if {$opt(dry_run)} {
	return
    }

    dbg "% copying files" 
	    
    foreach id $node_list {
	set hostname $net::host($id)
	set targetdir [get_rundir $hostname $id]
	set dist::distdirs($id) $targetdir

	dbg "% $distdir -> $hostname:$targetdir"

	if {$dist::cleanup_handler != ""} {
	    
	    dbg "% calling cleanup_handler $cleanup_handler"
	    $dist::cleanup_handler $hostname $targetdir
	} else {
	    dbg "% no cleanup handler set"

	}
	
	run::run_cmd $hostname rm -rf $targetdir
	
	if [net::is_localhost $hostname] {
	    exec cp -r $distdir $targetdir
	} else {
	    exec scp -C -r $distdir $hostname:$targetdir
	}
    }

    dbg "% removing $distdir" 
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

