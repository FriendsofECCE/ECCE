# Receptor/ligand split shared by every docking code's ai.<code> (xtb dock now,
# AutoDock Vina next).  The split is stored with the calculation as the key
#   ES.Runtype.Docking.LigandAtoms   1-based atom numbers in the Builder's order,
#                                    "1-5,9"; empty = automatic
#   ES.Runtype.Docking.LigandCharge  charge of the ligand (the receptor gets the
#                                    rest of the system charge)
# Automatic: the smallest of the separate molecules (atoms closer than 1.25 x the
# sum of covalent radii are bonded), which is the guest in a host-guest pair.
# Water and monatomic ions are never the ligand when something else is left; and
# where a polymer (standard amino-acid or nucleotide residues) is present the
# ligand is the largest molecule that is not the polymer.  The optional fifth
# argument, hash index => {name,num,chain} from the .frag's pdb_list, gives the
# residue names; without it only the element/size rules apply.
# Messages go to STDERR as "ai.<code> warning [LIGAND_ATOMS]: ..." lines, which
# the Calculation Editor shows under Verify.
# dockSplit() returns two lists of 0-based atom indices, each in input order.
use strict;

# Covalent radii (Angstrom); for Li, Na, K, Mg and Ca the ionic radius, since a
# bonded metal ion in a host-guest complex is a guest, not part of the host.
my %dockRadius = (H=>0.31,He=>0.28,Li=>0.68,Be=>0.96,B=>0.84,C=>0.76,N=>0.71,
  O=>0.66,F=>0.57,Ne=>0.58,Na=>0.97,Mg=>0.72,Al=>1.21,Si=>1.11,P=>1.07,S=>1.05,
  Cl=>1.02,Ar=>1.06,K=>1.33,Ca=>0.99,Fe=>1.32,Co=>1.26,Ni=>1.24,Cu=>1.32,
  Zn=>1.22,Br=>1.20,I=>1.39);

# $tags: ref to hash index => symbol; $coords: ref to hash "i,k" => value.
my %dockWater = map { $_ => 1 } qw(HOH WAT H2O DOD TIP TIP3 SOL T3P SPC);
my %dockPolymer = map { $_ => 1 } qw(ALA ARG ASN ASP CYS GLN GLU GLY HIS ILE LEU
  LYS MET PHE PRO SER THR TRP TYR VAL HID HIE HIP HSD HSE HSP CYX CYM ASH GLH LYN
  MSE SEC PYL A C G U T DA DC DG DT DU ADE CYT GUA URA THY);

# Advice about the split (automatic choice, waters, repeated chains): a
# "note", which the editor shows as worth checking, never as an error.
sub dockWarn {
  my ($text) = @_;
  my $prog = $0;
  $prog =~ s{.*/}{};
  print STDERR "$prog note [LIGAND_ATOMS]: $text\n";
}

sub dockRanges {
  my @n = sort { $a <=> $b } map { $_ + 1 } @_;
  my @out;
  while (@n) {
    my $lo = shift(@n);
    my $hi = $lo;
    $hi = shift(@n) while (@n && $n[0] == $hi + 1);
    push(@out, $lo == $hi ? "$lo" : "$lo-$hi");
  }
  return join(",", @out);
}

