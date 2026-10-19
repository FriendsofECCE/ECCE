#!/usr/bin/env python3
"""Lone UNK ions get the force field's residue names (scripts/parsers/GmxIons.pm).

Real gmx is the oracle: for every force field of its share directory and every
ion the table lists for it, a one-atom PDB as the Builder writes it (residue
UNK) is refused by pdb2gmx as it stands and accepted once renamed, by the Perl
function (the launch check) and by the awk program (the job script), which
must write the same text.  Atoms that must stay alone: a residue the user
named, a residue with two atoms, an element the force field has no ion for.

    tests/gromacs_md/gmx_ions_test.py [-v]

Exit status 77 (CTest SKIP) without gmx.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import run_tests as rt  # noqa: E402
from run_tests import check, PARSERS, run  # noqa: E402

PERL = ("use lib '%s'; use GmxIons; local $/; my $t = <STDIN>; "
        "my ($o) = GmxIons::rewrite($t, $ARGV[0]); print $o;" % PARSERS)


def pdb(res, atom, el, resno=1, serial=1):
    return ("HETATM%5d %-4s %3s %s%4d    %8.3f%8.3f%8.3f  1.00  0.00          %2s\n"
            % (serial, atom, res, " ", resno, 0.0, 0.0, 0.0, el))


def perl(text, ff):
    return subprocess.run(["perl", "-e", PERL, ff], input=text.encode(),
                          stdout=subprocess.PIPE).stdout.decode()


def awk(text, ff, awkfile):
    return subprocess.run(["awk", "-v", "ff=" + ff, "-f", awkfile],
                          input=text.encode(), stdout=subprocess.PIPE,
                          stderr=subprocess.DEVNULL).stdout.decode()


def pdb2gmx(gmx, ff, text, tmp):
    d = tempfile.mkdtemp(dir=tmp)
    with open(os.path.join(d, "i.pdb"), "w") as h:
        h.write(text)
    rc, out = run([gmx, "pdb2gmx", "-f", "i.pdb", "-ff", ff, "-water", "tip3p",
                   "-o", "o.gro", "-p", "o.top"], cwd=d, stdin=subprocess.DEVNULL)
    return rc == 0


def main():
    rt.verbose = "-v" in sys.argv
    gmx = rt.find_gmx()
    if gmx is None:
        print("SKIP: gmx is not installed")
        return 77
    out = subprocess.run([gmx, "--version"], stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT).stdout.decode()
    top = None
    for line in out.splitlines():
        if line.startswith("Data prefix:"):
            top = os.path.join(line.split(":", 1)[1].strip(), "share", "gromacs", "top")
    if not top or not os.path.isdir(top):
        print("SKIP: no gromacs top directory")
        return 77
    tmp = tempfile.mkdtemp(prefix="gmx-ions-")
    try:
        awkfile = os.path.join(tmp, "ions.awk")
        with open(awkfile, "w") as h:
            h.write(subprocess.run(["perl", "-I" + PARSERS, "-MGmxIons", "-e",
                                    "print GmxIons::awkProgram()"],
                                   stdout=subprocess.PIPE).stdout.decode())
        ions = subprocess.run(
            ["perl", "-I" + PARSERS, "-MGmxIons", "-e",
             'for my $f (@GmxIons::FAMILIES) { print join(" ", $f->{re}, '
             'map { "$_=$f->{ions}{$_}" } sort keys %{$f->{ions}}), "\\n" }'],
            stdout=subprocess.PIPE).stdout.decode()
        n = 0
        for ffdir in sorted(d for d in os.listdir(top) if d.endswith(".ff")):
            ff = ffdir[:-3]
            table = None
            for line in ions.splitlines():
                re_, *items = line.split()
                import re
                if re.search(re_, ff):
                    table = dict(i.split("=") for i in items)
                    break
            if table is None:
                continue
            for el, name in sorted(table.items()):
                text = pdb("UNK", el + "1", el)
                p, a = perl(text, ff), awk(text, ff, awkfile)
                check(p == a, "%s %s: Perl and awk write the same text" % (ff, el))
                check(not pdb2gmx(gmx, ff, text, tmp),
                      "%s %s: pdb2gmx refuses the UNK ion (the premise)" % (ff, el))
                check(pdb2gmx(gmx, ff, p, tmp),
                      "%s %s: pdb2gmx accepts it as %s" % (ff, el, name))
                n += 1
        ff = "amber99sb-ildn"
        keep = [
            ("a residue the user named", pdb("XYZ", "NA1", "NA")),
            ("two atoms in the residue", pdb("UNK", "NA1", "NA", 1, 1) + pdb("UNK", "O", "O", 1, 2)),
            ("no ion of that element", pdb("UNK", "XE1", "XE")),
            ("a carbon", pdb("UNK", "C1", "C")),
        ]
        for what, text in keep:
            check(perl(text, ff) == text and awk(text, ff, awkfile) == text,
                  "unchanged: %s" % what)
        # the Residue Table edit: residue typed as the ion's name, Builder atom name NA1
        named = pdb("NA", "NA1", "NA")
        check(perl(named, ff) == awk(named, ff, awkfile) and pdb2gmx(gmx, ff, perl(named, ff), tmp)
              and not pdb2gmx(gmx, ff, named, tmp),
              "a residue already named NA gets the atom name pdb2gmx wants")
        two = pdb("UNK", "NA1", "NA", 1, 1) + pdb("UNK", "CL1", "CL", 2, 2)
        check(perl(two, ff) == awk(two, ff, awkfile) and perl(two, ff).count(" NA ") >= 1
              and " CL " in perl(two, ff) and "UNK" not in perl(two, ff),
              "two lone ions in one file are both renamed")
        check(perl(pdb("", "NA1", "NA"), ff) == awk(pdb("", "NA1", "NA"), ff, awkfile)
              and "UNK" not in perl(pdb("", "NA1", "NA"), ff) and " NA " in perl(pdb("", "NA1", "NA"), ff),
              "an empty residue name is renamed too")
        print("%d ion/force-field pairs checked against pdb2gmx" % n)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    if rt.failures:
        print("FAILED: %d" % len(rt.failures))
        return 1
    print("lone ion names: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
