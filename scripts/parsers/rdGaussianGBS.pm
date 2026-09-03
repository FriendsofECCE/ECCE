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
  my ($numPrimitives,@centersInGroup,$orbital,
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

    # Read line containing the center numbers. A "Centers:" line can list
    # centers of DIFFERENT elements sharing one basis (e.g. a single "use
    # 6-31G for both C and H" group is completely normal in a real GEN
    # basis specification), and Gaussian wraps long center lists across
    # multiple consecutive "Centers:" lines before the shared basis name
    # -- confirmed directly against real Gaussian 16 log output.
    #
    # The previous version of this parser used `(.{3})` to grab 3
    # characters right after "Centers:\s" (one space), which for Gaussian's
    # right-justified number columns is almost always still whitespace,
    # not the digit -- confirmed live: for "Centers:       1", $1 came
    # back as three spaces, and Perl's numeric coercion of that is 0, so
    # `$centers[$1-1]` was `$centers[-1]` -- Perl's *last* array element,
    # not the first center in the list. Every group in a molecule ended
    # up keyed by whatever element the *last* atom in the whole molecule
    # happens to be, with each new group's data silently overwriting the
    # previous one in %gbs. Confirmed directly: a 2-group mixed-basis
    # test (C atoms on one named basis, H atoms on another) came back
    # with only an "H" entry -- the entire C group's data vanished.
    # This is at minimum a real, direct contributor to GitHub #28/#29
    # (both triggered by a real Gaussian log with more than one "Centers:"
    # group) -- for the specific reported repros, which use named-library
    # references ("cc-pvdz"/"cc-pvtz") rather than explicit numeric
    # primitives, Gaussian's own log doesn't expand the name into
    # primitives in this section at all, so this fix alone isn't
    # sufficient for that exact case -- see the accompanying issue
    # comment for what's still needed there.
    #
    # Fixed to collect every center number from every "Centers:" line in
    # the group (resetting the collection only when a *new* group starts,
    # i.e. right after the previous group's "****"), then storing the
    # orbital data under every distinct element symbol among them once
    # the group's terminating "****" is reached.
    if (/^\s*Centers:\s*(.*)$/)
    {
      $getdata = 1;
      foreach my $centerNum (split(/\s+/, $1)) {
        next if ($centerNum eq "");
        push(@centersInGroup, $centers[$centerNum-1]);
      }
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
        # Save off the data for every distinct element among all the
        # centers collected for this group (not just the first one --
        # see the "Centers:" handling above for why that was wrong).
        my %seen;
        foreach my $symbol (@centersInGroup) {
          next if ($seen{$symbol}++);
          $gbs{$symbol} = [@orbitalList];
        }
        @orbitalList = ();
      }
      @centersInGroup = ();
      $getdata = 0;
    }

  }

  close GBS;

  $bs{"gbs"} = \%gbs;

  # Return a reference to the data just read as the variable $gbs
  return \%bs;
}
1;
