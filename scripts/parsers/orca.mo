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
# ORCA's own row order is NOT sorted by angular momentum for a NAMED
# basis: a def2-SVP transition metal prints e.g. "S,S,S,S,S,P,P,D,D,P,F"
# (a polarization p shell after the d shells), because ORCA prints
# shells in its internal library order, not grouped by l. ECCE's own
# basis storage (TGBSConfig, and everything that enumerates it --
# BasisFlatten::flatten, ComputeMoCmd, MoCoeffs, MoDiagramPanel,
# MullikenPanel) is always l-sorted per atom. Left unreordered, the
# coefficient columns are silently assigned to the wrong basis function
# from the D shell onward for any such element: MOs no longer satisfy
# cT S c = 1 (issue: CrO/Cr(CO)6 norms 0.7-105 before this fix). Each
# row carries its own AO label (e.g. "1s", "2pz", "1dxy", "1f+3"), which
# is enough to recover the angular momentum and do a per-atom, STABLE
# sort by l -- "stable" so shells of the same l keep ORCA's relative
# order, which is what makes this match ECCE's own order (verified by
# re-parsing and checking cT S c on real jobs, not assumed).
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


# Angular momentum letter -> l, for the AAO label ("1s"/"2pz"/"1dxy"/
# "1f+3"/...) ORCA prints on each coefficient row.
my %L_OF_LETTER = (s=>0, p=>1, d=>2, f=>3, g=>4, h=>5, i=>6);

# Given the (atomIndex, aoLabel) of every row IN THE ORIGINAL ORCA ORDER,
# return the permutation new-row-index -> old-row-index that puts them
# in ECCE's canonical order: grouped by atom (already contiguous in
# ORCA's own output), stably sorted by angular momentum within each atom.
sub canonicalRowOrder {
  my @rowMeta = @_;  # array of [atomIndex, aoLabel]
  my @order;
  my $start = 0;
  while ($start <= $#rowMeta) {
    my $atomIdx = $rowMeta[$start][0];
    my $end = $start;
    $end++ while ($end <= $#rowMeta && $rowMeta[$end][0] == $atomIdx);
    my @withKey;
    for my $i ($start .. $end-1) {
      my ($letter) = ($rowMeta[$i][1] =~ /^\d*([A-Za-z])/);
      push(@withKey, [$i, $L_OF_LETTER{lc($letter)}]);
    }
    # Perl's sort is stable (guaranteed since 5.8), so shells sharing an
    # l keep ORCA's own relative order.
    push(@order, map { $_->[0] } sort { $a->[1] <=> $b->[1] } @withKey);
    $start = $end;
  }
  return @order;
}

sub parseSegment {
  my @seglines = @{$_[0]};
  my (@orbEnergy, @orbOcc, %coeff);
  my $nbas = 0;
  my $i = 0;
  my @rowMeta;
  my $rowOrder;  # computed once, from the first column-group's rows
  while ($i < @seglines) {
    # header line: only integers and whitespace
    last if ($seglines[$i] !~ /^\s*\d+(\s+\d+)*\s*$/);
    my @idx = split(' ', $seglines[$i]); $i++;
    my @en  = &splitFixed($seglines[$i]); $i++;
    my @occ = &splitFixed($seglines[$i]); $i++;
    $i++; # dashed separator line, discard
    my $row = 0;
    while ($i < @seglines &&
           $seglines[$i] =~ /^\s*(\d+)([A-Za-z]+)\s+(\S+)\s+(.+)$/) {
      my ($atomIdx, $aoLabel, $rest) = ($1, $3, $4);
      my @vals = &splitFixed($rest);
      for my $j (0 .. $#idx) {
        $coeff{$idx[$j]}{$row} = $vals[$j];
      }
      push(@rowMeta, [$atomIdx, $aoLabel]) if (!defined $rowOrder);
      $row++;
      $i++;
    }
    $nbas = $row if ($row > $nbas);
    $rowOrder = [ &canonicalRowOrder(@rowMeta) ] if (!defined $rowOrder);
    for my $j (0 .. $#idx) {
      $orbEnergy[$idx[$j]] = $en[$j];
      $orbOcc[$idx[$j]] = $occ[$j];
    }
  }
  if (defined $rowOrder && $nbas > 0) {
    my %reordered;
    for my $newRow (0 .. $#$rowOrder) {
      my $oldRow = $rowOrder->[$newRow];
      for my $col (keys %coeff) {
        $reordered{$col}{$newRow} = $coeff{$col}{$oldRow};
      }
    }
    %coeff = %reordered;
  }
  return (\@orbEnergy, \@orbOcc, \%coeff, $nbas);
}

sub printMO {
  my ($moKey, $engKey, $occKey, $orbEnergyRef, $orbOccRef, $coeffRef, $nbas) = @_;
  my @orbEnergy = @$orbEnergyRef;
  my @orbOcc = @$orbOccRef;
  my %coeff = %$coeffRef;
  my $nmo = scalar(@orbEnergy);
  return 0 if ($nmo == 0 || $nbas == 0);

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
  return 1;
}

my $printedAny = 0;
if (@segments >= 1) {
  my ($orbEnergy, $orbOcc, $coeff, $nbas) = parseSegment($segments[0]);
  $printedAny |= printMO("MO", "ORBENG", "ORBOCC", $orbEnergy, $orbOcc, $coeff, $nbas);
}
if (@segments >= 2) {
  my ($orbEnergy, $orbOcc, $coeff, $nbas) = parseSegment($segments[1]);
  $printedAny |= printMO("MOBETA", "ORBENGBETA", "ORBOCCBETA", $orbEnergy, $orbOcc, $coeff, $nbas);
}

# Tells the C++ side (MoAoOrder::reorderToNative()) that MO/MOBETA's
# basis-function columns are in canonical (atom, l, shell-of-that-l)
# order rather than whatever order ORCA printed them in -- see
# canonicalRowOrder() above. Absent on older/other-code data, which
# means "native order, leave alone".
if ($printedAny) {
  print "key: MOAOORDER\n";
  print "size:\n1\n";
  print "values:\nangular-momentum\n";
  print "END\n";
}

exit(0);
