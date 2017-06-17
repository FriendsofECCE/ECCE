#!/usr/bin/env perl
###############################################################################
#
# $Source$
#
#
# Purpose:
#   Parse MO MOBETA properties.
#
# Description:
#
###############################################################################

# Force output to be flushed
$| = 1;

# This script provides either MO and MOBETA
$whichprop = "MO";

########################################################################
# get and print the orbital occupations.  
########################################################################
$firstline = <STDIN>;

# Is this a beta MO?
if ($firstline =~ /beta/) { $whichprop = "MOBETA"; }

$firstline =~ /%(\d+)\s+(\d+)%double/;
$nbas = $1;
$nmo = $2;  # the number of MOs in this block (could be alpha, beta, or ROHF)
$numMissingVirtuals = $nbas - $nmo;

print "key: $whichprop\n";
print "size:\n$nmo $nbas\n"; # was $nmo, $nbas, but MO tool assumes square
print "rowlabels:\n";
foreach $i (1 .. $nmo) {  # was 1 .. $nmo, but MO tool assumes square
  print "MO-$i ";
  if (($i % 8) == 0 ) {
    print "\n";
  }
}
print "\ncolumnlabels:\n";
foreach $i (1 .. $nbas) { 
  print "BasFun-$i ";
  if (($i % 6) == 0) {
    print "\n";
  }
}

print "\nvalues:\n";

while (<STDIN>) {
  last if (/end/);
  if ( /\*/ ) {
    chop;
    s/^ +//g;
    s/ +$//g;
    @line = split(' +',$_);
    $nprint = 0;
    foreach $i (@line) {
      if ( $i =~ /(\d+)\*0\.0/ ) {
        $n = $1;
        while ($n-- > 0) { 
	  print "0.0 "; 
	  if ( (++$nprint % 10) == 0) { print "\n";$nprint=0;}
	}
      } elsif ($i !~ /^ +$/) {
        print "$i ";
        if ( (++$nprint % 10) == 0 ) { print "\n";$nprint=0;}
      }
    }
    if ($nprint != 0) {print " \n";}

  } else {
    print;
  }
}

print "END\n";
exit(0);
