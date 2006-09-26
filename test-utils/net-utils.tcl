#
# Manage network topology
#
namespace eval net {

#
# Number of nodes used in the test
#
set total_nodes 0

#
# Define a new node in the test
#
proc node { node_id hostname new_portbase } {
    global net::host net::portbase net::used_ports opt
    
    set net::host($node_id)       $hostname
    set net::portbase($node_id)   [expr $new_portbase + $opt(conf_id) * 100]
    set net::used_ports($node_id) {}
}

#
# Set the number of nodes for the test.
#
proc num_nodes { {num -1} } {
    global net::total_nodes net::host
    
    if {$num != -1} {
	set defined_nodes [llength [array names net::host]]
	
	if {$num > $defined_nodes} {
	    error "Cannot set number of nodes ($num) greater than the number \
		    defined ($defined_nodes)"
	}
	set net::total_nodes $num
    }
    
    return $net::total_nodes
}

#
# Set the number of nodes for the test, ignoring the setting if the
# user has already overridden it on the command line with -n <num>.
#
proc default_num_nodes {num} {
    global net::total_nodes 
    if {$net::total_nodes == 0} {
	num_nodes $num
    }
}

#
# Script to override the node ids used in the test so they aren't
# necessarily sequential and packed.
#
proc override_nodelist {ids {remap 1}} {
    global net::host net::portbase net::used_ports

    if {[net::num_nodes] == 0} {
	net::num_nodes [llength $ids]
    }

    array set oldhost  [array get net::host]
    array set oldports [array get net::portbase]

    array unset net::host
    array unset net::portbase
    array unset net::used_ports

    for {set id 0} {$id < [llength $ids]} {incr id} {
	set new_id [lindex $ids $id]
	if {$remap} {
	    set old_id $id
	} else {
	    set old_id $new_id
	}

	set net::host($new_id)       $oldhost($old_id)
	set net::portbase($new_id)   $oldports($old_id)
	set net::used_ports($new_id) {}
    }
}

#
# Return a list of ids for the defined nodes
#
proc nodelist {} {
    global net::total_nodes net::host
    return [lrange [lsort -increasing \
	    [array names net::host]] 0 [expr $net::total_nodes - 1]]
}

#
# Return a list of the defined hosts for the nodes in the test
#
proc hostlist {} {
    global net::host
    set hosts {}
    foreach id [net::nodelist] {
	lappend hosts $net::host($id)
    }
    return $hosts
}


# 
# @return 1 if host is localhost, o.w. 0 
#
proc is_localhost { host } {
    if {[string equal $host "localhost"]    ||
    [regexp {172\.16\.\d+\.\d+}  $host] ||
    [regexp {192\.168\.\d+\.\d+} $host] ||
    [regexp {10\.\d+\.\d+\.\d+}  $host]} {
	return 1
    }

    return 0
}

# namespace eval net
}
