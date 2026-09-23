#!/usr/bin/env perl
################################################################################
#
# Purpose:
# Parses MOPAC's "EIGENVECTORS" block into MO/ORBENG/ORBSYM (and
# MOBETA/ORBENGBETA/ORBSYMBETA for UHF).  Requires ai.mopac to put
# "VECTORS ALLVEC" on the keyword line -- without VECTORS the block is
# not printed at all, and without ALLVEC only the occupied set is.
#
# Real MOPAC 23.1.2 shape, column-groups of as many orbitals as fit the
# page width (6 for water, 8 for ethanol -- it is width-driven, never
# assume a fixed count):
#
#                 EIGENVECTORS
#
#    Root No.       1         2         3         4         5         6
#
#                  1 a1      1 b2      2 a1      1 b1      3 a1      2 b2
#
#                -29.619   -17.837   -14.219   -12.116     4.050     6.350
#
#   S   O    1    0.8516    0.0000    0.4261   -0.0000   -0.3055   -0.0000
#   Px  O    1   -0.0000    0.0000    0.0000    1.0000    0.0000    0.0000
#   ...
#
# Groups are separated by BLANK LINES, and the block as a whole ends with
# blank lines too -- the two are indistinguishable, which is why the
# .desc entry has to end on "NET ATOMIC CHARGES" and [MULLIKEN] is
# anchored past that line rather than on it.  Do not "fix" that by
# ending this block on a blank line.
#
# UHF prints "ALPHA EIGENVECTORS" and then "BETA EIGENVECTORS".  They do
# NOT arrive as two separate invocations: the .desc End is "NET ATOMIC
# CHARGES", which lies past both, so eccejobmonitor hands this script one
# block containing both spins.  Hence the per-spin accumulation below --
# an earlier version simply switched its output keys on seeing the BETA
# header and silently dropped the whole alpha set.  Skip=0 feeds the
# Begin-matching line so the spin headers are visible here at all.
#
# The symmetry label is TWO tokens ("1 a1"): an index within the irrep
# and the irrep itself.  Only the irrep is kept -- MoPanel::fillGraph()
# groups the orbital plot by this string, so keeping the index would put
# every orbital in a group of its own.
#
# MOPAC prints eigenvalues in eV; ECCE's ORBENG is Hartree (MoPanel
# hard-codes the column label "Energy Hartree"), so they are converted.
#
################################################################################

$| = 1;

use constant EV_PER_HARTREE => 27.211386245988;

#  Per spin: 0 = alpha (or the only set, for RHF), 1 = beta.
my @spin = ( { eng => [], sym => [], coeff => {}, nbas => 0 },
             { eng => [], sym => [], coeff => {}, nbas => 0 } );
my $cur = 0;
my $sawBeta = 0;

my @group;            # orbital indices of the group being read
my $row = 0;          # basis function row within the current group
my $sawHeader = 0;

while (<STDIN>) {
  chomp;
  my $line = $_;

  if ($line =~ /BETA\s+EIGENVECTORS/i) {
    $spin[$cur]{nbas} = $row if ($row > $spin[$cur]{nbas});
    $cur = 1;
    $sawBeta = 1;
    $row = 0;
    $sawHeader = 0;
    next;
  }
  next if ($line =~ /EIGENVECTORS/i);

  #  "Root No.  1  2  3 ..." opens a new column-group.
  if ($line =~ /^\s*Root No\.\s+(\d[\d\s]*)$/) {
    $spin[$cur]{nbas} = $row if ($row > $spin[$cur]{nbas});
    @group = split(' ', $1);
    $row = 0;
    $sawHeader = 1;
    next;
  }
  next if (!$sawHeader);
  next if ($line =~ /^\s*$/);

  #  Symmetry labels: pairs of "<n> <irrep>".  Recognised before the
  #  eigenvalue line because both are numeric-looking; this one always
  #  has an alphabetic token, which an eigenvalue row never does.
  if ($line =~ /^\s*\d+\s+[A-Za-z]/ && $line !~ /^\s*[A-Za-z]/) {
    my @tok = split(' ', $line);
    for (my $i = 0; $i + 1 <= $#tok; $i += 2) {
      my $idx = int($i / 2);
      last if ($idx > $#group);
      $spin[$cur]{sym}[$group[$idx] - 1] = $tok[$i + 1];
    }
    next;
  }

  #  Eigenvalues: a row of plain signed decimals and nothing else.
  if ($line =~ /^\s*-?\d+\.\d+(\s+-?\d+\.\d+)*\s*$/) {
    my @en = split(' ', $line);
    for my $i (0 .. $#en) {
      last if ($i > $#group);
      $spin[$cur]{eng}[$group[$i] - 1] = $en[$i] / EV_PER_HARTREE;
    }
    next;
  }

  #  Coefficient row: "<AO label> <element> <atom no> <values...>"
  if ($line =~ /^\s*[A-Za-z]+\s+\S+\s+\d+\s+(.+)$/) {
    my @vals = split(' ', $1);
    for my $i (0 .. $#vals) {
      last if ($i > $#group);
      $spin[$cur]{coeff}{$group[$i] - 1}{$row} = $vals[$i];
    }
    $row++;
    next;
  }
}
$spin[$cur]{nbas} = $row if ($row > $spin[$cur]{nbas});

#  A hole means a column-group was misread, so the vector no longer lines
#  up index for index with the others.  MoPanel reverses the energy,
#  occupancy and symmetry vectors INDEPENDENTLY and then indexes them all
#  by the same row, so a short or gappy vector shifts rather than running
#  out -- it mislabels every row.  Emit nothing instead.
sub complete {
  my ($ref, $n) = @_;
  return 0 if (scalar(@$ref) != $n);
  foreach my $x (@$ref) { return 0 if (!defined($x) || $x eq ""); }
  return 1;
}

sub emitSpin {
  my ($idx, $moKey, $engKey, $symKey) = @_;
  my $set  = $spin[$idx];
  my $nmo  = scalar(@{$set->{eng}});
  my $nbas = $set->{nbas};
  return if ($nmo == 0 || $nbas == 0);
  return if (!&complete($set->{eng}, $nmo));

  print "key: $engKey\n";
  print "size:\n$nmo\n";
  print "values:\n";
  printf("%.6f ", $_) foreach (@{$set->{eng}});
  print "\nunits:\nHartree\n";
  print "END\n";

  if (&complete($set->{sym}, $nmo)) {
    print "key: $symKey\n";
    print "size:\n$nmo\n";
    print "rowlabels:\n";
    foreach my $i (1 .. $nmo) { print " $i "; }
    print "\nvalues:\n";
    print join(" ", @{$set->{sym}}), "\n";
    print "END\n";
  }

  print "key: $moKey\n";
  print "size:\n$nmo $nbas\n";
  print "rowlabels:\n";
  for my $m (1 .. $nmo) {
    print "MO-$m ";
    print "\n" if (($m % 8) == 0);
  }
  print "\ncolumnlabels:\n";
  for my $b (1 .. $nbas) {
    print "BasFun-$b ";
    print "\n" if (($b % 6) == 0);
  }
  print "\nvalues:\n";
  for my $m (0 .. $nmo - 1) {
    for my $b (0 .. $nbas - 1) {
      my $c = $set->{coeff}{$m}{$b};
      print defined($c) ? "$c " : "0.0000 ";
    }
    print "\n";
  }
  print "END\n";
}

&emitSpin(0, "MO", "ORBENG", "ORBSYM");
&emitSpin(1, "MOBETA", "ORBENGBETA", "ORBSYMBETA") if ($sawBeta);
exit(0);
