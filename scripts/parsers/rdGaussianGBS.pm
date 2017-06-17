################################################################################
#
# This perl module will read a "gaussian" format gbs file.
# To use this module, "require" it in the calling program.
# The calling program must be running under perl5.
#
# The data is returned as $gbs
#    gbs  : \%atomList
#           a gbs is a pointer to a hash
#      %atomList  :  {  $centerName => \@orbitalList }
#                    an atomList is a hash of centers and pointers to arrays
#      @orbitalList : ( [ $orbitalType , \@primitiveList ] )
#                     orbitalList is an array of pointers to arrays, each of 
#                     which is one orbitalType and one pointer to an array
#      @primitiveList : ( { exponent     => $exponentValue,
#                           Scoefficient => $ScoefficientValue,
#                           Pcoefficient => $PcoefficientValue} )
#                       primitiveList is an array of pointers to hashes.
#                       each hash contains three key/value pairs.
#           
#      $centerName : number of the center or the atomic symbol
#      $orbitalType : S, P, SP, D...
#
# The file to read the data in from is in the current working directory
################################################################################

sub getGaussianGBS {
  my($mvmFile, $filename) = @_;
  # Initialize variables
  my ($numPrimitives,$center,$orbital,
      @contractionList,@contractionSet,
      @orbitalList,%atomList,@centers,$getdata,%gbs);
  $getdata = 0;

  # Open standard file to read
  open(GBS,$filename) || die "Cannot open file ./$filename";

  # Open and read the mvm file in this directory in order to associate
  # center numbers with atomic symbols
  open(MVM,$mvmFile) || die "Cannot open file ./$mvmFile";
  while (<MVM>)
  {
    if (/^atom_list:/)
    {
      while (<MVM>)
      {
        my($symbol,@coords);
        chop;
        s/^\s*(.*)\s*$/$1/;  # Get rid of leading and trailing blanks
        ($symbol,@coords) = split(/ /);
        push(@centers,$symbol);
      }
    }
  }
  close MVM;

  # Read and process the file
  while (<GBS>)
  {
    # Trim off comments starting with "#"
    s/#.*$//;

    # Trim off trailing spaces
    s/\s+$//;

    # Determine whether basis is in spherical or cartesian coordinants
    if (/General basis read from cards:\s*\((.*)\)\s*/)
    {
      my $coorddata = $1;
      # Gaussian 98 may have ability to assign 
      # spherical coordinants to higher orbitals
      # Gaussian 94 only supports sphericals in D
      # and F orbitals

      if($coorddata =~ /6D|10F|15G|21H|28I/) {
        $gbs{"coordinants"} = "cartesian";
      } elsif($coorddata =~ /5D|7F|9G|11H|13I/) {
        $gbs{"coordinants"} = "spherical";
      }
    }

    # Read line containing the center numbers
    # Assume that all of the centers have the same atomic symbol
    # (Read only the first one and ignore the rest)
    # Get the atomic symbol by indexing into the @centers array that
    # was read from the mvm format file. Subtract one to get the
    # array item counting from 0 instead of 1.
    if (/^\s*Centers:\s(.{3})/)
    {
      $getdata = 1;
      $center = $centers[$1-1];
    }

    # Read line containing the orbital and the number of primitives
    if ($getdata == 1 && /\s*(\w+)\s+(\d+)\s+1\.00$/)
    {
      $orbital = $1;
      $numPrimitives = $2;
      @contractionSet = ();
      # Then read the lines of primitives
      my $i;
      for ($i=0;$i<$numPrimitives;$i++)
      {
        my $line = <GBS>;
        chop $line;
        $line =~ s/D/E/;
        $line =~ /^\s*Exponent=\s+(.*)\s+Coefficients=\s+(.*)\s*/;
        push( @contractionSet, [$1, split(/\s+/,$2)] );
      }
      push(@orbitalList,[$orbital,[@contractionSet]]);
    }

    # Read the end of center mark
    if (/\*\*\*\*/)
    {
      if($getdata == 1) {
        # Save off the data for the center
        $gbs{$center} = [@orbitalList];
        @orbitalList = ();
      }
      $getdata = 0;
    }

  }

  close GBS;

  $bs{"gbs"} = \%gbs;

  # Return a reference to the data just read as the variable $gbs
  return \%bs;
}
1;
