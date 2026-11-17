# Counterpoise (#1091) shared by ai.ecceqm and ai.orca: reads the fragment
# definition the Builder stores in the .frag file and plans the extra runs.
# The runs are appended to the deck after "# ecce-run <name>" lines (a comment
# in both input formats); gensub splits them again, so there is one input file.

# Returns the runs to append: [name, charge, multiplicity, \@mode] with mode
# per atom 0 = real, 1 = ghost, 2 = left out.  Empty when the option is off.
# $nameOf->(i) gives the atomic number of atom i, for the electron parity.
sub cpPlan {
  my ($fragFile, $abi, $totalCharge, $nameOf) = @_;
  return () unless (($abi->{"ES.Runtype.Counterpoise"} || "") eq "1");
  die "ai.counterpoise error: the counterpoise correction is for Energy runs\n"
    unless (($abi->{"RunType"} || "") eq "Energy");
  my ($natoms, @ids, %state) = (0);
  open(my $fh, "<", $fragFile) || die "ai.counterpoise error: cannot read $fragFile\n";
  while (<$fh>) {
    if (/^num_atoms:\s*(\d+)/) { $natoms = $1; }
    elsif (/^fragment_states:\s*(\d+)/) {
      my $n = $1;
      for (my $i = 0; $i < $n; $i++) {
        my ($id, $q, $m) = split(' ', scalar(<$fh>));
        $state{$id} = [$q, $m];
      }
    } elsif (/^atom_fragment_list:/) {
      for (my $i = 0; $i < $natoms; $i++) { my $v = <$fh>; $v =~ s/\s+//g; push(@ids, $v); }
    }
  }
  close($fh);
  my %seen;
  my @order = grep { $_ != 0 && !$seen{$_}++ } @ids;
  die "ai.counterpoise error: the counterpoise correction needs at least two " .
      "fragments; define them in the Builder (fragments auto)\n" if (@order < 2);
  die "ai.counterpoise error: " . scalar(grep { $_ == 0 } @ids) .
      " atom(s) belong to no fragment\n" if (grep { $_ == 0 } @ids);
  my $sum = 0;
  foreach my $id (@order) {
    my ($q, $m) = @{ $state{$id} || [0, 0] };
    if (!$m) {            # no stored state: neutral, electron parity
      my $n = 0;
      for (my $i = 0; $i < @ids; $i++) { $n += $nameOf->($i) if ($ids[$i] == $id); }
      $m = ($n - $q) % 2 ? 2 : 1;
    }
    $state{$id} = [$q, $m];
    $sum += $q;
  }
  die "ai.counterpoise error: the fragment charges add up to $sum, the " .
      "calculation's charge is $totalCharge\n" if ($sum != $totalCharge);
  my @runs;
  my $own = (($abi->{"ES.Runtype.CounterpoiseOwn"} || "") eq "1");
  foreach my $pass ($own ? (1, 2) : (1)) {
    foreach my $id (@order) {
      my @mode = map { $_ == $id ? 0 : ($pass == 1 ? 1 : 2) } @ids;
      push(@runs, ["fragment-$id" . ($pass == 2 ? "-own" : ""),
                   $state{$id}[0], $state{$id}[1], \@mode]);
    }
  }
  return @runs;
}
# Expand a stored geometry snapshot through the same complex/monomer template.
# Names carry the point index so the job and parser cannot reorder the scan.
sub cpWriteDecks {
  my ($abi, $runs, $code) = @_;
  open(my $in, "<", "tmpfile") or die "Cannot read counterpoise template\n";
  my @base = <$in>;
  close($in);
  my $snapshot = $abi->{"ES.Runtype.CPScanGeometry"} || "";
  my @points = $snapshot ne "" ? split(/;/, $snapshot) : ("");
  open(my $out, ">", "tmpfile") or die "Cannot write counterpoise decks\n";
  print $out "# ecce-cp-points " . scalar(@points) . "\n" if ($snapshot ne "");
  for (my $point = 0; $point < @points; $point++) {
    my @xyz = split(' ', $points[$point]);
    my $prefix = $snapshot ne "" ? sprintf("point-%06d-", $point + 1) : "";
    foreach my $run (["complex", undef, undef, undef], @$runs) {
      my ($name, $q, $m, $mode) = @$run;
      print $out "# ecce-run $prefix$name\n" unless ($point == 0 && $name eq "complex");
      my ($geom, $atom) = (0, 0);
      foreach my $line (@base) {
        my $l = $line;
        next if ($code eq "ecceqm" && ($mode || $snapshot ne "") &&
                 $l =~ /^(write_orbitals|read_orbitals|charges)\b/);
        if ($geom) {
          if (($code eq "ecceqm" && $l =~ /^end\s*$/) ||
              ($code eq "orca" && $l =~ /^\*\s*$/)) {
            $geom = 0;
            die "Counterpoise scan geometry atom count differs\n"
              if ($snapshot ne "" && @xyz != 3 * $atom);
          } else {
            my $mo = $mode ? $mode->[$atom] : 0;
            if ($snapshot ne "") {
              my ($symbol) = split(' ', $l);
              my @v = @xyz[3*$atom .. 3*$atom+2];
              die "Invalid counterpoise scan geometry\n"
                if (grep { !defined($_) || !/^[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?$/ } @v);
              $l = " $symbol " . join(" ", @v) . "\n";
            }
            $atom++;
            next if ($mo == 2);
            if ($mo == 1) {
              if ($code eq "orca") { $l =~ s/^(\s*)(\S+)/$1$2:/; }
              else { chomp($l); $l .= " ghost\n"; }
            }
          }
        } elsif ($code eq "ecceqm") {
          $l =~ s/^charge .*/charge $q/ if ($mode);
          $l =~ s/^multiplicity .*/multiplicity $m/ if ($mode);
          $geom = 1 if ($l =~ /^geometry\s*$/);
        } elsif ($l =~ /^\*\s*xyz\s/i) {
          $l = "* xyz $q $m\n" if ($mode);
          $geom = 1;
        }
        print $out $l;
      }
    }
  }
  close($out);
}
1;
