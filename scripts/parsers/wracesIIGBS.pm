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

sub wracesII {
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
    # Each atom has a list of orbitals  associated with it.
    #
    my @orbitalList = @{$gbs{$atom}};
    print "EMSL BASIS SET LIBRARY\n\n";

    my @shells = ( "S", "P", "D", "F", "G", "H", "I", "J" );
    #
    # %shellctr will count the number of orbPtrs associated with a particular
    # orbitalType.
    #
    my %shellctr;
    #
    # %coeffctr will count the number of total exponents and coefficients
    # for every orbPtr associated with a particular orbitalType
    #
    my %coeffctr;
    my $sp_flag = 0;

    my $orbPtr;
    foreach $orbPtr ( @orbitalList )
    {
      my @orb = @{$orbPtr};
      my $orbitalType = $orb[0];
      if ( $orbitalType eq "SP" )
      {
        #
        # acesII DOES NOT SUPPORT SP ORBITALS SO SP ORBITALS IN
        # NWCHEM MUST BE SEPARATED INTO SEPARATE S AND P ORBITALS
        #
        $coeffctr{"S"}++;
        $coeffctr{"P"}++;
      }
      else
      {
        my @contractionSet = @{$orb[1]};
        my $rowPtr = $contractionSet[0];
        my($exponent,@coefficients);
        ($exponent,@coefficients) = @{$rowPtr};
        $coeffctr{$orbitalType} += $#coefficients + 1;
      }
      if ( $orbitalType eq "SP" )
      {
        ++$shellctr{"S"};
        ++$shellctr{"P"};
        $sp_flag = 1;
      }
      else
      {
        ++$shellctr{$orbitalType};
      }
    }
    #
    # the is_multi flag will find if any orbitalType has mutiple
    # orbPtr's associated with it
    #
    my $is_multi = 0;
    my $value;
    foreach $value ( values ( %shellctr ) )
    {
      $is_multi = 1 if ( $value > 1 );
    }
    my @keys = ( keys ( %shellctr ) );
    #
    # $numshells contains the number of different types of orbitalTypes
    #
    my $numshells = $#keys + 1;
    printf "%3d\n", $numshells;
    my $i;
    for ( $i=0; $i<$numshells; $i++ )
    {
      printf "%5d", $i;
    }
    print "\n";
    #
    # end first row of counters
    #
    my $shell;
    foreach $shell ( @shells )
    {
      #
      # if there are any SP orbitals comming from NWChem 
      #
      if ( $shellctr{$shell} > 0 && $sp_flag == 1 )
      {
        printf "%5d", $shellctr{$shell};
      }
      else
      {
        my $tot_coeffctr = 0;
        foreach $orbPtr ( @orbitalList )
        {
          my @orb = @{$orbPtr};
          my $orbitalType = $orb[0];
          my @contractionSet = @{$orb[1]};
          my $rowPtr = $contractionSet[0];
          my($exponent,@coefficients);
          ($exponent,@coefficients) = @{$rowPtr};
          #
          # if there is more than one coefficient in the first row 
          # and the orbPtr has an orbitalType of the
          # same shell that we are working with then tot_coeffctr will 
          # will be incremented the number of coefficients
          #
          if ( $orbitalType =~ /(.?)$shell/ && $#coefficients > 0 )
          {
            $tot_coeffctr += $#coefficients + 1;
          }
          elsif ( $orbitalType =~ /(.?)$shell/ && $#coefficients == 0 )
          {
            $tot_coeffctr++;
          }
        }
        printf "%5d", $tot_coeffctr if ( $tot_coeffctr > 0 );
      }
    }
    print "\n";
    #
    # end second row of counters
    #
    foreach $shell ( @shells )
    {
      my $totrows_ctr = 0;
      foreach $orbPtr ( @orbitalList )
      {
        my @orb = @{$orbPtr};
        my $orbitalType = $orb[0];
        if ( $orbitalType =~ /(.?)$shell/ )
        {
          my @contractionSet = @{$orb[1]};
          $totrows_ctr += $#contractionSet + 1;
        }
      }
      printf "%5d", $totrows_ctr if ( $totrows_ctr > 0 );
    }
    print "\n";
    #
    # end third row of counters
    #

#################end of counting####################

#################start of numbers###################
#################begin exponents#####################

    foreach $shell ( @shells )
    {
      #
      # $return_ctr counts the number of exponents that have been
      # printed and prints a return if 5 exponents have been printed
      #
      my $return_ctr = 0;
      my $orbPtr;
      #
      # localshellctr counts all orbPtrs
      #
      my $localshellctr = 1;
      print "\n" if ( $shellctr{$shell} > 0 );
      foreach $orbPtr ( @orbitalList )
      {
        my @orb = @{$orbPtr};
        my $orbitalType = $orb[0];
        if ( $orbitalType =~ /(.?)$shell/ )
        {
          my @contractionSet = @{$orb[1]};
          my $rowPtr;
          foreach $rowPtr ( @contractionSet )
          {
            my($exponent,@coefficients);
            ($exponent,@coefficients) = @{$rowPtr};
            printf "%14.6f", $exponent;
            $return_ctr++;

            if ( $return_ctr == 5 )
            {
              print "\n";
              $return_ctr = 0;
            }
          }
          #
          # if the number of matched orbitalTypes (localshellctr) 
          # equals the total number of orbPtrs that match that shell
          #
          # print one return if a return has already been printed because the 
          # return_ctr equaled 5 above, print two returns if not.
          #
          print "\n\n" if ( $localshellctr == $shellctr{$shell} && $return_ctr != 0 );
          print "\n" if ( $localshellctr == $shellctr{$shell} && $return_ctr == 0 );
          $localshellctr++;
        }
      }

##############end exponents########################
##############begin coefficients###################

      #
      # this $counter counts the number of orbPtrs with an orbitalType that
      # matches the current shell.  If there are multiple orbPtrs with matching
      # orbitalTypes or there is an SP orbital, then zeros will be intermixed
      # with the data.  The counter increments itself after an orbPtr has been
      # printed out.  Then before the rows of data for the next matching orbPtr
      # are printed there will a column of zeros because within the for loop 
      # below, the $counter is checked to see if it matches $i ( the current
      # column )
      #
      my $counter = 0;
      foreach $orbPtr ( @orbitalList )
      {
        my @orb = @{$orbPtr};
        my $orbitalType = $orb[0];
        if ( $orbitalType =~ /$shell/ )
        {
          my @contractionSet = @{$orb[1]};
          my $rowPtr;
          my $num_coeff;
          foreach $rowPtr ( @contractionSet )
          {
            my($exponent,@coefficients);
            ($exponent,@coefficients) = @{$rowPtr};
            $num_coeff = $#coefficients + 1;

            if ( $is_multi == 1 || $sp_flag == 1 )
            {
              #
              # depending on the basisset there may or may not be zeros intermixed
              # with the actual data.  If there are zeros then $shellctr{$shell} will
              # deturmine the number of columns of data.  If there are no zeros then
              # $coeffctr{$shell} will deturmine the number of columns.
              #
              # $i is the column
              #
              my $i;
              for ( $i=0; $i < $coeffctr{$shell} || $i < $shellctr{$shell}; $i++ )
              {
                if ( $counter == $i && $#coefficients == 0 )
                {
                  printf "%11.8f", $coefficients[0];
                }
                elsif ( $counter == $i && $shell eq "S" && $orbitalType eq "SP" )
                {
                  printf "%11.8f", $coefficients[0];
                }
                elsif ( $counter == $i && $shell eq "P" && $orbitalType eq "SP" )
                {
                  printf "%11.8f", $coefficients[1];
                }
                elsif ( $counter == $i && $#coefficients > 0 )
                {
                  foreach $coefficient ( @coefficients )
                  {
                    printf "%11.8f", $coefficient;
                    $i++;
                  }
                  $i--;
                }
#                elsif ( $is_multi == 1 && $coefficients[$i] != 0 && $#coefficients > 0 )
#                {
#                  printf "%11.8f", $coefficients[$i];
#                }
                else
                {
                  printf "%11.8f", 0;
                }
              }
              print "\n";
            }
            else
            {
              #
              # within this else statement basissets with rows that contain
              # more than 1 coefficient and do not contain SP orbitals.
              #
              my $return_ctr = 0;
              my $coefficient;
              foreach $coefficient ( @coefficients )
              {
                printf "%11.8f", $coefficient;
                $return_ctr++;
                if ( $return_ctr == 7 )
                {
                  print "\n";
                  $return_ctr = 0;
                }
                else
                {
                  print " ";
                }
              }
              print "\n" if ( $return_ctr != 0 );
            }
          }
          #
          # this if statement accounts for when there are more than one
          # coefficients and more than one orbPtrs
          #
          # if this condition arises then after the coefficients are printed,
          # the counter is incremented by the number of coefficients instead 
          # of one and then there will be
          # that number of columns of zeros before the next orbPtr's data
          #
          if ( $coeffctr{$shell} > 1 && $orbitalType ne "SP" )
          {
            $counter += $num_coeff;
          }
          else
          {
            $counter++;
          }
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
    print "*\n";
    print "$atom:ECP-$numElectrons\n";
    print "*\n";
    print "    NCORE =  $numElectrons    LMAX =$numComponents\n";

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
      my @primitiveList = @{$component[1]};
      print "$shellType\n";
      my $primPtr;
      foreach $primPtr (@primitiveList)
      {
        #
        # Each primitive has three elements in it:
        #  an Rexponent, a Gauss exponent, and a P coefficient
        #
        my %prim = %{$primPtr};
        printf ("%14.8f", $$primPtr{Coefficient});
        printf ( "%5d", $$primPtr{Rexponent} );
        printf ("%14.8f\n", $$primPtr{GaussExponent});
      }
      $cntr++;
    }
  }
  print "*\n";
}
