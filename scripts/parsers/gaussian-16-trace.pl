# Shared by the gaussian-16.geomtrace* scripts: one GEOMTRACE frame per
# optimisation step (#498).  Gaussian prints an Input (or Z-Matrix) block and,
# when symmetry is on, a Standard block for every step.  The Standard block is
# the frame normal modes and orbitals use (#312), so it wins; the Input block
# is held in $key/g16InputGeom and published only when no Standard block
# follows (symmetry off) or when the next block arrives and it is still held.

# Reads one orientation block from STDIN; returns the rows and labels.
sub readBlock {
  my (@rows, @labels, $n);
  while (<STDIN>) {
    last unless /^\s+\d+/;
    chop;
    s/^\s*//;
    s/D/e/g;
    my ($idx, $z, $type, $x, $y, $zc) = split(/ +/, $_);
    # Dummy centres (Z-matrix "X") are not in ECCE's fragment (#27, #298).
    next if ($z < 0);
    push(@rows, "$x $y $zc");
    push(@labels, ++$n . "-" . $atomicSymbol[$z]);
  }
  return (\@rows, \@labels);
}

# Gaussian prints the converged geometry once more after "Optimization
# completed" (and again for a following frequency job); that repeat is not a
# step, so a frame equal to the last published one is dropped.  Equal means
# numerically: the repeat can print -0.000000 where the step printed 0.000000.
sub sameFrame {
  my @a = split(" ", $_[0]);
  my @b = split(" ", $_[1]);
  return 0 unless @a == @b;
  for my $i (0 .. $#a) { return 0 unless $a[$i] == $b[$i]; }
  return 1;
}

sub emitFrame {
  my ($key, $rows, $labels) = @_;
  return unless @$rows;
  my $sig = join(" ", @$rows);
  my $lastFile = $key . "/g16LastGeom";
  if (open(my $fh, "<", $lastFile)) {
    my $last = <$fh>;
    close($fh);
    chomp $last if defined $last;
    return if (defined $last && sameFrame($last, $sig));
  }
  if (open(my $fh, ">", $lastFile)) { print $fh "$sig\n"; close($fh); }
  print "key: GEOMTRACE\nsize:\n1 " . scalar(@$rows) . " 3\n";
  print "rowlabels:\n " . join(" ", @$labels) . "\n";
  print "columnlabels:\nX Y Z\nvalues:\n";
  print " $_\n" foreach @$rows;
  print "units:\nAngstrom\nEND\n";
}

sub stashFile { return $_[0] . "/g16InputGeom"; }

sub saveStash {
  my ($key, $rows, $labels) = @_;
  return unless @$rows;
  if (open(my $fh, ">", stashFile($key))) {
    print $fh join(" ", @$labels), "\n";
    print $fh "$_\n" foreach @$rows;
    close($fh);
  }
}

# Returns the held Input frame (or empty lists) and removes it.
sub takeStash {
  my ($key) = @_;
  my (@rows, @labels);
  if (open(my $fh, "<", stashFile($key))) {
    my $l = <$fh>;
    if (defined $l) { chop $l; @labels = split(/ /, $l); }
    while (<$fh>) { chop; push(@rows, $_); }
    close($fh);
  }
  unlink(stashFile($key));
  return (\@rows, \@labels);
}

# IRC side files (gaussian-16.ircgeom, .ircts, .irc): one structure per line,
# its atoms' "x y z" joined by ",".
sub saveIrcInput {
  my ($key, $rows) = @_;
  return unless @$rows;
  if (open(my $fh, ">", "$key/g16IrcInput")) {
    print $fh join(",", @$rows), "\n";
    close($fh);
  }
}

1;
