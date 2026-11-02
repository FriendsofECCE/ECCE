# Receptor/ligand split shared by every docking code's ai.<code> (xtb dock now,
# AutoDock Vina next).  The split is stored with the calculation as the key
#   ES.Runtype.Docking.LigandAtoms   1-based atom numbers in the Builder's order,
#                                    "1-5,9"; empty = automatic
#   ES.Runtype.Docking.LigandCharge  charge of the ligand (the receptor gets the
#                                    rest of the system charge)
# Automatic: the smallest of the separate molecules (atoms closer than 1.25 x the
# sum of covalent radii are bonded), which is the guest in a host-guest pair.
# dockSplit() returns two lists of 0-based atom indices, each in input order.
use strict;

# Covalent radii (Angstrom); for Li, Na, K, Mg and Ca the ionic radius, since a
# bonded metal ion in a host-guest complex is a guest, not part of the host.
my %dockRadius = (H=>0.31,He=>0.28,Li=>0.68,Be=>0.96,B=>0.84,C=>0.76,N=>0.71,
  O=>0.66,F=>0.57,Ne=>0.58,Na=>0.97,Mg=>0.72,Al=>1.21,Si=>1.11,P=>1.07,S=>1.05,
  Cl=>1.02,Ar=>1.06,K=>1.33,Ca=>0.99,Fe=>1.32,Co=>1.26,Ni=>1.24,Cu=>1.32,
  Zn=>1.22,Br=>1.20,I=>1.39);

# $tags: ref to hash index => symbol; $coords: ref to hash "i,k" => value.
sub dockSplit {
  my ($dict, $natoms, $tags, $coords) = @_;
  my $spec = $dict->{"ES.Runtype.Docking.LigandAtoms"};
  $spec = "" if (!defined($spec));
  my %lig;
  if ($spec =~ /\S/) {
    foreach my $part (split(/[,\s]+/, $spec)) {
      next if ($part eq "");
      if ($part =~ /^(\d+)-(\d+)$/ && $1 <= $2) {
        $lig{$_ - 1} = 1 foreach ($1 .. $2);
      } elsif ($part =~ /^\d+$/) {
        $lig{$part - 1} = 1;
      } else {
        die "docking error: ligand atoms '$spec': '$part' is not a number or a range\n";
      }
    }
    foreach my $i (keys %lig) {
      die "docking error: ligand atom " . ($i + 1) . " is not in the system " .
          "($natoms atoms)\n" if ($i < 0 || $i >= $natoms);
    }
  } else {
    my @comp = (-1) x $natoms;
    my $n = 0;
    for my $s (0 .. $natoms - 1) {
      next if ($comp[$s] >= 0);
      my @stack = ($s);
      $comp[$s] = $n;
      while (@stack) {
        my $i = pop(@stack);
        for my $j (0 .. $natoms - 1) {
          next if ($comp[$j] >= 0);
          my $d2 = 0;
          $d2 += ($coords->{$i,$_} - $coords->{$j,$_}) ** 2 foreach (0 .. 2);
          my $ri = $dockRadius{$tags->{$i}}; $ri = 1.5 if (!defined($ri));
          my $rj = $dockRadius{$tags->{$j}}; $rj = 1.5 if (!defined($rj));
          if ($d2 < (1.25 * ($ri + $rj)) ** 2) {
            $comp[$j] = $n;
            push(@stack, $j);
          }
        }
      }
      $n++;
    }
    die "docking error: the system is one molecule; give the ligand atoms in " .
        "the Runtype Details\n" if ($n < 2);
    my @size = (0) x $n;
    $size[$_]++ foreach (@comp);
    my $best = 0;
    for my $c (1 .. $n - 1) { $best = $c if ($size[$c] < $size[$best]); }
    for my $i (0 .. $natoms - 1) { $lig{$i} = 1 if ($comp[$i] == $best); }
  }
  my (@receptor, @ligand);
  for my $i (0 .. $natoms - 1) {
    push(@{$lig{$i} ? \@ligand : \@receptor}, $i);
  }
  die "docking error: the receptor or the ligand has no atoms\n"
      if (!@receptor || !@ligand);
  return (\@receptor, \@ligand);
}

1;
