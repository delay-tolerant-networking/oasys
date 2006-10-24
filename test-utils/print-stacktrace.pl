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


##
## print-stacktrace.pl
##
## Turn a stack trace into a list of function names.
##
## usage: symtrace [flags] object-file
##
## On stdin, expects a list of hex numbers on one line, for example
##	0x03993944 0x47483758 0xaf4d5630 0x09d9bcff
## Alternatively, for an address that falls inside a dynamically loaded library,
## you may give it more information of the form:
##      <ADDR>:<LIB_NAME>@<LOAD_ADDR>+<INSTRUCTION_OFFSET>
##
## Author: Patrick Tullmann <tullmann@cs.utah.edu>
## 	hacked up and somewhat defluked by Bart Robinson <lomew@cs.utah.edu>
##      simplified by Mike Demmer <demmer@cs.berkeley.edu>
##

use English;
select(STDOUT); $OUTPUT_AUTOFLUSH = 1; # make unbuffered

#default object file
$libOffset=0; # offset dynamic lib would be loaded at...

chop($OS = `uname`);

## Parse the command line arguments
$inputFormat = 'oneline';  # default format

while (@ARGV) {
    last unless $ARGV[0] =~ /^-/;
    ## -o <objectfile> to use a different object file
    if ($ARGV[0] eq "-o") {
	if ($#ARGV >= 1) {
	    $objectFile = $ARGV[1];
	    shift @ARGV;
	} else {
	    &usage;
	}
    ##
    } elsif ($ARGV[0] eq "-offset") {
	if ($#ARGV >= 1) {
	    # Allow 0x... or 0...
	    eval "\$libOffset = $ARGV[1]";
	    shift @ARGV;
	} else {
	    &usage;
	}
    } elsif ($ARGV[0] eq "-oneline") {
	$inputFormat = 'oneline';
    } elsif ($ARGV[0] eq "-perline") {
	$inputFormat = 'perline';
    ## -h for usage
    } elsif ($ARGV[0] eq "-h") {
	&usage;
    ## Unknown args are fatal
    } else {
	print "Unknown option $ARGV[0]\n";
	&usage;
    }
    shift @ARGV;
}

if (! defined($objectFile)) {
    $objectFile = shift @ARGV;
}
usage() unless defined($objectFile);
die "objectFile '$objectFile' not found.\n" unless -e $objectFile;

# print STDERR "Reading trace from stdin.\n";
@BACKTRACE = <STDIN>;

@BACKTRACE || die "No stdin provided??\n";
$ct = 0;

printf "Generating backtrace for object file $objectFile (offset %#08x)\n", $libOffset;

while (@BACKTRACE) {
    $traceLine = shift @BACKTRACE;
    chop $traceLine;
    
    next if ($traceLine eq "");

    if ($inputFormat eq 'perline') {
	# If old, eip-per-line format	
	($bs, $eip) = split(/=/, $traceLine, 2);
	&findFunction($eip);
    } elsif ($inputFormat eq 'oneline') {
	# Else, the one line of eips format 
	@eips = split(' ', $traceLine);
	foreach $eip (@eips) {
	    next if ($eip =~ /Backtrace/);
#	    &findFunction("0x" . "$eip");
	    &findFunction("$eip");
	}
    } else {
	die "unknown inputformat $inputformat\n";
    }
}

# Given an EIP, find the name of the corresponding function and file
# using the addr2line utility.  EIP should be a hex number with a
# leading '0x'.
sub findFunctionAddr2Line {
    local($file, $addr, $eip) = @_;
    local($first, $second);
    if (! open(ADDR2LINEOUT, "addr2line --demangle --functions -e $file $addr|")) {
	die "open: can't fork: $!\n";
    }
    $first = <ADDR2LINEOUT>;
    $second = <ADDR2LINEOUT>;
    chomp $first;
    chomp $second;
    if (! close(ADDR2LINEOUT)) {
	die "close: couldn't run addr2line, try with -noaddr2line\n";
    }
    printf "$eip in $first:$second\n";
}

sub findFunction {
    local($a) = @_ ;

    if ($a =~ /0x(\w+):([^:@]+)\@0x(\w+)\+0x(\w+)/) {
        # If we have DLL and offset info, use that.
        my $eip      = $1;
        my $file     = $2;
        my $obj_base = $3;
        my $offset   = $4;
	if ( !(-f $file) ) {
	    die "File $file does not exist\n";
	}
	if ( is_shared_lib($file) ) {
	    # For shared libraries we pass in the offset of the address
	    # relative to the base address at which the library/executable
	    # was loaded.
	    $addr = $offset;
	} else {
	    # For an executable, addr2line wants the
	    # actual instruction address, not an offset.
	    $addr = $eip;
	}
	findFunctionAddr2Line($file, $addr, "0x$eip");
    } else {
	findFunctionAddr2Line($objectFile, $a, $a);
    }
}

sub is_shared_lib {
    my $file = shift;

    my $follow_symlinks = "-L";
    if ($OS eq "SunOS") {
        $follow_symlinks = "";
    }

    chop(my $type = `file $follow_symlinks $file`);
    if (($type =~ /shared object/) || ($type =~ /dynamic lib/)) {
        return 1;
    } else {
        return 0;
    }
}

sub usage
{
    print "$0 [-o <objectfile>] [-perline|-oneline] [-nativenm] [-offset <num>] [-h] [object file]\n";
    exit 0;
}
