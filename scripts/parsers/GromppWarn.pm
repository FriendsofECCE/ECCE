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

# The common warnings that stop a job, in ECCE's own words for the launch
# check's dialog: what it is and what to change, impersonal and action first.
# grompp's text is shown beside it.  A warning matching no row is shown as
# grompp wrote it.  Each text is a sub of the parsed warning.
our @EXPLAIN = (
  { re => 'Ewald electrostatics in a system with net charge',
    text => sub {
      my ($w) = @_;
      my $lead = "The system has a net charge.";
      if (defined($w->{charge})) {
        my $q = sprintf("%.3f", $w->{charge}); $q =~ s/\.?0+$//;
        $q =~ s/^-/\xe2\x88\x92/;
        $lead = "Net charge $q e.";
      }
      return "$lead With PME, a net charge adds a uniform background charge that can "
           . "cause artefacts. Neutralise the system with counter-ions "
           . "(Prepare: Neutralise), optionally with added salt.";
    } },
  { re => 'should be at least [0-9]+ times larger than nst[tp]couple',
    text => sub {
      my ($w) = @_;
      if ($w->{text} =~ /tau-([tp]) \(([0-9.eE+-]+)\) should be at least ([0-9]+) times larger than nst[tp]couple\*dt \(([0-9.eE+-]+)\)/) {
        my ($k, $tau, $times, $dt) = ($1, $2, $3, $4);
        my ($what, $step) = $k eq 't' ? ("thermostat", "nsttcouple") : ("barostat", "nstpcouple");
        my $min = sprintf("%.3g", $times * $dt);
        return "The $what coupling time tau-$k ($tau ps) is too short for its coupling interval "
             . "($step*dt = $dt ps). Raise tau-$k to $min ps or more in the task, or shorten $step.";
      }
      return undef;
    } },
  { re => 'constraints on all bonds',
    text => sub {
      return "Every bond is constrained, but this force field was parametrised with constraints "
           . "on bonds to hydrogen only. Choose h-bonds for constraints in the task, which is also faster.";
    } },
  { re => 'plain Coulomb cut-off',
    text => sub {
      return "Plain Coulomb cut-off electrostatics can produce artefacts. Choose PME for "
           . "coulombtype in the task.";
    } },
  { re => 'center of mass motion in the presence of position restraints',
    text => sub {
      return "Removing centre-of-mass motion while atoms are position-restrained can cause "
           . "artefacts; they are usually negligible when a macromolecule is restrained to "
           . "equilibrate it. Set comm-mode to None in the task to avoid them.";
    } },
);

# .mdp keys as the editor names them (tab: field, as in docs/GROMACS.md), so
# a grompp message the user must act on does not name a key the editor does
# not show.  A key marked own is one ECCE writes without a field; a message
# about it is ECCE's fault, to be reported.  Keys that are ordinary words in
# grompp's prose (dt, pbc) are left out.
our %MDPKEY = (
  'nstenergy' => ['Files tab, write energies interval', 0],
  'nstlog' => ['Files tab, write energies interval', 0],
  'nstcalcenergy' => [undef, 1],
  'nstxout-compressed' => ['Files tab, write trajectory interval', 0],
  'nsteps' => ['Dynamics tab, equilibration and data steps (Optimize tab, maximum steps)', 0],
  'tinit' => ['Dynamics tab, initial time', 0],
  'nstlist' => ['Interactions tab, pair-list update interval', 0],
  'rcoulomb' => ['Interactions tab, cutoff radius', 0],
  'rvdw' => ['Interactions tab, cutoff radius', 0],
  'coulombtype' => ['Interactions tab, PME / Cutoff only', 0],
  'pme-order' => ['Interactions tab, B-spline order', 0],
  'constraints' => ['Constraints tab', 0],
  'comm-mode' => ['Dynamics tab, remove centre of mass motion', 0],
  'nstcomm' => ['Dynamics tab, remove centre of mass motion interval', 0],
  'tcoupl' => ['Dynamics tab, constant temperature', 0],
  'ref-t' => ['Dynamics tab, temperature', 0],
  'tau-t' => ['Dynamics tab, temperature relaxation time', 0],
  'pcoupl' => ['Dynamics tab, constant pressure', 0],
  'ref-p' => ['Dynamics tab, pressure', 0],
  'tau-p' => ['Dynamics tab, pressure relaxation time', 0],
  'compressibility' => ['Dynamics tab, compressibility', 0],
  'pcoupltype' => ['Run settings, barostat coupling type', 0],
  'nstpcouple' => ['Run settings, barostat interval', 0],
  'emtol' => ['Optimize tab, maximum force', 0],
  'emstep' => ['Optimize tab, initial step', 0],
  'define' => ['Run settings, defines', 0],
  'tc-grps' => ['Run settings, groups', 0],
  'comm-grps' => ['Run settings, groups', 0],
  'gen-vel' => ['Dynamics tab, continue', 0],
  'gen-temp' => ['Dynamics tab, continue', 0],
  'continuation' => ['Dynamics tab, continue', 0],
  'cutoff-scheme' => [undef, 1],
  'integrator' => [undef, 1],
);

# $text with each known .mdp key followed by the editor's name for it, the
# key kept in parentheses; a note is added when ECCE wrote the key itself.
sub translate
{
  my ($text) = @_;
  my (%own, $changed);
  foreach my $k (sort { length($b) <=> length($a) } keys %MDPKEY) {
    (my $re = quotemeta($k)) =~ s/\\-/[-_]/g;
    my ($name, $own) = @{$MDPKEY{$k}};
    $changed += ($text =~ s/(?<![\w-])($re)(?![\w-])(?! \()/
                   defined($name) ? "$name ($1)" : $1/ge) if (defined($name));
    $own{$k} = 1 if ($own && $text =~ /(?<![\w-])$re(?![\w-])/);
  }
  my @own = sort keys %own;
  if (@own) {
    $text .= "\n\nECCE writes " . join(", ", @own) . " itself, without a field in the editor, "
           . "so this is a fault in ECCE, not in the settings: please report it.";
  }
  return $text;
}

# ECCE's own text for a warning from parse(), or undef when there is none.
sub explain
{
  my ($w) = @_;
  foreach my $e (@EXPLAIN) {
    next if ($w->{text} !~ /$e->{re}/);
    my $t = $e->{text}->($w);
    return $t if (defined($t));
  }
  return undef;
}

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
    $w->{charge} = $charge;
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
