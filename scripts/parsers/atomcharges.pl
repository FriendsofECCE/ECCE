# Shared by the per-scheme partial-charge parse scripts (orca.loewdin,
# gaussian-16.apt, ...): one number per atom in, a PropVector<Atom> out, the
# same shape as MULLIKEN so MullikenPanel can draw it.  Rows differ between
# programs only in where the element symbol sits and whether it is followed
# by a colon, so one matcher covers all of them.
#
#   ORCA      "   0 O :   -0.252811"      "   0 H    0.155782    0.000000"
#   Gaussian  "     1  O   -0.169203"
#   NBO (NPA) "    O  1   -0.88855      1.99996 ..."   (symbol first)

sub chargeRows {
  my @v;
  while (<STDIN>) {
    if (/^\s*\d+\s+[A-Za-z]{1,2}\w*\s*:?\s+(-?\d+\.\d+)/) {
      push(@v, $1);
    } elsif (/^\s*[A-Za-z]{1,2}\w*\s+\d+\s+(-?\d+\.\d+)\s+-?\d+\.\d+\s/) {
      push(@v, $1);
    }
  }
  return @v;
}

sub emitCharges {
  my ($key, @v) = @_;
  return if (!@v);
  print "key: $key\n";
  print "size:\n", scalar(@v), "\n";
  print "values:\n";
  print join(" ", @v), "\n";
  print "units:\ne\n";
  print "END\n";
}

1;
