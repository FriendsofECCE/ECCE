#!/usr/bin/env perl
################################################################################
# Parses the Molden file xtb writes with --molden (gensub appends it to the
# output between ECCE-XTB-MOLDEN / ECCE-XTB-ENDMOLDEN) into MO, ORBENG, ORBOCC
# (and the *BETA keys for --uhf), MOAOORDER and GTOBASIS.
#
# What the file is, checked against an independent overlap matrix built from
# its own [GTO] block (tests/parsers/xtb_trace_ps_test.py):
#   * the MO coefficients multiply UNNORMALISED primitives r^l exp(-a r^2),
#     and the contracted functions come out unit-normalised;
#   * the basis is NOT orthonormal (STO-nG expansions of xtb's Slater basis),
#     which is why the MO diagram works for xtb and not for MOPAC;
#   * s, p are x y z; d is CARTESIAN (6 functions, no [5D] flag) in the order
#     xx yy zz xy xz yz, and xy/xz/yz are unit-normalised: Gaussian's
#     convention, which xTB.edml selects with componentNormalized="true".
# Columns are put in ECCE's canonical order (atoms in order, shells of one
# atom stably sorted by l), and MOAOORDER says so.
################################################################################
$| = 1;

my %NF = (s => 1, p => 3, d => 6);
my %LV = (s => 0, p => 1, d => 2);
my $SQ3 = sqrt(3.0);

my @atomZ;
my @atomShells;          # per atom: [ [kind, [[exp, coef], ...]], ... ]
my @mo;                  # { spin, ene, occ, c => [] }
my $sec = "";
my $curAtom = -1;
my @lines = <STDIN>;
chomp(@lines);
for (my $i = 0; $i <= $#lines; $i++) {
  my $ln = $lines[$i];
  $ln =~ s/D([-+]\d)/E$1/g;
  if ($ln =~ /^\s*\[(\w+)\]/) { $sec = lc($1); next; }
  my @t = split(' ', $ln);
  next if (!@t);
  if ($sec eq "atoms") {
    push(@atomZ, $t[2]) if (@t >= 6);
  } elsif ($sec eq "gto") {
    if (@t == 2 && $t[0] =~ /^\d+$/ && $t[1] =~ /^\d+$/) {
      $curAtom = $t[0] - 1;
      $atomShells[$curAtom] = [];
    } elsif (@t == 3 && defined($NF{$t[0]})) {
      my @prims;
      for my $k (1 .. $t[1]) {
        my $p = $lines[$i + $k];
        $p =~ s/D([-+]\d)/E$1/g;
        my @q = split(' ', $p);
        push(@prims, [$q[0], $q[1]]);
      }
      push(@{$atomShells[$curAtom]}, [$t[0], [@prims]]);
      $i += $t[1];
    } elsif (@t == 3) {
      exit(0);           # f or higher: not in any GFN basis; refuse, don't guess
    }
  } elsif ($sec eq "mo") {
    if ($ln =~ /^\s*Spin=\s*(\w+)/i) { $mo[-1]{spin} = lc($1); }
    elsif ($ln =~ /^\s*Ene=\s*(\S+)/) { push(@mo, { ene => $1, occ => 0, spin => "alpha", c => [] }); }
    elsif ($ln =~ /^\s*Occup=\s*(\S+)/) { $mo[-1]{occ} = $1; }
    elsif (@t == 2 && $t[0] =~ /^\d+$/ && @mo) { push(@{$mo[-1]{c}}, $t[1]); }
  }
}
exit(0) if (!@mo || !@atomZ);

# Column permutation: molden order -> canonical (stable by l within an atom),
# (no value scaling: see above).
my (@perm, @scale);
my $col = 0;
for my $at (0 .. $#atomShells) {
  my @sh;
  foreach my $s (@{$atomShells[$at]}) {
    my @cols = ($col .. $col + $NF{$s->[0]} - 1);
    $col += $NF{$s->[0]};
    push(@sh, [$LV{$s->[0]}, \@cols, $s->[0]]);
  }
  foreach my $s (sort { $a->[0] <=> $b->[0] } @sh) {   # perl's sort is stable
    my $k = 0;
    foreach my $c (@{$s->[1]}) {
      push(@perm, $c);
      push(@scale, 1.0);
      $k++;
    }
  }
}
my $nbas = scalar(@perm);
foreach my $m (@mo) { exit(0) if (scalar(@{$m->{c}}) != $nbas); }

sub emit {
  my ($moKey, $engKey, $occKey, $spin) = @_;
  my @m = grep { $_->{spin} eq $spin } @mo;
  my $n = scalar(@m);
  return 0 if ($n == 0);
  print "key: $engKey\nsize:\n$n\nvalues:\n";
  print join(" ", map { sprintf("%.8f", $_->{ene}) } @m), "\nunits:\nHartree\nEND\n";
  print "key: $occKey\nsize:\n$n\nvalues:\n";
  print join(" ", map { sprintf("%.6f", $_->{occ}) } @m), "\nunits:\nelectrons\nEND\n";
  print "key: $moKey\nsize:\n$n $nbas\nrowlabels:\n";
  for my $i (1 .. $n) { print "MO-$i "; print "\n" if ($i % 8 == 0); }
  print "\ncolumnlabels:\n";
  for my $b (1 .. $nbas) { print "BasFun-$b "; print "\n" if ($b % 6 == 0); }
  print "\nvalues:\n";
  foreach my $x (@m) {
    print join(" ", map { sprintf("%.10f", $x->{c}[$perm[$_]] * $scale[$_]) } (0 .. $nbas - 1)), "\n";
  }
  print "END\n";
  return 1;
}

emit("MO", "ORBENG", "ORBOCC", "alpha") or exit(0);
emit("MOBETA", "ORBENGBETA", "ORBOCCBETA", "beta");
print "key: MOAOORDER\nsize:\n1\nvalues:\nangular-momentum\nEND\n";

# One row per primitive: Z, l, exponent, coefficient (for the unnormalised
# primitive), shell number within the element.  An element has one basis.
my (%seen, @rows);
for my $at (0 .. $#atomShells) {
  next if ($seen{$atomZ[$at]}++);
  my $sn = 0;
  foreach my $s (sort { $LV{$a->[0]} <=> $LV{$b->[0]} } @{$atomShells[$at]}) {
    $sn++;
    foreach my $p (@{$s->[1]}) {
      push(@rows, sprintf("%d %d %.12g %.12g %d", $atomZ[$at], $LV{$s->[0]}, $p->[0], $p->[1], $sn));
    }
  }
}
my $nr = scalar(@rows);
print "key: GTOBASIS\nsize:\n$nr 5\nrowlabels:\n";
for my $i (1 .. $nr) { print "Primitive-$i "; print "\n" if ($i % 8 == 0); }
print "\ncolumnlabels:\nZ l Exponent Coefficient Shell\nvalues:\n";
print "$_\n" foreach (@rows);
print "END\n";
exit(0);
