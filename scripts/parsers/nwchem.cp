#!/usr/bin/env perl
###############################################################################
#
# $Source$
#
#
# Purpose:
#
# Command line parameters:
#           
# Description:
#   Script used to output most of the NWChem Car-Parrinello scalar values
#
#   The change in the $key variable from database variable name to PID
#   number means that the scalar variable names must now be identified
#   by parsing the first line of text.
#
##############################################################################

($key, $runtype, $category, $theory) = @ARGV;

# Force output to be flushed
$| = 1;

$_ = <STDIN>;
$unit = "";
if ( /total energy/ ) {
  $propKey = "TECPVEC";
  $unit = "units:\na.u.\n";
} elsif ( /total kinetic/ ) {
  $propKey = "KECPVEC";
  $unit = "units:\na.u.\n";
} elsif ( /potential energy/ ) {
  $propKey = "PECPVEC";
  $unit = "units:\na.u.\n";
} elsif ( /electron kinetic/ ) {
  $propKey = "EKCPVEC";
  $unit = "units:\na.u.\n";
} elsif ( /ion kinetic/ ) {
  $propKey = "IKCPVEC";
  $unit = "units:\na.u.\n";
} elsif ( /time/ ) {
  $propKey = "TIMECPVEC";
  $unit = "units:\na.u.\n";
} elsif ( /orbital gradient norm/ ) {
  $propKey = "OGRADNORMCPVEC";
  $unit = "units:\na.u.\n";
} elsif ( /gradient norm/ ) {
  $propKey = "GRADNORMCPVEC";
  $unit = "units:\na.u.\n";
}
$value = <STDIN>;

print "key: $propKey\n";
print "size:\n1\n";
print "values:\n$value\n";
print "$unit";
print "END\n";
