################################################################################
#
# The output will be written to standard output
#
# The data is passed in as $bs
#    bs  : [ \%gbs, \%ecp ]
#           a bs is a pointer to a hash array:
#               KEY             VALUE
#               "gbs"           [%gbs]    - a pointer to a hash containing a gbs
#               "ecp"           [%ecp]    - a pointer to a hash containing a ecp
#               "dftCDfitting"  [%gbs]    - a pointer to a hash containing a gbs
#               "dftXCfitting"  [%gbs]    - a pointer to a hash containing a gbs
#     %gbs  :  {  $centerName => \@orbitalList }
#              - a gbs is a hash of centers and pointers to arrays
#              - there is one key named "polarization", it either
#                has the value of "spherical" or "cartesian".
#      $centerName : number of the center or the atomic symbol
#      @orbitalList : ( [ $orbitalType , \@GBSprimitiveList ] )
#              - orbitalList is an array of pointers to arrays, each of
#                which is one orbitalType and one pointer to an array
#        $orbitalType : S, P, SP, D...
#        @GBSprimitiveList : ( { exponent     => $exponentValue,
#                                Scoefficient => $ScoefficientValue,
#                                Pcoefficient => $PcoefficientValue} )
#              - GBSprimitiveList is an array of pointers to hashes.
#                each hash contains three key/value pairs.
#     %ecp  :  {  $centerName => \@ecpData }
#              - an ecp is a hash of centers and pointers to arrays
#      @ecpData  :  ( $numberElectrons, \@components )
#              - ecpData is an array containing the number of electrons
#                replaced by the function and a pointer to an array of
#                components.
#        @components  :  ( $shellType , \@ECPprimitiveList )
#              - components is an array containing a shell type and
#                a pointer to an array of primitives
#        @ECPprimitiveList : ( { Rexponent     => $RexponentValue,
#                                GaussExponent => $GaussExponentValue,
#                                Coefficient =>   $CoefficientValue} )
#      $shellType : ul, S, P, SP, D...
#
################################################################################

