# Writes the basis set ECCE holds for a calculation as ecce-qm input: a
# "basis <name>" line and a basis_data block with every shell of every
# element, so a basis edited in the Basis Set Tool is honoured and the
# engine needs no library of its own at run time.  Format: src/qm/input.hpp.
#
# Input is the structure readStandardBS returns (rdStandardGBS.pm).

sub writeECCEQM {
  my $bsPtr = $_[0];
  my %bs = %{$bsPtr};
  my %gbs = %{$bs{"gbs"}};
  my %name_gbs = exists($bs{"name_gbs"}) ? %{$bs{"name_gbs"}} : ();
  delete $gbs{"coordinants"};
  delete $gbs{"polarization"};
  delete $name_gbs{"coordinants"};

  if (exists $bs{"ecp"} && scalar(keys %{$bs{"ecp"}}) > 0) {
    my @els = grep { $_ ne "polarization" } sort keys %{$bs{"ecp"}};
    if (@els) {
      die "ECCE-QM has no effective core potentials; the basis set needs one for: " .
          join(" ", @els) . ". Choose a basis set that covers every element without one.\n";
    }
  }

  # The name written is only a label for the output; one library name when
  # every element has the same, otherwise "custom".
  my $label = "";
  foreach my $atom (sort keys %name_gbs) {
    my $n = $name_gbs{$atom};
    $n =~ s/^\"//;
    $n =~ s/\"$//;
    if ($label eq "") { $label = $n; }
    elsif ($label ne $n) { $label = "custom"; last; }
  }
  $label = "custom" if ($label eq "");
  print "basis $label\n";
  print "basis_data\n";

  foreach my $atom (sort keys %gbs) {
    my $el = $atom;
    if ($el !~ /^[A-Za-z]{1,2}$/) {
      die "ECCE-QM takes one basis set per element; '$atom' is a per-atom basis set.\n";
    }
    $el = ucfirst(lc($el));
    foreach my $orbPtr (@{$gbs{$atom}}) {
      my @orb = @{$orbPtr};
      my $type = uc($orb[0]);
      my @set = @{$orb[1]};
      if ($type eq "SP") {
        &writeECCEQMShell($el, "S", \@set, 0);
        &writeECCEQMShell($el, "P", \@set, 1);
      } else {
        my $ncols = 1;
        foreach my $row (@set) {
          my $n = scalar(@{$row}) - 1;
          $ncols = $n if ($n > $ncols);
        }
        for (my $col = 0; $col < $ncols; $col++) {
          &writeECCEQMShell($el, $type, \@set, $col);
        }
      }
    }
  }
  print "end\n";
}

sub writeECCEQMShell {
  my ($el, $type, $setRef, $col) = @_;
  my @rows;
  foreach my $row (@{$setRef}) {
    my ($exponent, @coefs) = @{$row};
    my $c = $coefs[$col];
    next if (!defined($c) || $c == 0);
    push(@rows, [$exponent, $c]);
  }
  return if (!@rows);
  print "shell $el $type\n";
  foreach my $r (@rows) {
    printf "  %.10g %.10g\n", $r->[0], $r->[1];
  }
}

1;
