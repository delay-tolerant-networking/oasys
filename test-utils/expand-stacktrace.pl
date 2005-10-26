#!/usr/bin/perl

use English;
select(STDOUT); $OUTPUT_AUTOFLUSH = 1; # make unbuffered

$dir = $0;
$dir =~ s|/[^/]+$||;

$print_stacktrace = "$dir/print-stacktrace.pl";

while (<STDIN>) {
    if (m/STACK TRACE: /) {
	s/STACK TRACE: //;
	print "STACK TRACE:\n";
	open(PRINTER, "| $print_stacktrace " . join(' ', @ARGV));
 	print PRINTER $_;
 	close(PRINTER);
    } else {
	print "$_"
    }
}
