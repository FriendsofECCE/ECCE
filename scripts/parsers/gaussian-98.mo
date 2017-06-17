#!/usr/bin/env perl
###############################################################################
#
# Name:   $Source$
#
#
# Purpose:
#
# Description:
#
#
#  The input stream is essentially taken directly from the g94 fort.7
#  file so look at one of those to understand the incoming format.
#  For U* theories both alpha and beta MOs will appear and must be
#  parsed into separate properties.  The alphas come first followed by
#  the betas.
#
###############################################################################

# provides loadPT to setup periodic table hashes
push(@INC,"$ENV{ECCE_HOME}/scripts/parsers");
require "pertab.pl";
&loadPT;

# Force output to be flushed
$| = 1;

# These change should we happen to find beta data in the parse stream.
$whichmo = "alpha";
$ekey = "ORBENG";
$mokey = "MO";

$norb = 0;
$nBasisFun = 0;
$occupied = 0;
$occno = 0.0;  # temporary patch, need real value!
$orbEngValues = "";
$occValues = "";
$values = "";
$loopValue = 0;

while (<STDIN>) {
  if ( /^\(/ || /^\s*\d\s*$/) {
    last;
  }
}
while (<STDIN>) {
  chop;
  if ( /^\(/ ) {
    next;
  } elsif ( /^\s*(\d+) (\S+)\s*OE=\s*(\S+)/ ) {
    $OrbEnergy = $3;    # make sure to assign this before clobbering it
    if ($2 =~ /Beta/ && $whichmo eq "alpha") {
      $whichmo = "beta";
      &printThem($ekey,$mokey);
      $ekey="ORBENGBETA";
      $mokey="MOBETA";
      $norb = 0;
      $nBasisFun = 0;
      $occupied = 0;
      $occno = 0.0;  # temporary patch, need real value!
      $orbEngValues = "";
      $occValues = "";
      $values = "";
      $loopValue = 0;
    }
    $norb++;
    $OrbEnergy =~ s/D/e/;  # replace D exponent with e
    $orbEngValues .= " $OrbEnergy";
    if (($loopValue % 4) == 0) {
      $orbEngValues .= "\n";
    }
    if (($OrbEnergy >= 0.0) && ($occupied == 0)) {
      $occupied = $norb - 1;
    }
    $occValues .= " $occno";
    if (($loopValue % 10) == 0) {
      $occValues .= "\n";
    }
    $loopValue++;
  } elsif ($_ eq "\001") {
    last;
  } else {  # just a line of coefficients
    s/D(...)/e$1 /g;  # replace D exponent with e
    if ($norb == 1) {
      s/^\s+//;  # delete lead whitespace (for correct field count)
      $temp = split;
      $nBasisFun += $temp;
    }
    $values .= "$_\n";
  }
}

&printThem($ekey,$mokey);
exit(0);


#
# Print the contents in the global structures (sorry but its easiest)
# using the property keys that are passed in.
#
sub printThem {

  ($ekey, $mokey) = @_;

  ## ORBITAL ENERGIES
  print "key: $ekey\n";
  print "size:\n$norb\n";
  print "rowlabels:\n";
  for ($index = $occupied; $index > 0; $index--) {
    print "LUMO-$index ";
    if (($index % 10) == 0) {
      print "\n";
    }
  }
  print "\n";
  for ($index = 1; $index <= ($norb - $occupied); $index++) {
    print "HOMO-$index ";
    if (($index % 10) == 0) {
      print "\n";
    }
  }
  print "\n";
  print "values:\n";
  print "$orbEngValues\n";
  print "units:\nHartree\n";
  print "END\n";

  ## ORBITAL OCCUPATIONS
  print "key: $mokey\n";
  print "size:\n$norb $nBasisFun\n";
  print "rowlabels:\n";
  for ($index = 1; $index <= $nBasisFun; $index++) {
    print "BasFun-$index ";
    if (($index % 8) == 0) {
      print "\n";
    }
  }
  print "\ncolumnlabels:\n";
  for ($index = $occupied; $index > 0; $index--) {
    print "LUMO-$index ";
    if (($index % 10) == 0) {
      print "\n";
    }
  }
  print "\n";
  for ($index = 1; $index <= ($norb - $occupied); $index++) {
    print "HOMO-$index ";
    if (($index % 10) == 0) {
      print "\n";
    }
  }
  print "\n";
  print "values:\n";
  print "$values\n";
  print "END\n";
}
