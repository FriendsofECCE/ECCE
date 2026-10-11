# Removes from a Strawberry Perl tree every vendor module that is not part
# of perl's core library, leaving what a stock perl has.  Strawberry moves
# the newer releases of dual-life core modules (Socket, Scalar::Util, ...)
# into vendor\lib, so those stay.  Run with that tree's own perl:
#   perl perl-core-only.pl <strawberry>\perl\vendor\lib
use strict;
use warnings;
use File::Find;
use Module::CoreList;

my $root = shift or die "usage: perl-core-only.pl <vendor lib dir>\n";
$root =~ s{\\}{/}g;
my $v = $];

# A file belongs to the module named by its path (A/B.pm -> A::B), or by
# its directory (A/B/x.txt, auto/A/B/B.dll -> A::B, else A).
sub core_owner {
    my ($rel) = @_;
    $rel =~ s{^auto/}{};
    my @p = split m{/}, $rel;
    if ($p[-1] =~ s/\.pm$//) {
        return 1 if Module::CoreList::is_core(join('::', @p), undef, $v);
    }
    pop @p;
    while (@p) {
        return 1 if Module::CoreList::is_core(join('::', @p), undef, $v);
        pop @p;
    }
    return 0;
}

my ($kept, $removed) = (0, 0);
my @gone;
find({ no_chdir => 1, wanted => sub {
    return unless -f $_;
    (my $rel = $File::Find::name) =~ s{^\Q$root\E/}{};
    if ($rel =~ m{^Portable(\.pm$|/)} || core_owner($rel)) { $kept++ }
    else { push @gone, $File::Find::name }
} }, $root);
for (@gone) { unlink $_ or die "unlink $_: $!\n"; $removed++ }
finddepth(sub { rmdir $_ if -d $_ }, $root);   # empty directories
print "perl-core-only: kept $kept, removed $removed files under $root\n";
