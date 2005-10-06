#
# Manage network topology
#
namespace eval net {

#
# Number of nodes used in the test
#
set nodes 0

#
# Number of nodes defined by the network definition file
#
set defined_nodes 0
    
#
# Define a new node in the test
#
proc node { node_id hostname new_portbase {new_extra {} } } {
    global net::host net::portbase net::extra net::defined_nodes
    global opt
    
    set  net::host($node_id)      $hostname
    set  net::portbase($node_id)  [expr $portbase + $opt(conf_id) * 100]
    set  net::extra($node_id)     $new_extra
    incr net::defined_nodes
}

#
# Set the number of nodes for the test.
#
proc num_nodes { {num -1} } {
    global net::nodes net::defined_nodes
    if {$num != -1} {
	if {$num > $net::defined_nodes} {
	    error "Cannot set number of nodes ($num) greater than the number \
		    defined ($net::defined_nodes)"
	}
	set net::nodes $num
    }
    return $net::nodes
}

#
# Set the number of nodes for the test, ignoring the setting if the
# user has already overridden it on the command line with -n <num>.
#
proc default_num_nodes {num} {
    global net::nodes 
    if {$net::nodes == 0} {
	num_nodes $num
    }
}

#
# Return a list of integers from 0 to num_nodes
#
proc nodelist {} {
    global net::nodes
    set ret {}
    for {set i 0} {$i < $net::nodes} {incr i} {
	lappend ret $i
    }
    return $ret
}

#
# Return a list of the defined hosts for the number of nodes in the test
#
proc hostlist {} {
    global net::host net::nodes
    set hosts {}

    for {set i 0} {$i<$net::nodes} {incr i} {
	lappend hosts $net::host($i)
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
