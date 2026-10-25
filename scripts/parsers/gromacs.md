#!/usr/bin/env perl
################################################################################
# Purpose:
# The speed distributions and mean-square displacements of a GROMACS run, as
# the mdanalysis program writes them to ecce-parse-md.txt,
#     ECCE-MDSPEED <record>
#     ECCE-MDMSD <record>
# one record (no blanks) per atom mass or molecule type.  They become the string
# vectors MDSPEED and MDMSD of the Speed Distribution and Displacement panels,
# which decode them with include/util/MdAnalysis.H and src/util/mdanalysis.
################################################################################

$| = 1;
my (@speed, @msd);
while (<STDIN>) {
  push(@speed, $1) if (/^ECCE-MDSPEED\s+(\S+)\s*$/);
  push(@msd, $1) if (/^ECCE-MDMSD\s+(\S+)\s*$/);
}
foreach my $v (["MDSPEED", \@speed], ["MDMSD", \@msd]) {
  my $n = scalar(@{$v->[1]});
  next if ($n == 0);
  print "key: $v->[0]\nsize:\n$n\nrowlabels:\n", join(" ", 1 .. $n),
        "\nvalues:\n", join(" ", @{$v->[1]}), "\nEND\n";
}
exit(0);
