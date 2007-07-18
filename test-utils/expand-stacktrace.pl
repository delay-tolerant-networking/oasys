#!/usr/bin/perl

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



use English;
select(STDOUT); $OUTPUT_AUTOFLUSH = 1; # make unbuffered

$dir = $0;
$dir =~ s|/[^/]+$||;
$print_stacktrace = "$dir/print-stacktrace.pl";

$OS = `uname -s`;
chomp($OS);

while (<STDIN>) {
    if (m/STACK TRACE: /) {
	s/STACK TRACE: //;
	print "** STACK TRACE **\n\n";

	if ($OS eq "Darwin") {
	    print $_;
	    print "\n";
	} else {
	    open(PRINTER, "| $print_stacktrace " . join(' ', @ARGV));
	    print PRINTER $_;
	    close(PRINTER);
	}
    } else {
	print "$_"
    }
}
