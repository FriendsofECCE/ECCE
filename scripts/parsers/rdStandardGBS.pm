################################################################################
#
# This perl module will read a "standard" format gbs file.
# To use this module, "require" it in the calling program.
# The calling program must be running under perl5.
#
# The input is always read from STDIN.
#
# The data is returned as %bs
#    bs  : { }
#           a bs is a hash. Key value pairs are:
#               KEY             VALUE
#               "gbs"           [%gbs]    - a pointer to a hash containing a gbs
#               "ecp"           [%ecp]    - a pointer to a hash containing a ecp
#               "dftCDfitting"  [%gbs]    - a pointer to a hash containing a gbs
#               "dftXCfitting"  [%gbs]    - a pointer to a hash containing a gbs
#     %gbs  :  {  $centerName => \@orbitalList }
#              - a gbs is a hash of centers and pointers to arrays
#      $centerName : number of the center or the atomic symbol
#      @orbitalList : ( [ $orbitalType , \@contractionSet ] )
#              - orbitalList is an array of pointers to arrays, each of 
#                which is one orbitalType and one pointer to an array
#        $orbitalType : S, P, SP, D...
#        @contractionSet : ( [@contractionRow],.... )
#              - contractionSet is an array of pointers to arrays
#          @contractionRow : ($exponent,$coefficient,$coefficient,....)
#              - a contractionRow is an array of scalar values. The first
#                value is the exponent, the remaining values are the
#                associated coefficients.
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
#   If appropriate, associative arrays containing the corresponding basis
#   using the names in the NWChem basis set library are also returned
#      %name_gbs  : {$centerName => $libraryName}
#      %name_ecp  : {$centerName => $libraryName}
#      %name_dftCDfitting  : {$centerName => $libraryName}
#      %name_dftXCfitting  : {$centerName => $libraryName}
################################################################################

