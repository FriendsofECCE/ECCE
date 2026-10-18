package GmxIons;
# pdb2gmx refuses a lone atom the Builder calls UNK ("residue UNK not found"):
# a force field knows an ion only under its own residue and atom name.  Those
# names were taken by running pdb2gmx 2025.2 on a one-atom PDB for every name
# and every force field of its share directory (the rtp files differ: AMBER
# has MG and ZN, CHARMM27 calls sodium SOD, GROMOS54A7 has only NA and CL).
# A lone atom is renamed only if its residue is UNK or empty, its element is
# in the force field's list, and no other atom has the same residue key; a
# residue name the user typed is never touched, except that a lone atom whose
# residue already is the force field's ion name gets that atom name too (the
# Builder numbers its atoms, NA1, and pdb2gmx wants NA).  Used by gromacs.launchpp (the
# launch check, rewrite) and, as an awk program made from the same table, by
# the job script (gensub).
use strict;

our @FAMILIES = (
  { re => '^amber', ions => { map { $_ => $_ } qw(NA CL K CA MG ZN LI RB CS) } },
  { re => '^oplsaa', ions => { map { $_ => $_ } qw(NA CL K CA MG LI RB CS F BR) } },
  { re => '^charmm', ions => { NA => 'SOD', CL => 'CLA', K => 'POT', CA => 'CAL',
                               CS => 'CES', MG => 'MG' } },
  { re => '^gromos54a7', ions => { map { $_ => $_ } qw(NA CL) } },
  { re => '^gromos', ions => { map { $_ => $_ } qw(NA CL CA MG ZN CU) } },
);

# element => name for a force field directory name ("amber99sb-ildn"), {} if unknown
sub ions
{
  my ($ff) = @_;
  $ff =~ s/\.ff$//;
  foreach my $f (@FAMILIES) {
    return $f->{ions} if ($ff =~ /$f->{re}/);
  }
  return {};
}

# The PDB text with each lone UNK ion renamed; returns (text, notes).
sub rewrite
{
  my ($text, $ff) = @_;
  my $ion = ions($ff);
  my @lines = split(/\n/, $text, -1);
  my %count;
  foreach my $l (@lines) {
    $count{substr($l, 17, 10)}++ if ($l =~ /^(ATOM  |HETATM)/);
  }
  my @notes;
  foreach my $l (@lines) {
    next if ($l !~ /^(ATOM  |HETATM)/);
    next if ($count{substr($l, 17, 10)} != 1);
    (my $res = substr($l, 17, 3)) =~ s/^\s+|\s+$//g;
    (my $el = length($l) > 76 ? substr($l, 76, 2) : '') =~ s/^\s+|\s+$//g;
    $el = uc($el);
    my $name = $ion->{$el};
    next if (!defined($name));
    next if ($res ne '' && $res ne 'UNK' && $res ne $name);
    my $atom = length($el) > 1 ? sprintf("%-4s", $name) : sprintf(" %-3s", $name);
    substr($l, 12, 4) = $atom;
    substr($l, 17, 3) = sprintf("%-3s", $name);
    push(@notes, "$el: residue " . ($res eq '' ? '(none)' : $res) . " -> $name") if ($res ne $name);
  }
  return (join("\n", @lines), @notes);
}

# An awk program for the job script: reads a PDB on stdin, writes it with the
# renames, notes on stderr; the force field is passed as -v ff=NAME.
sub awkProgram
{
  my $setup = '';
  foreach my $f (@FAMILIES) {
    die "GmxIons: '/' in a pattern" if ($f->{re} =~ m#/#);
    $setup .= "  if (!done && ff ~ /$f->{re}/) { done = 1\n";
    $setup .= "    ion[\"$_\"] = \"$f->{ions}{$_}\"\n" for sort keys %{$f->{ions}};
    $setup .= "  }\n";
  }
  my $prog = <<'AWK';
function trim(s) { gsub(/^[ \t]+|[ \t]+$/, "", s); return s }
BEGIN {
  sub(/\.ff$/, "", ff)
@@SETUP@@}
{ line[NR] = $0 }
/^(ATOM  |HETATM)/ { count[substr($0, 18, 10)]++ }
END {
  for (i = 1; i <= NR; i++) {
    l = line[i]
    if (l ~ /^(ATOM  |HETATM)/ && count[substr(l, 18, 10)] == 1) {
      res = trim(substr(l, 18, 3))
      el = toupper(trim(length(l) > 76 ? substr(l, 77, 2) : ""))
      if (el in ion && (res == "" || res == "UNK" || res == ion[el])) {
        nm = ion[el]
        an = (length(el) > 1) ? sprintf("%-4s", nm) : sprintf(" %-3s", nm)
        l = substr(l, 1, 12) an substr(l, 17, 1) sprintf("%-3s", nm) substr(l, 21)
        if (res != nm) print "ECCE-PREPARE ion " el ": residue " (res == "" ? "(none)" : res) " -> " nm > "/dev/stderr"
      }
    }
    print l
  }
}
AWK
  $prog =~ s/\@\@SETUP\@\@/$setup/;
  return $prog;
}

1;
