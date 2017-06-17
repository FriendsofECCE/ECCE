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


sub writemolpro_int_lib{

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

  my $atom;

  ########################################33
  # Write out GBS information
  ########################################33
  #
  # Write out information for each atom in the gbs
  #
  foreach $atom (keys %gbs)
  {
    #
    # Each atom has a list of orbitals associated with it
    #
    my @orbitalList = @{$gbs{$atom}};
    #
    # Get element name
    #
    my $en = $elementName{$atom};
    $en =~ tr/a-z/A-Z/;
    $atom =~ tr/a-z/A-Z/;
    print "! $en\n";

    #
    # Counting different orbitalTypes
    # Increment S and P if is an SP orbital
    #
    my %shellctr = ("S", 0, "P", 0, "D", 0, "F", 0, "G", 0, "H", 0, "I", 0);
    my $orbPtr;
    foreach $orbPtr ( @orbitalList )
    {
      my @orb = @{$orbPtr};
      my $orbitalType = $orb[0];
      if ( $orbitalType eq "SP" )
      {
        $shellctr{"S"}++;
        $shellctr{"P"}++;
      }
      else
      {
        $shellctr{$orbitalType}++;
      }
    }

    my @shells = ("S", "P", "D", "F", "G", "H", "I");
    my $shell;
    foreach $shell ( @shells )
    {
      my $OT = $shell;
      $OT =~ tr/A-Z/a-z/;
      #
      # OT stands for orbitalType
      #
      my $zero = 0;
      #
      # when I want to print 0 it seems to be
      # easier to print the variable $zero instead
      # of 0.
      #
      my $to_ret = 0;
      #
      # $to_ret knows when to print after 5 exponenets
      # have been printed
      #
      my $flag = 0;
      #
      # $flag knows when to print a new orbitalType
      # to prompt for a new set of numbers
      #

      foreach $orbPtr (@orbitalList)
      {
        my @orb = @{$orbPtr};
        my $orbitalType = $orb[0];
        my @contractionSet = @{$orb[1]};
        if ( $orbitalType =~ /(.?)$shell/i ) {

          print "  $atom  $OT\n" if $flag == 0;
          $flag = 1;

          my $rowPtr;
          foreach $rowPtr ( @contractionSet )
          {
            my($exponent,@coefficients);
            ($exponent,@coefficients) = @{$rowPtr};

            my $tmp = sprintf ( '%17.9e', $exponent * 10);
            $tmp =~ s/e/E/g;
            $tmp =~ s/(-*)(\d)(\.)(\d)(\d+)/$1$zero$3$2$4$5/;
            print "$tmp";
            $to_ret++;
            if ( $to_ret == 5 )
            {
              print "\n";
              $to_ret = 0;
            }
          }
        }
      }
      my $s_ctr = 0;
      foreach $orbPtr (@orbitalList)
      {
        my @orb = @{$orbPtr};
        my $orbitalType = $orb[0];
        my @contractionSet = @{$orb[1]};
        if ( $orbitalType =~ /$shell/ ) 
        {
          $s_ctr++;
          my $rowPtr = $contractionSet[0];
          my($exponent,@coefficients);
          ($exponent,@coefficients) = @{$rowPtr};

          my $num_coeff = $#coefficients + 1;
          $num_coeff = 1 if ( $orbitalType eq "SP" );
          #
          # if the $orbitalType equals SP then only
          # allow the for loop to happen once
          #
          my $i;
          for ( $i=0; $i < ( $num_coeff ); $i++ )
          {

            foreach $rowPtr ( @contractionSet )
            {
              my($exponent,@coefficients);
              ($exponent,@coefficients) = @{$rowPtr};

              $tmp = sprintf ('%17.9e', $coefficients[$i] * 10);
              if ( $orbitalType eq "SP" && $shell eq "S" )
              {
                $tmp = sprintf ('%17.9e', $coefficients[0] * 10);
              }
              if ( $orbitalType eq "SP" && $shell eq "P" )
              {
                $tmp = sprintf ('%17.9e', $coefficients[1] * 10);
              }
              $tmp =~ s/e/E/g;
              $tmp =~ s/(-*)(\d)(\.)(\d)(\d+)/$1$zero$3$2$4$5/;
              print "$tmp" if $tmp != 0;
              #
              # return after five coefficients are printed
              #
              $to_ret++ if $tmp != 0;
              if ( $to_ret == 5 )
              {
                print "\n";
                $to_ret = 0;
              }
            }
          }
          #
          # print a return if there are no more orbPtrs that have the same
          # orbitalType as the shell and if a return
          # has not already been printed due to the $to_ret variable
          #
          print "\n" if ( $s_ctr == $shellctr{$shell} && $to_ret != 0 );
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
  my @shells = ("s", "p", "d", "f", "g", "h");
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
    printf "ECP,atom #,%d,%d;\n", $numElectrons, $numComponents;

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
      printf("%d; ! %-13s\n",$primCnt, $shellType);
      my $primPtr;
      foreach $primPtr (@primitiveList)
      {
        #
        # Each primitive has three elements in it:
        #  an Rexponent, a Gauss exponent, and a P coefficient
        #
        my %prim = %{$primPtr};
#        print  ("  $prim{Rexponent}");
        printf ("%d,%8.8f,%8.8f;\n", $prim{Rexponent}, $prim{GaussExponent}, $prim{Coefficient});
#        printf ("%16.8f\n", $prim{Coefficient});
      }
      $cntr++;
    }
  }

}
1;
