package QEFunctional;
# The exchange-correlation functional a Quantum ESPRESSO pseudopotential
# was generated with, read from its UPF header, as one comparable name.
# pw.x stops with "conflicting values for icorr" (or iexch, igcx, igcc)
# when two pseudopotentials of a run disagree, so ai.qe uses this to pick
# a matching set and verifyinput to check a deck.  Used by both, so the
# two cannot drift.
use strict;

# The four-part form pw.x prints (exchange, correlation, gradient
# exchange, gradient correlation) for the functionals a pseudopotential
# library ships.  Anything not listed is compared by its four parts.
my %FOUR = (
  "SLA PW PBX PBC"   => "PBE",
  "SLA PW PSX PSC"   => "PBESOL",
  "SLA PW RPB PBC"   => "REVPBE",
  "SLA PZ NOGX NOGC" => "PZ",
  "SLA PW NOGX NOGC" => "PW",
  "SLA LYP B88 BLYP" => "BLYP",
  "SLA PW GGX GGC"   => "PW91",
);
my %ALIAS = ("LDA" => "PZ", "SLA PZ" => "PZ");

# A functional as written in a UPF header or in input_dft -> its name.
sub canonical {
  my ($s) = @_;
  return "" unless (defined($s));
  my @t = split(' ', uc($s));
  return "" unless (@t);
  if (@t >= 4) {
    # Old UPF files spell the gradient parts of PBE as "PBE PBE".
    $t[2] = "PBX" if ($t[2] eq "PBE");
    $t[3] = "PBC" if ($t[3] eq "PBE");
    my $four = join(" ", @t[0 .. 3]);
    return $FOUR{$four} if (exists($FOUR{$four}));
    return $four;
  }
  my $name = join(" ", @t);
  return exists($ALIAS{$name}) ? $ALIAS{$name} : $name;
}

# The functional of the UPF file at $path, or "" when the file cannot be
# read or has no recognisable header.  Only the header is read.
sub ofFile {
  my ($path) = @_;
  open(my $fh, '<', $path) or return "";
  my $n = 0;
  my $found = "";
  while (my $line = <$fh>) {
    last if (++$n > 400);
    if ($line =~ /\bfunctional\s*=\s*"([^"]*)"/i) { $found = $1; last; }
    if ($line =~ /^\s*(.*?)\s+Exchange-Correlation functional/i) { $found = $1; last; }
  }
  close($fh);
  return canonical($found);
}

1;
