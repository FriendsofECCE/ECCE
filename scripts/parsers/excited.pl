# Shared by the per-code excited-state parse scripts (gaussian-16.excited,
# orca.excited): one root per row, as four vectors over "Excited State".
# Energies are eV and oscillator strengths dimensionless, whatever the code
# prints; wavelength is derived from the energy by the panel, not stored.
# Transitions are one whitespace-free string per root, "8>9:0.70503,7>10:-0.189"
# (occupied>virtual:coefficient), because a string-vector value cannot hold
# spaces.  MO numbers are 1-based, as in the MO panel.
#
# A state is a hash: ev, f, label, trans (array of "i>a:c").

sub emitExcited {
  my @st = @_;
  return if (!@st);
  my $n = scalar(@st);
  my $labels = join(" ", 1 .. $n);

  print "key: EXCIENERGY\nsize:\n$n\nrowlabels:\n$labels\nvalues:\n";
  print join(" ", map { $_->{ev} } @st), "\nunits:\neV\nEND\n";

  print "key: EXCIOSC\nsize:\n$n\nrowlabels:\n$labels\nvalues:\n";
  print join(" ", map { $_->{f} } @st), "\nunits:\nf\nEND\n";

  print "key: EXCISYM\nsize:\n$n\nrowlabels:\n$labels\nvalues:\n";
  print join(" ", map { my $l = $_->{label}; $l =~ s/\s+/_/g;
                         $l eq "" ? "-" : $l } @st), "\nEND\n";

  print "key: EXCITRANS\nsize:\n$n\nrowlabels:\n$labels\nvalues:\n";
  print join(" ", map { @{$_->{trans}} ? join(",", @{$_->{trans}}) : "-" } @st),
        "\nEND\n";
}

1;
