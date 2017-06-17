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

sub wrNWChem{

  my $bsPtr = $_[0];
  my %bs = %{$bsPtr};
  my %gbs = ();
  my %ecp;
  my $coordinants;
  my $gbsPolarization;
  my $ecpPolarization;
  my $dftXCPolarization;
  my $dftCDPolarization;

# HACK!! check basis set names for library to see if they are compound
# basis set that are incorrectly merged into a single basis. If so, force
# basis to use explicit list of coefficients.
# Also, must use explicit exp & coef for the IGLO-II and IGLO-III
# basis sets.  They are not in NWChems library, yet.
  $useExplicitBasis = 0;
  if (exists($bs{"name_gbs"})) {
#
#   Print out basis using library names
#
    if (exists $bs{"name_gbs"}) {
      %gbs = %{$bs{"name_gbs"}};
    }
    my $atom;
    foreach $atom (keys %gbs)
    {
      if (!($atom =~ /coordinants/)) {
        if ($gbs{$atom} =~ /^\s*\"\s*aug/i ||
            $gbs{$atom} =~ /^\s*\"\s*d-aug/i ||
            $gbs{$atom} =~ /-pcv/i ||
            $gbs{$atom} =~ /IGLO-II/i) {
          $useExplicitBasis = 1;
        }
      }
    }
  }

  if (!exists($bs{"name_gbs"}) || $useExplicitBasis == 1) {
    if (exists $bs{"ecp"})
    {
      %ecp = %{$bs{"ecp"}};
      $ecpPolarization = delete $ecp{"polarization"};
    }

    foreach $part (( "gbs", "dftXCfitting", "dftCDfitting" )) {

      if ($part eq "gbs" && exists $bs{"gbs"}) {
        %gbs = %{$bs{$part}};
        $coordinants = delete $gbs{"coordinants"};
        $dftXCPolarization = delete $gbs{"polarization"};
        print "basis \"ao basis\" $coordinants print\n";
      }
      elsif ($part eq "dftXCfitting" && exists $bs{"dftXCfitting"}) {
        %gbs = %{$bs{$part}};
        $coordinants = delete $gbs{'coordinants'};
        $dftXCPolarization = delete $gbs{"polarization"};
        print "basis \"xc basis\" $coordinants print\n";
      }
      elsif ($part eq "dftCDfitting" && exists $bs{"dftCDfitting"}) {
        %gbs = %{$bs{$part}};
        $coordinants = delete $gbs{'coordinants'};
        $dftCDPolarization = delete $gbs{"polarization"};
        print "basis \"cd basis\" $coordinants print\n";
      }
      else {
        undef %gbs;
      }

      my $atom;
      foreach $atom (keys %gbs)
      {
        my @orbitalList = @{$gbs{$atom}};
  
        my $orb;
        foreach $orbPtr (@orbitalList)
        {
          my @orb = @{$orbPtr};
          my $orbitalType = $orb[0];
          my @contractionSet = @{$orb[1]};

          print "$atom    $orbitalType\n";
          my $rowPtr;
          foreach $rowPtr (@contractionSet)
          {
            my($exponent,@coefficients);
            ($exponent,@coefficients) = @{$rowPtr};
            printf "%19s", $exponent;
            foreach $coefficient (@coefficients) {
              printf "%19s", $coefficient;
            }
            print "\n";
          }
        }
      }
      print "END\n" if(%gbs);
    }
    #####ECP#######################
    print "ECP\n" if (exists $bs{"ecp"});
    foreach $atom (keys %ecp)
    {
      #
      # Each atom has a set of data associated with it
      #
      my @ecpData = @{$ecp{$atom}};
      my $numElectrons = $ecpData[0];
      my @Components = @{$ecpData[1]};
      print "$atom nelec $numElectrons\n";
      foreach $componentPtr (@Components) {

        my @component = @{$componentPtr};
        my $shellType = $component[0];
        my @ECPprimitiveList = @{$component[1]};
        my $primPtr;
        my $once = 1;

        foreach $primPtr (@ECPprimitiveList) {
          my %prim = %{$primPtr};
          if(!($prim{'GaussExponent'} == 0) &&
            !($prim{'Coefficient'} == 0)) {
            if($once) {
              print " $atom $shellType\n";
              $once = 0;
            }
            print"    $prim{'Rexponent'}";
            printf("%20.8f",$prim{'GaussExponent'});
            printf("%20.8f",$prim{'Coefficient'});
            print "\n";
          }
        }
      }
    }
    print"END\n" if (exists $bs{"ecp"});
    #####ECP#######################
  } else {
#
#   Print out basis using library names
#
    foreach $part (( "name_gbs", "name_dftXCfitting", "name_dftCDfitting",
                     "name_ecp" )) {

      if ($part eq "name_gbs" && exists $bs{"name_gbs"}) {
        %gbs = %{$bs{$part}};
        $coordinants = delete $gbs{"coordinants"};
        print "basis \"ao basis\" $coordinants print\n";
      }
      elsif ($part eq "name_dftXCfitting" && exists $bs{"name_dftXCfitting"}) {
        %gbs = %{$bs{$part}};
        $coordinants = delete $gbs{'coordinants'};
        print "basis \"xc basis\" $coordinants print\n";
      }
      elsif ($part eq "name_dftCDfitting" && exists $bs{"name_dftCDfitting"}) {
        %gbs = %{$bs{$part}};
        $coordinants = delete $gbs{'coordinants'};
        print "basis \"cd basis\" $coordinants print\n";
      }
      elsif ($part eq "name_ecp" && exists $bs{"name_ecp"}) {
        %gbs = %{$bs{$part}};
        print "ECP\n";
      } else {
        undef %gbs;
      }

      my $atom;
      foreach $atom (keys %gbs)
      {
         print("  $atom library $gbs{$atom}\n");
      }
      print "END\n" if(%gbs);
    }
  }
}
1;