my %elementName =
(
 H  =>  "Hydrogen",
 He =>  "Helium",
 Li =>  "Lithium",
 Be =>  "Beryllium",
 B  =>  "Boron",
 C  =>  "Carbon",
 N  =>  "Nitrogen",
 O  =>  "Oxygen",
 F  =>  "Flourine",
 Ne =>  "Neon",
 Na =>  "Sodium",
 Mg =>  "Magnesium",
 Al =>  "Aluminum",
 Si =>  "Silicon",
 P  =>  "Phosphorus",
 S  =>  "Sulfur",
 Cl =>  "Chlorine",
 Ar =>  "Argon",
 K  =>  "Potassium",
 Ca =>  "Calcium",
 Sc =>  "Scandium",
 Ti =>  "Titanium",
 V  =>  "Vanadium",
 Cr =>  "Chromium",
 Mn =>  "Manganese",
 Fe =>  "Iron",
 Co =>  "Cobalt",
 Ni =>  "Nickel",
 Cu =>  "Copper",
 Zn =>  "Zinc",
 Ga =>  "Gallium",
 Ge =>  "Germanium",
 As =>  "Arsenic",
 Se =>  "Selenium",
 Br =>  "Bromine",
 Kr =>  "Krypton",
 Rb =>  "Rubidium",
 Sr =>  "Strontium",
 Y  =>  "Yttrium",
 Zr =>  "Zirconium",
 Nb =>  "Nioblum",
 Mo =>  "MolyBdenum",
 Tc =>  "Technetium",
 Ru =>  "Ruthenium",
 Rh =>  "Rhodium",
 Pd =>  "Palladium",
 Ag =>  "Silver",
 Cd =>  "Cadmium",
 In =>  "Indium",
 Sn =>  "Tin",
 Sb =>  "Antimony",
 Te =>  "Tellurium",
 I  =>  "Iodine",
 Xe =>  "Xenon",
 Cs =>  "Cesium",
 Ba =>  "Barium",
 La =>  "Lanthanum",
 Ce =>  "Cerium",
 Pr =>  "Praseodymium",
 Nd =>  "Neodymium",
 Pm =>  "Promethium",
 Sm =>  "Samarium",
 Eu =>  "Europium",
 Gd =>  "Gadolinium",
 Tb =>  "Terbium",
 Dy =>  "Dysprosium",
 Ho =>  "Holmium",
 Er =>  "Erbium",
 Tm =>  "Thulium",
 Yb =>  "Ytterbium",
 Lu =>  "Lutetium",
 Hf =>  "Hafnium",
 Ta =>  "Tantalum",
 W  =>  "Tungsten",
 Re =>  "Rhenium",
 Os =>  "Osmium",
 Ir =>  "Iridium",
 Pt =>  "Platinum",
 Au =>  "Gold",
 Hg =>  "Mercury",
 Tl =>  "Thallium",
 Pb =>  "Lead",
 Bi =>  "Bismuth",
 Po =>  "Polonium",
 At =>  "Astatine",
 Rn =>  "Radon",
 Fr =>  "Francium",
 Ra =>  "Radium",
 Ac =>  "Actinium",
 Th =>  "Thorium",
 Pa =>  "Protactinium",
 U  =>  "Uranium",
 Np =>  "Neptunium",
 Pu =>  "Plutonium",
 Am =>  "Americium",
 Cm =>  "Curium",
 Bk =>  "Berkelium",
 Cf =>  "Californium",
 Es =>  "Einsteinium",
 Fm =>  "Fermium",
 Md =>  "Mendelevium",
 No =>  "Nobelium",
 Lr =>  "Lawrencium",
 Rf =>  "Rutherfordium",
 Db =>  "Dubnium",
 Sg =>  "Seaborgium",
 Bh =>  "Bohrium",
 Hs =>  "Hassium",
 Mt =>  "Meitnerium"
);

my %elementNum =
(
 H  =>  1,
 He =>  2,
 Li =>  3,
 Be =>  4,
 B  =>  5,
 C  =>  6,
 N  =>  7,
 O  =>  8,
 F  =>  9,
 Ne =>  10,
 Na =>  11,
 Mg =>  12,
 Al =>  13,
 Si =>  14,
 P  =>  15,
 S  =>  16,
 Cl =>  17,
 Ar =>  18,
 K  =>  19,
 Ca =>  20,
 Sc =>  21,
 Ti =>  22,
 V  =>  23,
 Cr =>  24,
 Mn =>  25,
 Fe =>  26,
 Co =>  27,
 Ni =>  28,
 Cu =>  29,
 Zn =>  30,
 Ga =>  31,
 Ge =>  32,
 As =>  33,
 Se =>  34,
 Br =>  35,
 Kr =>  36,
 Rb =>  37,
 Sr =>  38,
 Y  =>  39,
 Zr =>  40,
 Nb =>  41,
 Mo =>  42,
 Tc =>  43,
 Ru =>  44,
 Rh =>  45,
 Pd =>  46,
 Ag =>  47,
 Cd =>  48,
 In =>  49,
 Sn =>  50,
 Sb =>  51,
 Te =>  52,
 I  =>  53,
 Xe =>  54,
 Cs =>  55,
 Ba =>  56,
 La =>  57,
 Ce =>  58,
 Pr =>  59,
 Nd =>  60,
 Pm =>  61,
 Sm =>  62,
 Eu =>  63,
 Gd =>  64,
 Tb =>  65,
 Dy =>  66,
 Ho =>  67,
 Er =>  68,
 Tm =>  69,
 Yb =>  70,
 Lu =>  71,
 Hf =>  72,
 Ta =>  73,
 W  =>  74,
 Re =>  75,
 Os =>  76,
 Ir =>  77,
 Pt =>  78,
 Au =>  79,
 Hg =>  80,
 Tl =>  81,
 Pb =>  82,
 Bi =>  83,
 Po =>  84,
 At =>  85,
 Rn =>  86,
 Fr =>  87,
 Ra =>  88,
 Ac =>  89,
 Th =>  90,
 Pa =>  91,
 U  =>  92,
 Np =>  93,
 Pu =>  94,
 Am =>  95,
 Cm =>  96,
 Bk =>  97,
 Cf =>  98,
 Es =>  99,
 Fm =>  100,
 Md =>  101,
 No =>  102,
 Lr =>  103,
 Rf =>  104,
 Db =>  105,
 Sg =>  106,
 Bh =>  107,
 Hs =>  108,
 Mt =>  109
);

