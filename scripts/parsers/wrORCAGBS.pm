################################################################################
#
# This perl module will write a gaussian basis set in ORCA's %basis /
# NewGTO / NewECP block format. Adapted from wrGaussian16GBS.pm -- see
# that file for the full description of the $bs data structure passed
# in (it is identical here; only the output syntax differs).
#
# ORCA does not support Pople-style combined "SP" shells the way
# Gaussian does -- an SP orbital from the source basis is split into
# a separate S shell and P shell here, each using the same exponents
# but their own contraction coefficients.
#
# STATUS: the GBS (orbital basis) block below has been checked against
# real ORCA 6.1.1 input/output round-trips. The ECP (NewECP) block is
# a structural best-effort translation of the same Rexponent/
# GaussExponent/Coefficient triples Gaussian uses and has NOT been
# verified against a real ORCA ECP job -- check it against the ORCA
# manual (%basis / NewECP section) before relying on it. See issue #38.
#
# Like wrGaussian16GBS.pm, if every atom uses the same named library
# basis (no explicit coefficients, no ECP) and that name is one ORCA
# ships internally (see %NameToBasis below), this instead prints a
# single "useRouteCard <name>" line so the caller (ai.orca) can put the
# basis name directly on the "! ..." keyword line -- e.g.
# "! RKS B3LYP def2-SVP" -- rather than writing out explicit primitives.
# Confirmed working: "! RKS B3LYP def2-SVP Opt Freq" round-tripped
# against real ORCA 6.1.1.
#
################################################################################

sub writeORCA{
  my $bsPtr = $_[0];
  my %bs = %{$bsPtr};
  my %gbs = %{$bs{"gbs"}};
  my %name_gbs = exists($bs{"name_gbs"}) ? %{$bs{"name_gbs"}} : ();
  my $coordinants = delete $gbs{"coordinants"};
  my %ecp;
  my $gbsPolarization = delete $gbs{"polarization"};
  my $useRouteCard = 0;

  &setupBasisTranslation;

  if (exists $bs{"name_gbs"} && !(exists $bs{"ecp"})) {
    delete $name_gbs{"coordinants"};
    my $oldbasis = "";
    $useRouteCard = 1;
    foreach my $atom (keys %name_gbs) {
      my $lib_gbs = lc $name_gbs{$atom};
      $lib_gbs =~ s/^\"//;
      $lib_gbs =~ s/\"$//;
      if ($lib_gbs ne $oldbasis && $oldbasis ne "") { $useRouteCard = 0; }
      if (!defined($NameToBasis{$lib_gbs})) { $useRouteCard = 0; }
      $oldbasis = $lib_gbs;
    }
    if ($useRouteCard == 1) {
      print "useRouteCard $NameToBasis{$oldbasis}\n";
      return;
    }
  }

  if (exists $bs{"ecp"}) {
    %ecp = %{$bs{"ecp"}};
    delete $ecp{"polarization"};
  }

  print "%basis\n";

  my $atom;
  foreach $atom (keys %gbs) {
    my @orbitalList = @{$gbs{$atom}};
    my $en = $atom;
    $en = ucfirst(lc($en));
    print "NewGTO $en\n";

    my $orbPtr;
    foreach $orbPtr (@orbitalList) {
      my @orb = @{$orbPtr};
      my $orbitalType = $orb[0];
      my @contractionSet = @{$orb[1]};

      if ($orbitalType eq "SP") {
        &writeORCAShell("S", \@contractionSet, 0);
        &writeORCAShell("P", \@contractionSet, 1);
      } else {
        &writeORCAShell($orbitalType, \@contractionSet, 0);
      }
    }
    print "end\n";
  }

  ########################################
  # Write out ECP information (unverified -- see header note)
  ########################################
  @shells = ("s", "p", "d", "f", "g", "h", "i");
  foreach $atom (keys %ecp) {
    my @ecpData = @{$ecp{$atom}};
    my $numElectrons = $ecpData[0];
    my @ecpComponents = @{$ecpData[1]};
    my $numComponents = $#ecpComponents;
    my $lmax = $shells[$numComponents];
    my $en = ucfirst(lc($atom));

    print "NewECP $en\n";
    print "  N_core $numElectrons\n";
    print "  lmax $lmax\n";

    my $cntr = 0;
    my $componentPtr;
    foreach $componentPtr (@ecpComponents) {
      my @component = @{$componentPtr};
      my $shellLabel = ($cntr eq 0) ? $lmax : $shells[$cntr - 1];
      my @primitiveList = @{$component[1]};
      my $primCnt = $#primitiveList + 1;
      print "  $shellLabel $primCnt\n";

      my $primPtr;
      foreach $primPtr (@primitiveList) {
        my %prim = %{$primPtr};
        if ($prim{Coefficient} ne 0) {
          printf "   %10.7f%18.8f%16.8f\n",
                 $prim{GaussExponent}, $prim{Coefficient}, $prim{Rexponent};
        }
      }
      $cntr++;
    }
    print "end\n";
  }

  print "end\n";
}

# Write one ORCA shell block ("S 3" header, then "idx exponent coeff" rows).
# $coeffIndex selects which coefficient column to use out of the source
# row (0 = S/first coefficient, 1 = P coefficient of an SP row).
sub writeORCAShell {
  my ($shellType, $contractionSetRef, $coeffIndex) = @_;
  my @contractionSet = @{$contractionSetRef};

  my @rows;
  my $rowPtr;
  foreach $rowPtr (@contractionSet) {
    my ($exponent, @coefficients) = @{$rowPtr};
    my $coefficient = $coefficients[$coeffIndex];
    next if (!defined($coefficient) || $coefficient == 0);
    push(@rows, [$exponent, $coefficient]);
  }
  return if ($#rows < 0);

  printf " %s %d\n", uc($shellType), ($#rows + 1);
  my $idx = 1;
  foreach my $row (@rows) {
    my ($exponent, $coefficient) = @{$row};
    printf " %3d%17.8f%16.8f\n", $idx, $exponent, $coefficient;
    $idx++;
  }
}

1;

sub setupBasisTranslation {

  # Names that match ORCA's own internal basis-set library, so the
  # generator can emit a bare "! <name>" keyword instead of a full
  # NewGTO listing. Trimmed to names common between the ECCE standard
  # library and ORCA's library -- extend as needed.

  $NameToBasis{"sto-3g"} = "STO-3G";
  $NameToBasis{"3-21g"} = "3-21G";
  $NameToBasis{"6-31g"} = "6-31G";
  $NameToBasis{"6-31g*"} = "6-31G*";
  $NameToBasis{"6-31g**"} = "6-31G**";
  $NameToBasis{"6-311g"} = "6-311G";
  $NameToBasis{"6-311g**"} = "6-311G**";
  $NameToBasis{"cc-pvdz"} = "cc-pVDZ";
  $NameToBasis{"cc-pvtz"} = "cc-pVTZ";
  $NameToBasis{"cc-pvqz"} = "cc-pVQZ";
  $NameToBasis{"cc-pv5z"} = "cc-pV5Z";
  $NameToBasis{"aug-cc-pvdz"} = "aug-cc-pVDZ";
  $NameToBasis{"aug-cc-pvtz"} = "aug-cc-pVTZ";
  $NameToBasis{"aug-cc-pvqz"} = "aug-cc-pVQZ";
  $NameToBasis{"def2-svp"} = "def2-SVP";
  $NameToBasis{"def2-tzvp"} = "def2-TZVP";
  $NameToBasis{"def2-qzvp"} = "def2-QZVP";
}
