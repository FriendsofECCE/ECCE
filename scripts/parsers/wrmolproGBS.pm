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
 Unq => "Unnilquaduim",
 Unp => "Unnilpentium",
 Unh => "Unnihexium",
 Uns => "Unnioctium"
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
 Unq =>  104,
 Unp =>  105,
 Unh =>  106,
 Uns =>  107
);


sub writemolpro{

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
  print "basis={\n";
  foreach $atom (keys %gbs)
  {
    #
    # Each atom has a list of orbitals associated with it
    #
    my @orbitalList = @{$gbs{$atom}};
    #
    # get element name
    #
    my $en = $elementName{$atom};
    $en =~ tr/a-z/A-Z/;
    $atom =~ tr/a-z/A-Z/;
    my $tag = 0;
    print "! $en\n";

    my @shells = ("S", "P", "D", "F", "G", "H", "I", "J");
    my $shell;
    foreach $shell ( @shells ) {

      my %shell_exists;
      my @exponents;
      my $orbPtr;
      my @total_coeff;
      #
      # If the orbitalType matches the shell then
      # push the exponent onto the exponents array
      # for this shell's loop.
      #
      foreach $orbPtr ( @orbitalList ) {

        my @orb = @{$orbPtr};
        my $orbitalType = $orb[0];
        if ( $orbitalType =~ /(.?)$shell/i ) {

          $shell_exists{$shell} = "true";
          $orbitalType =~ tr/A-Z/a-z/;
          my @contractionSet = @{$orb[1]};

          my $rowPtr;
          foreach $rowPtr ( @contractionSet ) {
            my($exponent,@coefficients);
            ($exponent,@coefficients) = @{$rowPtr};
            push ( @exponents, $exponent );
          }
        }
      }
  
      my $orbPtr;
      my @total_coeff = ();
      foreach $orbPtr ( @orbitalList ) {
        my @orb = @{$orbPtr};
        my $orbitalType = $orb[0];
        my @contractions_coeff = ();
        #
        # @contractions_coeff represents all of the columns
        # that contain a column of coefficients for a particular
        # orbPtr.
        #

        if ( $orbitalType =~ /(.?)$shell/i ) {

          $orbitalType =~ tr/A-Z/a-z/;
          my @contractionSet = @{$orb[1]};

          my $num_coeff = 1;
          #
          # set num_coeff equal to 1 so that $i <= $num_coeff
          # would be true the first time through the for loop
          #
          for ( $i=0; $i <= $num_coeff; $i++ ) {
            my $rowPtr;
            my @column = ();
            foreach $rowPtr ( @contractionSet ) {
              my($exponent,@coefficients);
              ($exponent,@coefficients) = @{$rowPtr};
              if ( $shell eq "S" && $orbitalType eq "sp" )
              {
                push ( @column, $coefficients[0] );
                $num_coeff = 0;
                #
                # setting num_coeff equal to zero allows the for loop
                # to cycle through only once to pick up the first coefficient
                # if the shell is equal to "S" and the orbitalType is equal to sp
                #
              }
              elsif ( $shell eq "P" && $orbitalType eq "sp" )
              {
                push ( @column, $coefficients[1] );
                $num_coeff = 0;
                #
                # setting num_coeff equal to zero allows the for loop
                # to cycle through only once to pick up the second coefficient
                # if the shell is equal to "P" and the orbitalType is equal to sp
                #
              }
              else
              {
                # if the orbitalType is not equal to sp then the $num_coeff
                # value will be made the number of columns of coefficients
                # for the orbPtr
                push ( @column, $coefficients[$i] );
                $num_coeff = $#coefficients;
              }
            }
            #
            # this stores each orbPtr column within a @contractions_coeff array
            # and if the shell matches the orbitalTypes for multiple orbPtrs
            # then the orbPtr's columns that are recieved first are stored first
            # and printed first
            #
            push ( @contractions_coeff, \@column );
          }
        }
        #
        # @total_coeff contains all the numbers for the entire file
        #
        push ( @total_coeff, \@contractions_coeff );
      }
#######End of setting new data structure##############
#######Printing Exponents#############################

      if ($shell_exists{$shell} eq "true" ) {
        $shell =~ tr/A-Z/a-z/;
        print "$shell,$atom ";
      }
      my $exponent;
      foreach $exponent ( @exponents ) {
        $tmp = sprintf (',%14.8e', $exponent * 10);
        $tmp =~ s/e/E/g;
        my $zero = 0;
        $tmp =~ s/(-*)(\d)(\.)(\d)(\d+)/$1$zero$3$2$4$5/;
        print "$tmp";
        print "\n" if ( $exponent == $exponents[$#exponents] );
      }

#########Counting##############
      my $ctr1 = 1;
      my $ctr2 = 0;

      my $contPtr;
      foreach $contPtr ( @total_coeff ) {
        my @contractions_coeff = @{$contPtr};

        #
        # $set_ctr will set the value of 
        # $ctr1 and $ctr2 to 1 and 0 respectively
        # when a new shell is printing out.
        #
        my $set_ctr = 0;
        my $columnPtr;
        foreach $columnPtr ( @contractions_coeff ) {
          my @column = @{$columnPtr};

          if ( $set_ctr == 1 )
          {
            #
            # reset the counters
            #
            $ctr1 = 1;
            $ctr2 = 0;
          }
          $set_ctr = 1;

          my $coefficient;
          my $num_found = 0;
          foreach $coefficient ( @column ) {

            if ( $num_found != 1 && $coefficient == 0 ) {
              ##if number is not found and coefficient is 0 
              ##then increment both counters
              $ctr1++;
              $ctr2++;
            }
            elsif ( $num_found == 1 && $coefficient == 0 ) {
              ##after num 1 is found increment nothing and kill loop
              last;
            }
            elsif ( $coefficient != 0 && $num_found == 0 )
            {
              ##first num found
              $ctr2++;
              $ctr1 = $ctr2 if $#column == 0;
            }
            elsif ( $coefficient != 0 ) {
              ##increment ctr2
              $ctr2++;
            }
            $num_found = 1 if ( $coefficient != 0 );
          }

          printf "c,%d.%d", $ctr1, $ctr2;
#########Printing Coefficients####

          my $i;
          for ( $i=0; $i<=$#column; $i++ ) {
            $tmp = sprintf (',%15.8e', $column[$i] * 10);
            $tmp =~ s/e/E/g;
            my $zero = 0;
            $tmp =~ s/(-*)(\d)(\.)(\d)(\d+)/$1$zero$3$2$4$5/;
            print "$tmp" if ( $column[$i] != 0 );
            print "\n" if ( $i == $#column );
          }
        }
        $ctr1 = $ctr2 + 1 if ( $ctr2 != 0 );
      }
    }
  }
  print "}\n";
}
