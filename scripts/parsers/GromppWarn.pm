package GromppWarn;
# Which grompp warnings are harmless.  grompp stops on any warning unless
# -maxwarn allows it, so ECCE runs it once with a high -maxwarn, sorts the
# warnings it printed with this table, and passes the count of harmless ones
# as -maxwarn: any warning not listed here still stops the run.  Used by
# gromacs.launchpp (the launch check) and, as an awk program made from the
# same table, by the job script (gensub; run with -v marked=0|1, the
# topology's net-charge marker).
#
# Patterns are POSIX ERE, matched against the warning's text on one line, so
# that Perl and awk agree.  Add a warning here only when it cannot mean a
# wrong calculation on its own; say why.
use strict;

our @TABLE = (
  { id  => 'atom-names',
    re  => 'non-matching atom names|atom name [0-9]+ in .* does not match',
    why => 'the atom names of the structure and the topology differ; grompp takes the topology\'s. '
         . 'Harmless only when the atoms are in the same order, which grompp cannot check '
         . '(a different count is a grompp error).' },
  { id  => 'berendsen',
    re  => 'Berendsen (thermostat|barostat) does not generate',
    why => 'Berendsen coupling was chosen in the task; grompp warns about it whatever the system.' },
  { id  => 'gromos-twin-range',
    re  => 'GROMOS force fields have been parametrized with a physically incorrect',
    why => 'any GROMOS force field draws this warning, whatever the system.' },
  # Needs the charge from grompp's "System has non-zero total charge" note.
  { id  => 'net-charge',
    re  => 'Ewald electrostatics in a system with net charge',
    why => 'the net charge is below 1e-4, which is rounding, or the Prepare left it on purpose '
         . '(Neutralise unticked: the topology carries ECCE-NETCHARGE-INTENDED).',
    charge => 1 },
);

our $CHARGE_LIMIT = 1e-4;

# The warnings in grompp's output, each as { head, text, harmless, why } (text
# on one line).  $intended: the topology carries the Prepare's marker for a
# net charge left on purpose.
sub parse
{
  my ($out, $intended) = @_;
  my (@w, $cur);
  my $charge;
  foreach my $l (split(/\n/, $out)) {
    if ($l =~ /^WARNING \d+( \[.*\])?:\s*$/) {
      push(@w, $cur) if ($cur);
      $cur = { head => $l, text => '' };
      next;
    }
    if ($cur && $l =~ /^\s+\S/) {
      (my $t = $l) =~ s/^\s+//;
      $cur->{text} .= ($cur->{text} ne '' ? ' ' : '') . $t;
      next;
    }
    push(@w, $cur) if ($cur);
    $cur = undef;
    $charge = $1 if ($l =~ /System has non-zero total charge:\s*(\S+)/);
  }
  push(@w, $cur) if ($cur);
  foreach my $w (@w) {
    $w->{harmless} = 0;
    foreach my $e (@TABLE) {
      next if ($w->{text} !~ /$e->{re}/);
      next if ($e->{charge} && !$intended && !(defined($charge) && abs($charge) < $CHARGE_LIMIT));
      $w->{harmless} = 1;
      $w->{why} = $e->{why};
      last;
    }
  }
  return @w;
}

# An awk program that reads grompp's output and prints "harmless N" and
# "stopping M", then one "harmless: <text>" or "stopping: <text>" line per
# warning, from the same table.
sub awkProgram
{
  my $conds = '';
  foreach my $e (@TABLE) {
    die "GromppWarn: '/' in a pattern" if ($e->{re} =~ m#/#);
    my $test = "t ~ /$e->{re}/";
    $test .= " && (marked == 1 || (q != \"\" && q < $CHARGE_LIMIT))" if ($e->{charge});
    $conds .= "    if ($test) h = 1\n";
  }
  my $prog = <<'AWK';
function push() { if (inw) { n++; txt[n] = cur }; inw = 0; cur = "" }
/^WARNING [0-9]+( \[.*\])?:[ \t]*$/ { push(); inw = 1; next }
inw && /^[ \t]+[^ \t]/ { l = $0; sub(/^[ \t]+/, "", l); cur = (cur == "" ? l : cur " " l); next }
{ push() }
/System has non-zero total charge:/ { q = $NF + 0; if (q < 0) q = -q }
END {
  push()
  for (i = 1; i <= n; i++) {
    t = txt[i]; h = 0
@@CONDS@@    if (h) nh++; else ns++
    line[i] = (h ? "harmless: " : "stopping: ") t
  }
  print "harmless " nh + 0
  print "stopping " ns + 0
  for (i = 1; i <= n; i++) print line[i]
}
AWK
  $prog =~ s/\@\@CONDS\@\@/$conds/;
  return $prog;
}

1;
