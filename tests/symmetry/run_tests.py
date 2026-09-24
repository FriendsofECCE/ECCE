#!/usr/bin/env python3
"""Are the character tables right?

    tests/symmetry/run_tests.py
    tests/symmetry/run_tests.py -v

A character table is a hand-entered grid of small integers, which is
exactly the kind of data a transcription slip survives in: a wrong sign
in one cell produces a table that still looks like a character table and
silently misassigns one irrep.

It does not have to be checked by eye.  The great orthogonality theorem
makes the data self-verifying, and this enforces all of it:

  * the class counts sum to the group order
  * the squared dimensions sum to the group order
  * every irrep's own norm equals the group order
  * distinct irreps are orthogonal
  * distinct classes are orthogonal, with the right normalisation
  * there are as many irreps as classes
  * (x, y, z) reduces to the irreps the table says it should

Together those pin every cell.  The last one is not redundant:
orthogonality CANNOT catch two rows being swapped, because a relabelling
leaves every orthogonality relation intact -- the table stays perfectly
self-consistent while two irreps wear each other's names.  Reducing the
Cartesian representation, whose characters follow from the class names
alone, ties each label to something physical and closes that gap.  Both
were self-tested by introducing the error and watching it fail.  A table that passes is correct, not
merely plausible.

Also checks that the irrep names match data/client/config/PointGroups
for the same group, since the whole point of the labels is to be
compared against the ones the codes themselves report in ORBSYM.

Exit 0 if every table is sound, 1 otherwise.
"""

import argparse
import math
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
CONFIG = os.path.join(ROOT, "data", "client", "config")


def readTables(path):
    """Parse the character table file into {group: (h, classes, counts, irreps)}."""
    groups = {}
    name = None
    h = 0
    classes = counts = None
    translations = ""
    irreps = []

    for raw in open(path):
        line = raw.split("#")[0].strip()
        if not line:
            continue

        if line.startswith("["):
            if name:
                groups[name] = (h, classes, counts, irreps, translations)
            close = line.index("]")
            name = line[1:close]
            h = int(line[close + 1:].strip().split("=")[1])
            classes = counts = None
            translations = ""
            irreps = []
            continue

        key, _, rest = line.partition(":")
        key = key.strip()
        fields = rest.split()

        if key == "classes":
            classes = fields
        elif key == "counts":
            counts = [int(f) for f in fields]
        elif key == "translations":
            translations = rest.strip()
        else:
            irreps.append((key, [int(f) for f in fields]))

    if name:
        groups[name] = (h, classes, counts, irreps, translations)
    return groups


def readIrrepNames(path):
    """The irrep names PointGroups already lists, by group."""
    names = {}
    for raw in open(path):
        line = raw.split("#")[0].strip()
        if not line or ":" not in line:
            continue
        key, _, rest = line.partition(":")
        names[key.strip().upper()] = rest.split()
    return names


class Report(object):
    def __init__(self, verbose):
        self.verbose = verbose
        self.failures = 0
        self.checks = 0

    def check(self, ok, what):
        self.checks += 1
        if not ok:
            self.failures += 1
            print("    FAIL  %s" % what)
        elif self.verbose:
            print("    ok    %s" % what)


def cartesianCharacter(className):
    """Character of the 3-D Cartesian representation for one class.

    A proper rotation through theta contributes 1 + 2cos(theta), an
    improper one -1 + 2cos(theta), a mirror 1 and inversion -3.  So this
    needs nothing but the class NAME, which is what makes it an
    independent check on the table rather than a restatement of it.
    """
    n = className.split("_")[0].rstrip("'").strip()
    if n.upper() == "E":
        return 3.0
    if n.lower() == "i":
        return -3.0
    #  A mirror: sigma, written sh/sv/sd here.  Distinguished from S4/S6
    #  by not having a digit after the letter.
    if n[0].lower() == "s" and not n[1:2].isdigit():
        return 1.0
    order = int("".join(c for c in n if c.isdigit()) or 2)
    angle = 2.0*math.pi/order
    if n[0].upper() == "C":
        return 1.0 + 2.0*math.cos(angle)
    if n[0].upper() == "S":
        return -1.0 + 2.0*math.cos(angle)
    raise ValueError("unrecognised class name %r" % className)


def formatReduction(parts):
    return " + ".join(label if n == 1 else "%d%s" % (n, label)
                      for label, n in parts)


def checkTranslations(name, h, classes, counts, irreps, expected, report):
    """Does (x, y, z) span what the table claims?"""
    if not expected:
        report.check(False, "%s has no translations line" % name)
        return
    try:
        chi = [cartesianCharacter(c) for c in classes]
    except ValueError as exc:
        report.check(False, "%s: %s" % (name, exc))
        return

    parts = []
    dimension = 0
    for label, row in irreps:
        n = sum(counts[i]*row[i]*chi[i] for i in range(len(classes)))/float(h)
        if abs(n - round(n)) > 1e-9 or round(n) < 0:
            report.check(False,
                         "%s/%s: (x,y,z) reduces to a non-integer or negative "
                         "multiplicity %g -- the table is not a character "
                         "table" % (name, label, n))
            return
        n = int(round(n))
        if n:
            parts.append((label, n))
            dimension += n*row[0]

    report.check(dimension == 3,
                 "%s: (x,y,z) reduces to dimension 3 (got %d)"
                 % (name, dimension))
    got = formatReduction(parts)
    report.check(got == expected,
                 "%s: (x,y,z) spans %s, table says %s"
                 % (name, got, expected))