sub dockSplit {
  my ($dict, $natoms, $tags, $coords, $res) = @_;
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
    dockReceptorWarnings(\%lig, $natoms, $res, undef);
  } else {
    # Bonded neighbours through a grid of cells as large as the longest
    # possible bond: a protein with its waters (~5000 atoms) took 17 s with
    # every pair compared, a fraction of a second this way.
    my (@x, @y, @z, @r);
    my $rmax = 0;
    for my $i (0 .. $natoms - 1) {
      ($x[$i], $y[$i], $z[$i]) = map { $coords->{$i,$_} } (0 .. 2);
      $r[$i] = $dockRadius{$tags->{$i}};
      $r[$i] = 1.5 if (!defined($r[$i]));
      $rmax = $r[$i] if ($r[$i] > $rmax);
    }
    my $cs = 1.25 * 2 * $rmax;
    my %cell;
    my @key;
    for my $i (0 .. $natoms - 1) {
      my @c = map { int(($_ < 0) ? $_ / $cs - 1 : $_ / $cs) } ($x[$i], $y[$i], $z[$i]);
      $key[$i] = \@c;
      push(@{$cell{"@c"}}, $i);
    }
    my @comp = (-1) x $natoms;
    my $n = 0;
    for my $s (0 .. $natoms - 1) {
      next if ($comp[$s] >= 0);
      my @stack = ($s);
      $comp[$s] = $n;
      while (@stack) {
        my $i = pop(@stack);
        my ($a, $b, $c) = @{$key[$i]};
        for my $da (-1 .. 1) { for my $db (-1 .. 1) { for my $dc (-1 .. 1) {
          my $list = $cell{($a + $da) . " " . ($b + $db) . " " . ($c + $dc)};
          next if (!$list);
          for my $j (@$list) {
            next if ($comp[$j] >= 0);
            my $d2 = ($x[$i] - $x[$j]) ** 2 + ($y[$i] - $y[$j]) ** 2 + ($z[$i] - $z[$j]) ** 2;
            if ($d2 < (1.25 * ($r[$i] + $r[$j])) ** 2) {
              $comp[$j] = $n;
              push(@stack, $j);
            }
          }
        }}}
      }
      $n++;
    }
    die "docking error: the system is one molecule; give the ligand atoms in " .
        "the Runtype Details\n" if ($n < 2);
    my (@size, @members);
    for my $i (0 .. $natoms - 1) {
      $size[$comp[$i]]++;
      push(@{$members[$comp[$i]]}, $i);
    }
    # Per component: is it water, an ion, the polymer; what is it called.
    my (@water, @ion, @poly, @name);
    for my $c (0 .. $n - 1) {
      my @m = @{$members[$c]};
      my (%rn, $poly, $o, $h);
      foreach my $i (@m) {
        my $rn = $res && $res->{$i} ? $res->{$i}{name} : "";
        $rn{$rn}++;
        $poly++ if ($dockPolymer{$rn});
        $o++ if ($tags->{$i} eq "O");
        $h++ if ($tags->{$i} eq "H" || $tags->{$i} eq "D");
      }
      $ion[$c] = (@m == 1);
      $water[$c] = (@m <= 3 && $o == 1 && $o + $h == @m) ||
                   (keys(%rn) == 1 && $dockWater{(keys %rn)[0]});
      $poly[$c] = ($poly && $poly * 2 > @m);
      $name[$c] = join("/", grep { $_ ne "" } sort keys %rn);
    }
    my @cand = grep { !$water[$_] && !$ion[$_] } (0 .. $n - 1);
    my $best;
    if (@cand >= 2) {
      my @np = grep { !$poly[$_] } @cand;
      if (@np < @cand && @np) {
        # A polymer is present: the largest other molecule is the ligand.
        $best = $np[0];
        foreach my $c (@np) { $best = $c if ($size[$c] > $size[$best]); }
      } else {
        $best = $cand[0];
        foreach my $c (@cand) { $best = $c if ($size[$c] < $size[$best]); }
      }
    } else {
      $best = 0;
      for my $c (1 .. $n - 1) { $best = $c if ($size[$c] < $size[$best]); }
    }
    $lig{$_} = 1 foreach (@{$members[$best]});
    my $what = $name[$best] ne "" ? "residue $name[$best], " : "";
    dockWarn("the ligand was chosen automatically: ${what}$size[$best] atoms " .
             "(atoms " . dockRanges(@{$members[$best]}) . "). If that is not " .
             "the molecule to dock, give the Ligand atoms in the Runtype Details.");
    my $nw = grep { $water[$_] && $_ != $best } (0 .. $n - 1);
    dockReceptorWarnings(\%lig, $natoms, $res, $nw);
  }
  my (@receptor, @ligand);
  for my $i (0 .. $natoms - 1) {
    push(@{$lig{$i} ? \@ligand : \@receptor}, $i);
  }
  die "docking error: the receptor or the ligand has no atoms\n"
      if (!@receptor || !@ligand);
  return (\@receptor, \@ligand);
}

# Warn (never block) when the receptor still holds waters or several identical
# chains; the user decides whether to remove them.  $nwater is the molecule
# count when the caller found the components, else it is counted by residue name.
sub dockReceptorWarnings {
  my ($lig, $natoms, $res, $nwater) = @_;
  my (%wat, %chain);
  if ($res) {
    for my $i (0 .. $natoms - 1) {
      next if ($lig->{$i} || !$res->{$i});
      my $r = $res->{$i};
      $wat{"$r->{chain}:$r->{num}"} = 1 if ($dockWater{$r->{name}});
      push(@{$chain{$r->{chain}}}, $r->{name} . $r->{num}) if ($dockPolymer{$r->{name}});
    }
  }
  $nwater = scalar(keys %wat) if (!defined($nwater) || (!$nwater && %wat));
  dockWarn("the receptor still holds $nwater water molecule" .
           ($nwater == 1 ? "" : "s") . ", which Vina treats as part of the " .
           "receptor and which can block the binding site. Remove them in " .
           "the Builder if that is not wanted.") if ($nwater);
  my %seq;
  foreach my $c (sort keys %chain) {
    my $key = join(" ", map { s/\d+$//r } @{$chain{$c}});
    push(@{$seq{$key}}, $c eq " " ? "(blank)" : $c);
  }
  foreach my $key (sort keys %seq) {
    next if (@{$seq{$key}} < 2);
    dockWarn("the receptor holds identical chains (" . join(", ", @{$seq{$key}}) .
             "); the ligand may be docked against any of them. Delete the " .
             "extra chains in the Builder if one site is wanted.");
  }
}

1;
