#
#    Copyright 2006 Intel Corporation
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
# Various status for the running test processes
#

proc help {} {
    puts "commands: help status xterm"
}

proc status {} {
    foreach id [net::nodelist] {
	set dir $::dist::distdirs($id)
	set hostname $::net::host($id)

	if [run::check_pid $hostname $::run::pids($id)] {
	    set status "running"
	} else {
	    set status "dead"
	}
	puts "host $id $hostname $dir $status"
    }
}

proc xterm {args} {
    foreach id [net::nodelist] {
	set dir $::dist::distdirs($id)
	set hostname $::net::host($id)
	
	# ssh will barf trying to lock the xauthority XXX/bowei --
	# this still has some mysterious problems and interactions
	# with the command prompt
	after 100
	
	eval "exec ssh -X $hostname $args \"cd $dir; xterm\" &"
    }
}

namespace eval status {
    proc get_distdirs {} {
	global ::dist::distdirs
	return [array get ::dist::distdirs]
    }
}
