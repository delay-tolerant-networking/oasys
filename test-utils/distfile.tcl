#
# Generate a distribution set
# 
# @param manifest_list List of manifest files
# @param basedir       Basedir from which the files are taken
# @param subst         A list of mappings to be done in the 
#                      manifests, e.g. { exe stripped }
# @param verbose       Print out what is happening
#
# Manifest file format:
#
# Manifest is a tcl file that returns a list of tuples, each tuple
# being of several types:
#     { F local-filename remote-filename}
#     { D remote-directory }
#
# Example: { { F "bin/tierd" "bin/tierd" } { D "tierstore" } }
#
proc create-dist {manifest_list basedir subst verbose} {
    set tmpdir "/tmp/distrun.tcl-[pid]"
    exec mkdir $tmpdir

    if {$verbose} { puts "% constructing file list" }
    foreach manifest_file $manifest_list {
	if [catch { set manifest [source $manifest_file] }] {
	    puts "! bad manifest $manifest_file, skipping"
	}

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
    }
    return $tmpdir
}   

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
    foreach host $host_list {
	if [is-localhost $host] {
	    if {$verbose} { puts "% $distdir -> $distdir-$host" }
	    exec cp -r $distdir $targetdir-$host
	} else {
	    if {$verbose} { puts "% $distdir -> $host:$targetdir-$host" }
	    exec scp -C -r $distdir $host:$targetdir-$host
	}
    }

    if {$verbose} { puts "% removing $distdir" }
    exec rm -rf $distdir
}