sub wrAmicaGBS{

  my $bsPtr = $_[0];
  my %bs = %{$bsPtr};
  my %gbs = %{$bs{"gbs"}};
  my $coordinants = delete $gbs{"coordinants"};
  my %ecp;
  my $gbsPolarization = delete $gbs{"polarization"};
  my $ecpPolarization;
  my $dftXCPolarization;
  my $dftCDPolarization;
  my $tmpbasis = "";

  if (exists $bs{"ecp"})
  {
    %ecp = %{$bs{"ecp"}};
    $ecpPolarization = delete $ecp{"polarization"};
  }
  my %dftXC;
  if (exists $bs{"dftXCfitting"})
  {
    %dftXC = %{$bs{"dftXCfitting"}};
    $dftXCPolarization = delete $dftXC{"polarization"};
  }
  my %dftCD;
  if (exists $bs{"dftCDfitting"})
  {
    %dftCD = %{$bs{"dftCDfitting"}};
    $dftCDPolarization = delete $dftCD{"polarization"};
  }

  ########################################33
  # Write out GBS information
  ########################################33
  #
  # Write out information for each atom in the gbs
  #
  foreach $atom (keys %gbs)
  {
    #
    # Each atom has a list of orbitals  associated with it.
    #
    my @orbitalList = @{$gbs{$atom}};
    my $en;
    $en = $elementName{$atom};
    $en =~ tr/a-z/A-Z/;
    $tmpbasis .= "\$$en\n";

    # Read in exponents and contraction coefficients for each
    # shell and store them in arrays for later printing. This
    # could probably be cleaned up by putting all this stuff
    # into a series of higher level hash tables, but for the
    # moment, it works.

    # Pointers to exponent and contraction array
    my @Sshell=(), @Pshell=(), @Dshell=(), @Fshell=();
    my @Gshell=(), @Hshell=(), @Ishell=(), @Kshell=();

    # Total number of exponents associated with each shell;
    my $Sptr=0, $Pptr=0, $Dptr=0, $Fptr=0;
    my $Gptr=0, $Hptr=0, $Iptr=0, $Kptr=0;

    # total number of contraction coefficients associated with each shell
    my $Sccntr=0, $Pccntr=0, $Dccntr=0, $Fccntr=0;
    my $Gccntr=0, $Hccntr=0, $Iccntr=0, $Kccntr=0;

    # total number of orbitals associated with each shell
    my $Socntr=0, $Pocntr=0, $Docntr=0, $Focntr=0;
    my $Gocntr=0, $Hocntr=0, $Iocntr=0, $Kocntr=0;

    # Exponents and contraction coefficents associated with each shell
    my @Scntrct=(), @Pcntrct=(), @Dcntrct=(), @Fcntrct=();
    my @Gcntrct=(), @Hcntrct=(), @Icntrct=(), @Kcntrct=();

    my @shells = ("S","P","D","F","G","H","I","K");
    my $numCntrct;
    # sort all exponents and contractions into shells
    foreach $orbPtr (@orbitalList) {
      my @orb = @{$orbPtr};
      my $orbitalType = $orb[0];
      my @contractionSet = @{$orb[1]};
      if ($orbitalType =~ /SP/) {
        $Sshell[$Socntr] = $Sptr;
        $Pshell[$Pocntr] = $Pptr;
        $Socntr++;
        $Pocntr++;
        $length = @contractionSet;
        foreach $rowPtr ( @contractionSet) {
          my @row = @{$rowPtr};
          push(@Scntrct,[$row[0],$row[1]]);
          push(@Pcntrct,[$row[0],$row[2]]);
          $Sptr++;
          $Pptr++;
        }
        $Sccntr++;
        $Pccntr++;
      } elsif ($orbitalType =~ /S/) {
        # Note: $#row is index of last element in @row
        $Sshell[$Socntr] = $Sptr;
        $Socntr++;
        $length = @contractionSet;
        foreach $rowPtr ( @contractionSet) {
          my @row = @{$rowPtr};
          push(@Scntrct,[@row]);
          $numCntrct = $#row;
          $Sptr++;
        }
        $Sccntr = $Sccntr + $numCntrct;
      } elsif ($orbitalType =~ /P/) {
        $Pshell[$Pocntr] = $Pptr;
        $Pocntr++;
        $length = @contractionSet;
        foreach $rowPtr ( @contractionSet) {
          my @row = @{$rowPtr};
          push(@Pcntrct,[@row]);
          $numCntrct = $#row;
          $Pptr++;
        }
        $Pccntr = $Pccntr + $numCntrct;
      } elsif ($orbitalType =~ /D/) {
        $Dshell[$Docntr] = $Dptr;
        $Docntr++;
        foreach $rowPtr ( @contractionSet) {
          my @row = @{$rowPtr};
          push(@Dcntrct,[@row]);
          $numCntrct = $#row;
          $Dptr++;
        }
        $Dccntr = $Dccntr + $numCntrct;
      } elsif ($orbitalType =~ /F/) {
        $Fshell[$Focntr] = $Fptr;
        $Focntr++;
        foreach $rowPtr ( @contractionSet) {
          my @row = @{$rowPtr};
          push(@Fcntrct,[@row]);
          $numCntrct = $#row;
          $Fptr++;
        }
        $Fccntr = $Fccntr + $numCntrct;
      } elsif ($orbitalType =~ /G/) {
        $Gshell[$Gocntr] = $Gptr;
        $Gocntr++;
        foreach $rowPtr ( @contractionSet) {
          my @row = @{$rowPtr};
          push(@Gcntrct,[@row]);
          $numCntrct = $#row;
          $Gptr++;
        }
        $Gccntr = $Gccntr + $numCntrct;
      } elsif ($orbitalType =~ /H/) {
        $Hshell[$Hocntr] = $Hptr;
        $Hocntr++;
        foreach $rowPtr ( @contractionSet) {
          my @row = @{$rowPtr};
          push(@Hcntrct,[@row]);
          $numCntrct = $#row;
          $Hptr++;
        }
        $Hccntr = $Hccntr + $numCntrct;
      } elsif ($orbitalType =~ /I/) {
        $Ishell[$Iocntr] = $Iptr;
        $Iocntr++;
        foreach $rowPtr ( @contractionSet) {
          my @row = @{$rowPtr};
          push(@Icntrct,[@row]);
          $numCntrct = $#row;
          $Iptr++;
        }
        $Iccntr = $Iccntr + $numCntrct;
      } elsif ($orbitalType =~ /K/) {
        $Kshell[$Kocntr] = $Kptr;
        $Kocntr++;
        foreach $rowPtr ( @contractionSet) {
          my @row = @{$rowPtr};
          push(@Kcntrct,[@row]);
          $numCntrct = $#row;
          $Kptr++;
        }
        $Kccntr = $Kccntr + $numCntrct;
      }
    }
    $Sshell[$Socntr] = $Sptr;
    $Pshell[$Pocntr] = $Pptr;
    $Dshell[$Docntr] = $Dptr;
    $Fshell[$Focntr] = $Fptr;
    $Gshell[$Gocntr] = $Gptr;
    $Hshell[$Hocntr] = $Hptr;
    $Ishell[$Iocntr] = $Iptr;
    $Kshell[$Kocntr] = $Kptr;

    # print out shells
#    foreach $shell (@shells) {
#      print "Print shell $shell\n";
#      print "Print $shell";
#      my $var=${$shell."ocntr"};
#      print "ocntr $var\n";
#      if (${$shell."ocntr"} > 0) {
#        print "\$ $shell-TYPE FUNCTIONS\n";
#        printf "%5d%5d%5d\n", ${$shell."ptr"}, ${$shell."ccntr"}, 0;
#        # scan over orbitals
#        ${$shell."ptr"} = 0;
#        for ($i=0; $i<${$shell."ocntr"}; $i++) {
#          for ($j=${$shell."shell"}[$i]; $j<${$shell."shell"}[$i+1]; $j++) {
#            @line = @{${$shell."cntrct"}[$j]};
#            $numCntrct = $#line;
#            print_exp($line[0]);
#            for ($k=0; $k<${$shell."ptr"}; $k++) {
#              print_coef(0);
#            }
#            for ($k=1; $k<=$#line; $k++) {
#              print_coef($line[$k]);
#            }
#            for ($k=${$shell."ptr"}+$numCntrct; $k<${$shell."ocntr"}; $k++) {
#              print_coef(0);
#            }
#            print "\n";
#          }
#          ${$shell."ptr"} = ${$shell."ptr"}+ $numCntrct;
#        }
#      }
#    }

# Print each shell, one at a time. Again, this could probably be cleaned
# up at some future point.

    my $Shellmax;
    if ( $Socntr > 0) {$Shellmax = 1};
    if ( $Pocntr > 0) {$Shellmax = 2};
    if ( $Docntr > 0) {$Shellmax = 3};
    if ( $Focntr > 0) {$Shellmax = 4};
    if ( $Gocntr > 0) {$Shellmax = 5};
    if ( $Hocntr > 0) {$Shellmax = 6};
    if ( $Iocntr > 0) {$Shellmax = 7};
    if ( $Kocntr > 0) {$Shellmax = 8};
    $tmpbasis .= sprintf("%5d\n",$Shellmax);
    if ($Socntr > 0) {
      $tmpbasis .= sprintf("%5d%5d%5d\n", $Sptr, 1, $Sccntr);
      # scan over orbitals
      $Sptr = 0;
      for ($i=0; $i<$Socntr; $i++) {
        for ($j=$Sshell[$i]; $j<$Sshell[$i+1]; $j++) {
          @line = @{$Scntrct[$j]};
          $numCntrct = $#line;
          $tmpbasis .= print_exp($line[0]);
          for ($k=0; $k<$Sptr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          for ($k=1; $k<=$#line; $k++) {
            $tmpbasis .= print_coef($line[$k]);
          }
          for ($k=$Sptr+$numCntrct; $k<$Sccntr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          $tmpbasis .= "\n";
        }
        $Sptr = $Sptr+ $numCntrct;
      }
    }
    if ($Pocntr > 0) {
      $tmpbasis .= sprintf("%5d%5d%5d\n", $Pptr, 2, $Pccntr);
      # scan over orbitals
      $Pptr = 0;
      for ($i=0; $i<$Pocntr; $i++) {
        for ($j=$Pshell[$i]; $j<$Pshell[$i+1]; $j++) {
          @line = @{$Pcntrct[$j]};
          $numCntrct = $#line;
          $tmpbasis .= print_exp($line[0]);
          for ($k=0; $k<$Pptr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          for ($k=1; $k<=$#line; $k++) {
            $tmpbasis .= print_coef($line[$k]);
          }
          for ($k=$Pptr+$numCntrct; $k<$Pccntr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          $tmpbasis .= "\n";
        }
        $Pptr = $Pptr+ $numCntrct;
      }
    }
    if ($Docntr > 0) {
      $tmpbasis .= sprintf("%5d%5d%5d\n", $Dptr, 3, $Dccntr);
      # scan over orbitals
      $Dptr = 0;
      for ($i=0; $i<$Docntr; $i++) {
        for ($j=$Dshell[$i]; $j<$Dshell[$i+1]; $j++) {
          @line = @{$Dcntrct[$j]};
          $numCntrct = $#line;
          $tmpbasis .= print_exp($line[0]);
          for ($k=0; $k<$Dptr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          for ($k=1; $k<=$#line; $k++) {
            $tmpbasis .= print_coef($line[$k]);
          }
          for ($k=$Dptr+$numCntrct; $k<$Dccntr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          $tmpbasis .= "\n";
        }
        $Dptr = $Dptr+ $numCntrct;
      }
    }
    if ($Focntr > 0) {
      $tmpbasis .= sprintf("%5d%5d%5d\n", $Fptr, 4, $Fccntr);
      # scan over orbitals
      $Fptr = 0;
      for ($i=0; $i<$Focntr; $i++) {
        for ($j=$Fshell[$i]; $j<$Fshell[$i+1]; $j++) {
          @line = @{$Fcntrct[$j]};
          $numCntrct = $#line;
          $tmpbasis .= print_exp($line[0]);
          for ($k=0; $k<$Fptr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          for ($k=1; $k<=$#line; $k++) {
            $tmpbasis .= print_coef($line[$k]);
          }
          for ($k=$Fptr+$numCntrct; $k<$Fccntr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          $tmpbasis .= "\n";
        }
        $Fptr = $Fptr+ $numCntrct;
      }
    }
    if ($Gocntr > 0) {
      $tmpbasis .= sprintf("%5d%5d%5d\n", $Gptr, 5, $Gccntr);
      # scan over orbitals
      $Gptr = 0;
      for ($i=0; $i<$Gocntr; $i++) {
        for ($j=$Gshell[$i]; $j<$Gshell[$i+1]; $j++) {
          @line = @{$Gcntrct[$j]};
          $numCntrct = $#line;
          $tmpbasis .= print_exp($line[0]);
          for ($k=0; $k<$Gptr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          for ($k=1; $k<=$#line; $k++) {
            $tmpbasis .= print_coef($line[$k]);
          }
          for ($k=$Gptr+$numCntrct; $k<$Gccntr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          $tmpbasis .= "\n";
        }
        $Gptr = $Gptr+ $numCntrct;
      }
    }
    if ($Hocntr > 0) {
      $tmpbasis .= sprintf("%5d%5d%5d\n", $Hptr, 6, $Hccntr);
      # scan over orbitals
      $Hptr = 0;
      for ($i=0; $i<$Hocntr; $i++) {
        for ($j=$Hshell[$i]; $j<$Hshell[$i+1]; $j++) {
          @line = @{$Hcntrct[$j]};
          $numCntrct = $#line;
          $tmpbasis .= print_exp($line[0]);
          for ($k=0; $k<$Hptr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          for ($k=1; $k<=$#line; $k++) {
            $tmpbasis .= print_coef($line[$k]);
          }
          for ($k=$Hptr+$numCntrct; $k<$Hccntr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          $tmpbasis .= "\n";
        }
        $Hptr = $Hptr+ $numCntrct;
      }
    }
    if ($Iocntr > 0) {
      $tmpbasis .= sprintf("%5d%5d%5d\n", $Iptr, 7, $Iccntr);
      # scan over orbitals
      $Iptr = 0;
      for ($i=0; $i<$Iocntr; $i++) {
        for ($j=$Ishell[$i]; $j<$Ishell[$i+1]; $j++) {
          @line = @{$Icntrct[$j]};
          $numCntrct = $#line;
          $tmpbasis .= print_exp($line[0]);
          for ($k=0; $k<$Iptr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          for ($k=1; $k<=$#line; $k++) {
            $tmpbasis .= print_coef($line[$k]);
          }
          for ($k=$Iptr+$numCntrct; $k<$Iccntr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          $tmpbasis .= "\n";
        }
        $Iptr = $Iptr+ $numCntrct;
      }
    }
    if ($Kocntr > 0) {
      $tmpbasis .= sprintf("%5d%5d%5d\n", $Kptr, 8, $Kccntr);
      # scan over orbitals
      $Kptr = 0;
      for ($i=0; $i<$Kocntr; $i++) {
        for ($j=$Kshell[$i]; $j<$Kshell[$i+1]; $j++) {
          @line = @{$Kcntrct[$j]};
          $numCntrct = $#line;
          $tmpbasis .= print_exp($line[0]);
          for ($k=0; $k<$Kptr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          for ($k=1; $k<=$#line; $k++) {
            $tmpbasis .= print_coef($line[$k]);
          }
          for ($k=$Kptr+$numCntrct; $k<$Kccntr; $k++) {
            $tmpbasis .= print_coef(0);
          }
          $tmpbasis .= "\n";
        }
        $Kptr = $Kptr+ $numCntrct;
      }
    }
    $tmpbasis .= "\n";
  }

  ########################################33
  # Write out ECP information
  ########################################33
  #
  # Set shell type translations for gaussian code
  #
  @shells = ("s", "p", "d", "f", "g", "h");
  #
  # Write out information for each atom in the ecp
  #
  
  foreach $atom (keys %ecp)
  {
    #
    # Each atom has a set of data associated with it
    #
    my @ecpData = @{$ecp{$atom}};
    my $numElectrons = $ecpData[0];
    my @ecpComponents = @{$ecpData[1]};
    my $numComponents = $#ecpComponents;
    my $firstShell = $shells[$numComponents];
    $atom =~ tr/[a-z]/[A-Z]/;
    $atom = $atom.-ECP;
    my $gen = GEN;
    $tmpbasis .= sprintf "%-7s%s %d    %d\n",$atom, $gen, $numElectrons, $numComponents;

    my $cntr = 0;
    my $componentPtr;
    foreach $componentPtr (@ecpComponents)
    {
      #
      # Each component has a type string and a list of primitives
      #
      my @component = @{$componentPtr};
      my $shellType = ($cntr eq 0) ? $firstShell
                                   : ($shells[$cntr-1] .  "-$firstShell");
      $shellType .= " potential";
      my @primitiveList = @{$component[1]};
      $primCnt = $#primitiveList + 1;
      $tmpbasis .= sprintf(" %d      ----- %-13s   -----\n",$primCnt, $shellType);
      my $primPtr;
      foreach $primPtr (@primitiveList)
      {
        #
        # Each primitive has three elements in it:
        #  an Rexponent, a Gauss exponent, and a P coefficient
        #
        my %prim = %{$primPtr};
        $tmpbasis .= sprintf ("%16.8f", $prim{Coefficient});
        $tmpbasis .= ("  $prim{Rexponent}");
        $tmpbasis .= sprintf ("%18.8f\n",$prim{GaussExponent});
      }
      $cntr++;
    }
    $tmpbasis .=  "\n";
  }
  $tmpbasis .= "\$END\n";
  return $tmpbasis;
}

sub print_exp {
  local($string);
  $_ = $_[0];
  $CharCount = tr/0-9//;
  if ( $CharCount < 16 ) {
    $string .= sprintf "%15.7f", $_;
  }
  else {
    $string .= "***************";
  }
}  
sub print_coef {
  local($string);
  $_ = $_[0];
  $CharCount = tr/0-9//;
  if ( $CharCount < 13 ) {
    if (abs($_-1.0) < 1.0e-8) {
      $string .= "  1.0       ";
    } elsif (abs($_) <1.0e-8) {
      $string .= "  0.0       ";
    } else {
      $string .= sprintf "%12.8f", $_;
    }
  }
  else {
    $string .= print "***************";
  }
}

# This is a filter that beautifies the Amica basis set listing as output by 
# Ecce. Gdanitz, Nov. 3, 2001.

sub beautify {
  my $tmpbasis = $_[0];
# Loop over elements.
  while(1) {

# Read element string and copy it to output.
    $tmpbasis =~ /(^[^\n]*\n)/;
    $line = $1;
    $tmpbasis =~ s/^[^\n]*\n//;
    print ($line);
    if($line =~ /\$END/) {return;} # Exit loop if '$END' is read.
    if ($tmpbasis eq "") {return;} # Prevent accidental infinite loops

# Read #contraction sets.
    $tmpbasis =~ /(^[^\n]*\n)/;
    $nsets = $1;
    $tmpbasis =~ s/^[^\n]*\n//;
    printf("%5d \n", $nsets);

# Loop over contraction sets.
    foreach $n (1..$nsets) {

# Read #primitives, value of (l+1), and #contracted functions.
      $tmpbasis =~ /(^[^\n]*\n)/;
      $line = $1;
      $tmpbasis =~ s/^[^\n]*\n//;
      @num = split(/\s+/,$line);
      $nprim = $num[1]; $lval  = $num[2]; $ncont = $num[3];
      printf("%5d%5d%5d \n", $nprim, $lval, $ncont);
  
      foreach $i (1 .. $nprim) {
  
# Read as many input lines as needed to get all contraction coefficients for 
# the $i-th primitive function.

        undef($line); undef($sline[$ncont+1]);
        while(! defined($sline[$ncont+1])) { 
          $tmpbasis =~ /(^[^\n]*\n)/;
          $aline = $1;
          $tmpbasis =~ s/^[^\n]*\n//;
          $line = join("", $line, $aline);
          $line = join(" ", "", $line);
          @sline = split(/\s+/, $line);
        }
        foreach $j (0 .. $ncont) {
          $cont[$j][$i] = $sline[$j+1];
        }
      }
# Sort contractions sets so that exponents are decreasing.
      sort_bas();

# Write sorted contraction set in a compact format to output.
      write_bas();

    } # End of loop over contraction sets.

# Skip blank line in input.
    $tmpbasis =~ /(^[^\n]*\n)/;
    $tmpbasis =~ s/^[^\n]*\n//;
    print "\n";

  } # End of loop over elements.
}

sub write_bas {

# Write sorted contraction set in a compact format to output.

  my($i, $j, $c, @fmt);

# Determine format for contraction sets.
  foreach $j (1..$ncont) {
    $fmt[$j]='short';
    foreach $i (1..$nprim) {
      $c = $cont[$j][$i];
      if($c != 0 && $c != 1) {$fmt[$j] = 'long';}
    }
  }

  foreach $i (1 .. $nprim) {
    printf("%15.7f", $cont[0][$i]);
    foreach $j (1 .. $ncont) {
      $c = $cont[$j][$i];
      if($fmt[$j] eq 'long') {
        if($c == 0) {
          printf("  %3.1f       ", 0.0);}
        else {
          printf("%12.8f", $c);}
    } else {
        if($j == 1) {print(" ");} # Add space.
        printf("%4.1f", $c);
      }
    }
    print("\n");
  }
}

sub sort_bas {

# Sort contractions sets so that exponents are decreasing.

  my($i, $j, $k);
  foreach $i (2 .. $nprim) {
    foreach $j (1 .. $i-1) {
      if($cont[0][$i] > $cont[0][$j]) {
        foreach $k (0 .. $ncont) {
          ($cont[$k][$i], $cont[$k][$j]) = ($cont[$k][$j], $cont[$k][$i]);
        }
      }
    }
  }

  if($nprim == $ncont) {
# Copy unit matrix on @cont.
    foreach $i (1 .. $nprim) {
      foreach $j (1 .. $nprim) {
        $cont[$i][$j] = 0
      }
      $cont[$i][$i] = 1
    }
  }
}
1;
