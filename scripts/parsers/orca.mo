#!/usr/bin/env perl
################################################################################
#
# Purpose:
# Parses ORCA's "MOLECULAR ORBITALS" block into MO/ORBENG/ORBOCC (and, for
# UHF/UDFT, MOBETA/ORBENGBETA/ORBOCCBETA). Requires ai.orca's unconditional
# "%output\n  Print[P_MOs] 1\nend\n" block -- without it ORCA only prints
# orbital energies/occupations (truncated to the first 10 virtuals) and
# leaves the coefficient matrix in the binary .gbw file.
#
# Real ORCA 6.1.1 shape (RHF/STO-3G H2O and UHF/STO-3G O2 test runs, this
# session), one "column-group" of up to 6 orbitals at a time, repeated
# until all orbitals are covered, e.g.:
#
#                       0         1         2         3         4         5
#                  -20.24383  -1.26327  -0.61112  -0.45287  -0.39092   0.59535
#                    2.00000   2.00000   2.00000   2.00000   2.00000   0.00000
#                   --------  --------  --------  --------  --------  --------
#   0O   1s        -0.994158 -0.233196 -0.000000  0.102896 -0.000000  0.130544
#   ...
#                       6
#                    0.72749
#                    0.00000
#                   --------
#   0O   1s         0.000000
#   ...
#
# Column-groups within one spin channel are NOT separated by a blank line;
# a blank line only appears between the alpha and beta channels for UHF/UDFT
# (confirmed: RHF prints one unbroken run of groups, UHF prints alpha
# groups, one blank line, then beta groups with different eigenvalues)
# -- so splitting the whole block on blank lines cleanly separates spins.
#
################################################################################

$| = 1;

($key, $runtype, $ucCategory, $theory) = @ARGV;

my @lines;
while (<STDIN>) {
  chomp;
  push(@lines, $_);
}

# Split into segments on blank lines; drop empty segments (trailing blanks
# before the next output section).
my @segments;
my @current;
foreach my $line (@lines) {
  if ($line =~ /^\s*$/) {
    push(@segments, [@current]) if (@current);
    @current = ();
  } else {
    push(@current, $line);
  }
}
push(@segments, [@current]) if (@current);

#  ORCA prints this block in FIXED-WIDTH columns, so a value wide enough to
#  fill its field runs straight into the previous one with no space between
#  them -- "0.452636-11.266403" is two coefficients, not one.  Splitting on
#  whitespace then silently yields one token too few for that row, and the
#  whole MO table ends up short: PropTable rejects it on load with "input
#  vector length does not match rows*columns" and the panel gets nothing.
#  Seen live with a coefficient of -11.266403 (issue #108).
#
#  A minus sign directly after a digit can only be the start of the next
#  number, never part of this one -- this block prints plain decimals, and
#  an exponent's sign follows an "e"/"E" rather than a digit, so that case
#  is left alone.
sub splitFixed {
  my $text = shift;
  $text =~ s/(?<=\d)-(?=\d)/ -/g;
  return split(' ', $text);
}


sub parseSegment {
  my @seglines = @{$_[0]};
  my (@orbEnergy, @orbOcc, %coeff);
  my $nbas = 0;
  my $i = 0;
  while ($i < @seglines) {
    # header line: only integers and whitespace
    last if ($seglines[$i] !~ /^\s*\d+(\s+\d+)*\s*$/);
    my @idx = split(' ', $seglines[$i]); $i++;
    my @en  = &splitFixed($seglines[$i]); $i++;
    my @occ = &splitFixed($seglines[$i]); $i++;
    $i++; # dashed separator line, discard
    my $row = 0;
    while ($i < @seglines &&
           $seglines[$i] =~ /^\s*\d+[A-Za-z]+\s+\S+\s+(.+)$/) {
      my @vals = &splitFixed($1);
      for my $j (0 .. $#idx) {
        $coeff{$idx[$j]}{$row} = $vals[$j];
      }
      $row++;
      $i++;
    }
    $nbas = $row if ($row > $nbas);
    for my $j (0 .. $#idx) {
      $orbEnergy[$idx[$j]] = $en[$j];
      $orbOcc[$idx[$j]] = $occ[$j];
    }
  }
  return (\@orbEnergy, \@orbOcc, \%coeff, $nbas);
}

sub printMO {
  my ($moKey, $engKey, $occKey, $orbEnergyRef, $orbOccRef, $coeffRef, $nbas) = @_;
  my @orbEnergy = @$orbEnergyRef;
  my @orbOcc = @$orbOccRef;
  my %coeff = %$coeffRef;
  my $nmo = scalar(@orbEnergy);
  return if ($nmo == 0 || $nbas == 0);

  print "key: $engKey\n";
  print "size:\n$nmo\n";
  print "values:\n";
  print join(" ", @orbEnergy), "\n";
  print "units:\nHartree\n";
  print "END\n";

  print "key: $occKey\n";
  print "size:\n$nmo\n";
  print "values:\n";
  print join(" ", @orbOcc), "\n";
  print "units:\nelectrons\n";
  print "END\n";

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
      print "$coeff{$m}{$b} ";
    }
    print "\n";
  }
  print "END\n";
}

if (@segments >= 1) {
  my ($orbEnergy, $orbOcc, $coeff, $nbas) = parseSegment($segments[0]);
  printMO("MO", "ORBENG", "ORBOCC", $orbEnergy, $orbOcc, $coeff, $nbas);
}
if (@segments >= 2) {
  my ($orbEnergy, $orbOcc, $coeff, $nbas) = parseSegment($segments[1]);
  printMO("MOBETA", "ORBENGBETA", "ORBOCCBETA", $orbEnergy, $orbOcc, $coeff, $nbas);
}
exit(0);
