################################################################################
#
# This perl module will write a gaussian basis set in the Gaussian94 file format
# To use this module, "require" it in the calling program.
# The calling program must be running under perl5.
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

sub wrMeldef{

  my $bsPtr = $_[0];
  my %bs = %{$bsPtr};
  my %gbs = %{$bs{"gbs"}};
  my $coordinants = delete $gbs{"coordinants"};
  my %ecp;
  my $gbsPolarization = delete $gbs{"polarization"};
  my $ecpPolarization;
  my $dftXCPolarization;
  my $dftCDPolarization;

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
    #
    # Get Element Name en
    #
    my $en;
    $en = $elementName{$atom};
    $en =~ tr/a-z/A-Z/;
    print "\$ $en\n";
    $atom =~ tr/a-z/A-Z/;
    #
    # Each group of numbers is contained in an orbPtr
    #
    my $orbPtr;
    my $last_orbType;
    my $counter = 0;
    foreach $orbPtr (@orbitalList)
    {
      my @orb = @{$orbPtr};
      my $orbitalType = $orb[0];
      my @contractionSet = @{$orb[1]};
      #
      # $contCntr counts rows of NWChem for one single orbPtr
      # not all rows of NWChem numbers associated with a single orbitalType
      # that is contained within multiple orbPtrs
      #
      my $contCntr = $#contractionSet + 1;

      my $i;
      my $shell;
      #
      # $coeffctr counts the exponent and coefficients for the first row
      # of the orbPtr.  Perhaps $coeffctr should be renamed to be
      # $Exponent_and_Coefficients_ctr
      #
      my $coeffctr = $#{$contractionSet[0]};
      #
      # loop through for loop as many times as the number of numbers
      # on the first row for the orbPtr
      #
      for ($i=0; $i<$coeffctr; $i++)
      {

        #
        # Meldef DOES NOT support SP orbitals so numbers are divided into
        # separate S and P orbitals
        #
        $shell = $orbitalType;
        $shell = "S" if ($orbitalType eq "SP" && $i == 0);
        $shell = "P" if ($orbitalType eq "SP" && $i == 1);

        #
        # Will match even when:
        # orbitalType == SP and $lastshell == S (or) P
        # as well as when:
        # orbitalType == lastshell
        #
        if ( $orbitalType =~ /$lastshell/ ) {++$counter;}
        else {$counter = 1;}
        #
        # Meldef counts P orbitals in otherwise SP orbitals by making
        # the counter decrement one below the S orbital from the same
        # SP orbital.  The --$counter only makes the counter have the
        # same value as the counter for the S orbital or the otherwise 
        # SP orbital.  To make the $counter be one below the S orbital
        # the $counter_minus_one value is used.  --$counter is used
        # rather than decrementing the $counter value twice so that
        # the $counter can continue to count at the correct value
        # in the next go around in the for loop.
        #
        if ( $shell eq "P" && $orbitalType eq "SP" ){--$counter;}
        $lastshell = $shell;

        my $counter_minus_one;
        my $group_1;

        if ($orbitalType eq "SP" && $i == 1)
        {
          $counter_minus_one = $counter - 1;
          $group_1 = $atom.$counter_minus_one.$shell;
        }
        else
        {
          $group_1 = $atom.$counter.$shell;
        }
        printf "%-5s%2d %s\n", $group_1, $contCntr, $shell;

        my $row_ctr;
        my $count;
        my $rowPtr;
        $row_ctr = 1;
        foreach $rowPtr (@contractionSet)
        {
          my($exponent,@coefficients);
          ($exponent,@coefficients) = @{$rowPtr};
          $_ = $coefficients[$i];
          $count = tr/0-9//;
          if ( $count >= 16 )
          {
            #
            # This line of * accounts for the way fortran handles 
            # numbers that are too large.  (Probably unneeded...)
            #
            print "****************";
          }
          else
          {
            printf("%16s", $coefficients[$i]);
          }
          if( $rowPtr eq $contractionSet[$#contractionSet] ) 
          {
            print"\n";
          }
          elsif ( $row_ctr == 5 )
          {
            #
            # one row must only have 5 numbers before returning
            #
            $row_ctr = 0;
            print "\n";
          }
          $row_ctr++;
        }
        $row_ctr = 1;
        foreach $rowPtr (@contractionSet)
        {
          my($exponent,@coefficients);
          ($exponent,@coefficients) = @{$rowPtr};
          $_ = $exponent;
          $count = tr/0-9//;
          if ( $count >= 16 )
          {
            print "****************";
          }
          else
          {
            printf("%16s", $exponent);
          }
          #
          # if you get to the last row in the set of numbers
          #
          if($rowPtr eq $contractionSet[$#contractionSet]){print"\n";}
          elsif ($row_ctr == 5)
          {
            $row_ctr = 0;
            print "\n";
          }
          $row_ctr++;
        }
      }
    }
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
    ###on Meldef last ecpComponent is first###############
    my $d_comp = shift(@ecpComponents);
    push(@ecpComponents, $d_comp);
    ######################################################
    my $numComponents = $#ecpComponents;
    my $firstShell = $shells[$numComponents];
    $atom =~ tr/[a-z]/[A-Z]/;
    print"! EFFECTIVE CORE POTENTIAL FOR ATOM $atom\n";
    print"! NO. OF CORE ELECTRONS = $numElectrons\n";

    my $cntr = 0;
    my $componentPtr;
    foreach $componentPtr (@ecpComponents)
    {
      #
      # Each component has a type string and a list of primitives
      #
      my @component = @{$componentPtr};
      my $shellType =  ($cntr eq $numComponents) ? $firstShell : ($shells[$cntr] .  "-$firstShell");
                                  
      $shellType .= " potential";
      my @primitiveList = @{$component[1]};
      $primCnt = $#primitiveList + 1;
      print"! TYPE OF POTENTIAL = $shellType\n";
      print"! NO. OF TERMS = ! $primCnt\n";
      my $primPtr;
      foreach $primPtr (@primitiveList)
      {
        #
        # Each primitive has three elements in it:
        #  an Rexponent, a Gauss exponent, and a P coefficient
        #
        my %prim = %{$primPtr};
        print" $prim{Rexponent}";
        if($primPtr eq $primitiveList[$#primitiveList]){print"\n";}
        else{}
      }
      foreach $primPtr (@primitiveList)
      {
        my %prim = %{$primPtr};
        printf ("%16.8f",$prim{GaussExponent});
        if($primPtr eq $primitiveList[$#primitiveList]){print"\n";}
        elsif($primPtr eq $primitiveList[4]){print"\n";}
        else{}
      }
      foreach $primPtr (@primitiveList)
      {
        my %prim = %{$primPtr};
        printf ("%16.8f", $prim{Coefficient});
        if($primPtr eq $primitiveList[$#primitiveList]){print"\n";}
        elsif($primPtr eq $primitiveList[4]){print"\n";}
        else{}
      }
      $cntr++;
    }
  }
  print "\$END\n";
}
1;