sub readStandardBS {
$|=1;
  # Initialize variables
  my ($numPrimitives,$center,$orbital,$coordinants,
      @contractionSet,@contractionRow,@ECPprimitiveList,
      @orbitalList,@centers);
  my ($orbType,%gbs,%ecp,%dftXC,%dftCD);
  my (%name_gbs,%name_ecp,%name_dftXC,%name_dftCD);
  my $readingNameBasis=0, $readingNumericalBasis=0;
  $echo = "";

  # Read and process the file
  while (<STDIN>)
  {
    $echo .= $_;
    # Figure out whether data is being read from NameBasis or
    # NumericalBasis part of input file

    if (/^\s*NameBasis/) { $readingNameBasis = 1; }
    if (/^\s*EndNameBasis/) { $readingNameBasis = 0; }
    if (/^\s*NumericalBasis/) { $readingNumericalBasis = 1; }
    if (/^\s*EndNumericalBasis/) { $readingNumericalBasis = 0; }

    # Trim off comments starting with "#"
    s/#.*$//;

    # Trim off comments starting with "!"
    s/!.*$//;

    # Trim off trailing spaces
    s/\s+$//;
    next if (!$_);

    # Parse explicit list of coefficients and exponents
    if ($readingNameBasis == 0 && $readingNumericalBasis == 1) {
      if (/^basis \"(\w\w) basis\" (\w+) print/)
      {
        $coordinants = $2;
        if ($1 eq "ao")
        {
          $orbType = "orbital";
        }
        elsif ($1 eq "xc")
        {
          $orbType = "exchange";
        }
        elsif ($1 eq "cd")
        {
          $orbType = "charge";
        }
      }

      # Read line containing the Atomic Symbol and orbital
      if ( (/^(\w+)\s+(\w+)$/) &&
           ($orbType ne "ecp") )
      {
        #
        # Save off information from previous orbital if this is
        # not the first.
        #
        if ($#contractionSet >= 0)
        {
          push(@orbitalList,[$orbital,[@contractionSet]]);
          @contractionSet = ();
          #
          # Save off the orbitals for this center if starting a new one
          #
          if ($center ne $1)
          {
            if ($orbType eq "orbital")
            {
              $gbs{$center} = [@orbitalList];
            }
            if ($orbType eq "exchange")
            {
              $dftXC{$center} = [@orbitalList];
            }
            if ($orbType eq "charge")
            {
              $dftCD{$center} = [@orbitalList];
            }
            @orbitalList = ();
          }
        }
        $center = $1;
        $orbital = $2;
      }

      #
      # Read lines containing primitives (both gbs and ecp)
      # Each line contains two or more columns of numbers separated by white space
      # Each number is an optional sign followed by a combination of
      # numbers and a decimal point.
      #
      if (/^\s*-?[\d\.]+\s+-?[\d\.]+/)
      {
        (@contractionRow) = split;
        if ($orbType ne "ecp")   # GBS
        {
          #
          # Add this contractionRow onto the contractionSet array
          #
          push (@contractionSet,[@contractionRow]);
        }
        elsif ($orbType eq "ecp") # then it is an ecp
        {
          #
          # Add this primitive onto the ECPprimitiveList array
          #
          push (@ECPprimitiveList,{Rexponent => $contractionRow[0],
                                   GaussExponent => $contractionRow[1],
                                   Coefficient => $contractionRow[2] } ); 
        }
      }

      # Save off GBS function information
      if ( (/END/i) && ($orbType ne "ecp") )
      {
        push(@orbitalList,[$orbital,[@contractionSet]]);
        @contractionSet = ();
        if ($orbType eq "orbital")
        {
          $gbs{"coordinants"} = $coordinants;
          $gbs{$center} = [@orbitalList];
        }
        if ($orbType eq "exchange")
        {
          $dftXC{"coordinants"} = $coordinants;
          $dftXC{$center} = [@orbitalList];
        }
        if ($orbType eq "charge")
        {
          $dftCD{"coordinants"} = $coordinants;
          $dftCD{$center} = [@orbitalList];
        }
        @orbitalList = ();
      }

      # flag the start of an ecp
      if (/^\s*ECP/i)
      {
        $orbType = "ecp";
      }

      # Detect the start of an element's ecp data
      if ( ($orbType eq "ecp") &&
           (/^\s*(\w+)\s+nelec\s+(\d+)$/) )
      {
        if ($#ECPprimitiveList >= 0) # Don't save on 1st one
        {
          push(@components, [$shellType, [@ECPprimitiveList]]);
          @ECPprimitiveList = ();
        }
        if ($#components >= 0)       # Don't save on 1st one
        {
          $ecp{$center} = [$numberElectrons, [@components]];
          @components = ();
        }
        $center = $1;
        $numberElectrons = $2;
      }

      # Detect the start of an ecp's component
      if ( ($orbType eq "ecp") &&
           (/^\s*\w+\s+(\w+)$/) )
      {
        if ($#ECPprimitiveList >= 0) # Don't save on 1st one
        {
          push(@components, [$shellType, [@ECPprimitiveList]]);
          @ECPprimitiveList = ();
        }
        $shellType = $1;
      }

      # Save off ECP function information
      if ( (/END/i) &&
           ($orbType eq "ecp") )
      {
        if($#components >= 0) {
          push(@components, [$shellType, [@ECPprimitiveList]]);
          @ECPprimitiveList = ();
          $ecp{$center} = [$numberElectrons, [@components]];
          @components = ();
        }
      }

    # Parse basis set library names
    } elsif ($readingNameBasis == 1 && $readingNumericalBasis == 0) {

      if (/^basis \"(\w\w) basis\" (\w+) print/)
      {
        $coordinants = $2;
        if ($1 eq "ao")
        {
          $orbType = "orbital";
          $name_gbs{"coordinants"} = $coordinants;
        }
        elsif ($1 eq "xc")
        {
          $orbType = "exchange";
          $name_dftXC{"coordinants"} = $coordinants;
        }
        elsif ($1 eq "cd")
        {
          $orbType = "charge";
          $name_dftCD{"coordinants"} = $coordinants;
        }
      }

      # Read line containing the Atomic Symbol and basis set
      if ( (/^(\w+)\s+library\s+(\".+\")$/i) )
      {
        $center = $1;
        $basisName = $2;
        if ($orbType eq "orbital")
        {
          $name_gbs{$center} = $basisName;
        }
        if ($orbType eq "exchange")
        {
          $name_dftXC{$center} = $basisName;
        }
        if ($orbType eq "charge")
        {
          $name_dftCD{$center} = $basisName;
        }
        if ($orbType eq "ecp")
        {
          $name_ecp{$center} = $basisName;
        }
      }

      # flag the start of an ecp
      if (/^\s*ECP/i)
      {
        $orbType = "ecp";
      }
    }
  }

  # Return a reference to the data just read as the variable %bs
  my %bs;
  $bs{"gbs"} = \%gbs;
  $bs{"ecp"} = \%ecp if (scalar(%ecp));
  $bs{"dftXCfitting"} = \%dftXC if (scalar(%dftXC));
  $bs{"dftCDfitting"} = \%dftCD if (scalar(%dftCD));

  $bs{"name_gbs"} = \%name_gbs if (scalar(%name_gbs));
  $bs{"name_ecp"} = \%name_ecp if (scalar(%name_ecp));
  $bs{"name_dftXCfitting"} = \%name_dftXC if (scalar(%name_dftXC));
  $bs{"name_dftCDfitting"} = \%name_dftCD if (scalar(%name_dftCD));

  return \%bs;
}
1;
