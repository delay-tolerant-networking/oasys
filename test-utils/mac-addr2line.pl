#!/usr/bin/perl -w

#
# BD: I hate perl
#
# mac-addr2line <exe> <address1> [address2 ...]
#
# XXX/bowei - this seems to be broken, but I'm too lazy to figure out
# why
#

# Does a binary search for the entry in symtab. XXX/bowei - actually
# do binary search.
sub find_entry
{
    # Unfortunately there is some fuzz in the addresses at the
    # beginning of the nm dump for some stupid reason.
    
    my $addr = shift;
    my $idx  = 0;
    
#    print "addr = $addr\n";
    while ($idx <= $#symtab) {
#	print $symtab[$idx]{"addr"} . " >= " . $addr . "\n";
	if ($addr >= $symtab[$idx]{"addr"} && $addr <  $symtab[$idx]{"addr_end"}) {
	    return $symtab[$idx];
	}
	++$idx;
    }

    return 0;
}

# demangle a symbol name
sub demangle
{
    my $symname = shift;
    open(DEMANGLE, "echo $symname | $DEMANGLE_EXE |") || die "Can't run demangler $DEMANGLE_EXE";
    my $d = <DEMANGLE>;
    close DEMANGLE;
    chomp($d);
    return $d;
}

local $NM_EXE="nm";
local $DEMANGLE_EXE="c++filt";

$ARGV[0] || die "Need to supply a binary exe.";
my $exe = $ARGV[0];
shift @ARGV;

my @addr;
while ($ARGV[0]) {
    push(@addr, $ARGV[0]);
    shift;
}

($#addr >= 0) || die "Need at least one address.";

# print("bin=$exe\n");
# foreach (@addr) {
#     print "addr=$_\n";
# }

local @symtab;
open(SYM, "$NM_EXE -n $exe | sort -n |") || die "Can't run $NM_EXE on $exe";

my $i    = 0;
my $addr = 0;
my $type = "-";
my $sym  = "start";
my $prev = 0;

# XXX/bowei -- This loop missed the last symbol, but it's probably not
# needed anyways.
while (<SYM>) {
    if ($_ =~ m/([0-9a-f]+) ([a-zA-Z]) (.+)/) {
	# print "$num_addr $addr $type $sym\n";
	($next_addr, $next_type, $next_sym) = ($1, $2, $3);
	$symtab[$i] = {
	    addr     => hex($addr), 
	    addr_end => hex($next_addr), 
	    hex_addr => $addr, 
	    type     => $type, 
	    sym      => $sym
	};
	($addr, $type, $sym) = ($next_addr, $next_type, $next_sym);
	$i++;
    }
}

foreach (@addr) {
    my $addr = $_;
    my $sym = find_entry(hex($addr))->{sym};

    if (! $sym) {
	printf("%08x\tsymbol lookup failure\n", hex($addr));
    } else {
	my $dsym = demangle($sym);
	printf("%08x\t%s\n", hex($addr), $dsym);
    }
}
