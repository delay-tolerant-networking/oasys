#
# Manage network topology
#
namespace eval net {

set nodes 0
    
# Network definitions
proc node { node_id hostname new_portbase {new_extra {} } } {
    global net::host net::portbase net::extra net::nodes

    set  net::host($node_id)      $hostname
    set  net::portbase($node_id)  $new_portbase
    set  net::extra($node_id)     $new_extra
    incr net::nodes
}

proc num_nodes { {num -1} } {
    global net::nodes
    if {$num != -1} {
	set net::nodes $num
    }
    return $net::nodes
}

proc nodelist {} {
    global net::nodes
    set ret {}
    for {set i 0} {$i < $net::nodes} {incr i} {
	lappend ret $i
    }
    return $ret
}

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