def checkGroup(name, h, classes, counts, irreps, report):
    nclass = len(classes)

    report.check(counts is not None and classes is not None,
                 "%s has both a classes and a counts line" % name)
    if counts is None or classes is None:
        return
    report.check(len(counts) == nclass,
                 "%s: %d counts for %d classes" % (name, len(counts), nclass))

    #  As many irreps as classes -- a basic theorem, and the check that
    #  catches a whole row being left out.
    report.check(len(irreps) == nclass,
                 "%s: %d irreps for %d classes (must be equal)"
                 % (name, len(irreps), nclass))

    report.check(sum(counts) == h,
                 "%s: class counts sum to h (%d vs %d)"
                 % (name, sum(counts), h))

    for label, chi in irreps:
        report.check(len(chi) == nclass,
                     "%s/%s: %d characters for %d classes"
                     % (name, label, len(chi), nclass))

    if any(len(chi) != nclass for _, chi in irreps):
        return

    #  The dimension of an irrep is its character under the identity,
    #  which must be the first class.
    report.check(classes[0].upper() == "E" and counts[0] == 1,
                 "%s: first class is the identity" % name)
    dims = [chi[0] for _, chi in irreps]
    report.check(sum(d * d for d in dims) == h,
                 "%s: squared dimensions sum to h (%d vs %d)"
                 % (name, sum(d * d for d in dims), h))

    #  Row orthogonality, including each row's own norm.
    for i, (li, ci) in enumerate(irreps):
        for j, (lj, cj) in enumerate(irreps):
            total = sum(counts[c] * ci[c] * cj[c] for c in range(nclass))
            want = h if i == j else 0
            report.check(total == want,
                         "%s: <%s|%s> = %d, want %d"
                         % (name, li, lj, total, want))

    #  Column orthogonality: sum over IRREPS this time, which is an
    #  independent constraint and catches errors row orthogonality can
    #  miss when two rows are swapped.
    for a in range(nclass):
        for b in range(nclass):
            total = sum(chi[a] * chi[b] for _, chi in irreps)
            want = (h // counts[a]) if a == b else 0
            report.check(total == want,
                         "%s: columns %s.%s = %d, want %d"
                         % (name, classes[a], classes[b], total, want))


def checkLoader(tablePath, verbose):
    """Does the C++ loader read the same file faithfully?

    run_tests.py proves the DATA is sound.  A parser that drops a row or
    goes off by one between classes and counts would leave a table that
    is still internally consistent and simply describes a different
    group, so the loader is checked separately -- against the same
    oracle, what (x,y,z) spans, which is computed from the class names
    and so does not come from the numbers being read.

    Compiled with plain g++: CharacterTable deliberately has no ECCE
    runtime dependency in its parsing, so this needs no build tree.
    """
    out = os.path.join(HERE, "testCharacterTable")
    cmd = ["g++", "-O2", "-w", "-I", os.path.join(ROOT, "include"),
           "-o", out,
           os.path.join(HERE, "testCharacterTable.C"),
           os.path.join(ROOT, "src/tdat/chemistry/CharacterTable.C")]

    build = subprocess.run(cmd, capture_output=True, text=True)
    if build.returncode != 0:
        print("  could not build the C++ loader test:")
        print(build.stderr)
        return 1

    run = subprocess.run([out, tablePath], capture_output=True, text=True)
    if verbose or run.returncode != 0:
        print(run.stdout, end="")
        if run.stderr:
            print(run.stderr, end="")
    elif run.returncode == 0:
        print("  C++ loader: PASS")
    os.unlink(out)
    return run.returncode


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    tablePath = os.path.join(CONFIG, "CharacterTables")
    if not os.path.exists(tablePath):
        print("no CharacterTables at %s" % tablePath)
        return 1

    groups = readTables(tablePath)
    report = Report(args.verbose)

    print("Character tables: %d groups\n" % len(groups))

    for name in sorted(groups):
        h, classes, counts, irreps, translations = groups[name]
        checkGroup(name, h, classes, counts, irreps, report)
        checkTranslations(name, h, classes, counts, irreps, translations,
                          report)

    #  The labels must match what PointGroups already says, because the
    #  codes' own ORBSYM labels are compared against those.
    namePath = os.path.join(CONFIG, "PointGroups")
    if os.path.exists(namePath):
        known = readIrrepNames(namePath)
        for name in sorted(groups):
            if name not in known:
                report.check(False,
                             "%s is not listed in PointGroups at all" % name)
                continue
            mine = set(label for label, _ in groups[name][3])
            theirs = set(known[name])
            report.check(mine == theirs,
                         "%s irrep names match PointGroups (only here: %s; "
                         "only there: %s)"
                         % (name, sorted(mine - theirs) or "-",
                            sorted(theirs - mine) or "-"))

    print("\n%d checks run" % report.checks)
    if report.failures:
        print("FAILED  %d" % report.failures)
        return 1

    print("")
    if checkLoader(tablePath, args.verbose) != 0:
        print("FAILED  the C++ loader")
        return 1

    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
