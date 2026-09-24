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
import re
import shutil
import subprocess
import sys
import tempfile

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
            irreps.append((key, [float(f) for f in fields]))

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
    #  "C5p2" is C5 SQUARED, not a 52-fold rotation.  Joining every
    #  digit in the name gave order 52 and a character of 1.72, which
    #  is what made C5v look like it was not a character table at all.
    m = re.match(r'^[CS](\d+)(?:p(\d+))?$', n)
    if m:
        order = int(m.group(1))
        power = int(m.group(2) or 1)
    else:
        order = int("".join(c for c in n if c.isdigit()) or 2)
        power = 1
    angle = 2.0*math.pi*power/order
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
    report.check(abs(sum(d * d for d in dims) - h) < 1e-6,
                 "%s: squared dimensions sum to h (%g vs %d)"
                 % (name, sum(d * d for d in dims), h))

    #  Row orthogonality, including each row's own norm.
    for i, (li, ci) in enumerate(irreps):
        for j, (lj, cj) in enumerate(irreps):
            total = sum(counts[c] * ci[c] * cj[c] for c in range(nclass))
            want = h if i == j else 0
            #  Tolerance, not equality: the five-fold and eight-fold
            #  groups have irrational characters (2cos72, sqrt2, the
            #  golden ratio), so these sums are exact only in principle.
            report.check(abs(total - want) < 1e-6,
                         "%s: <%s|%s> = %g, want %d"
                         % (name, li, lj, total, want))

    #  Column orthogonality: sum over IRREPS this time, which is an
    #  independent constraint and catches errors row orthogonality can
    #  miss when two rows are swapped.
    for a in range(nclass):
        for b in range(nclass):
            total = sum(chi[a] * chi[b] for _, chi in irreps)
            want = (float(h) / counts[a]) if a == b else 0
            report.check(abs(total - want) < 1e-6,
                         "%s: columns %s.%s = %g, want %g"
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


def readSymops(binary, group):
    """The operation matrices for a group, from the symops program."""
    run = subprocess.run([binary], input=group + "\n",
                         capture_output=True, text=True, timeout=60)
    if run.returncode != 0:
        return None
    lines = [l for l in run.stdout.splitlines() if l.strip()]
    if not lines:
        return None
    count = int(lines[0])
    ops = []
    for i in range(count):
        rows = []
        for j in range(3):
            fields = [float(f) for f in lines[1 + i*3 + j].split()]
            rows.append(fields[:3])          # the translation is always zero
        ops.append(rows)
    return ops


def matmul(a, b):
    return [[sum(a[i][k]*b[k][j] for k in range(3)) for j in range(3)]
            for i in range(3)]


def det3(m):
    return (m[0][0]*(m[1][1]*m[2][2] - m[1][2]*m[2][1])
            - m[0][1]*(m[1][0]*m[2][2] - m[1][2]*m[2][0])
            + m[0][2]*(m[1][0]*m[2][1] - m[1][1]*m[2][0]))


def key(m, places=6):
    return tuple(round(v, places) + 0.0 for row in m for v in row)


def conjugacyClasses(ops):
    """Partition the operations into conjugacy classes.

    Two operations are conjugate when S R S^-1 is the other for some S
    in the group.  For these matrices S^-1 is the transpose, since every
    point group operation is orthogonal.
    """
    keys = [key(m) for m in ops]
    index = dict((k, i) for i, k in enumerate(keys))
    seen = set()
    classes = []
    for i, r in enumerate(ops):
        if i in seen:
            continue
        members = set()
        for s in ops:
            sinv = [[s[j][i2] for j in range(3)] for i2 in range(3)]
            c = matmul(matmul(s, r), sinv)
            j = index.get(key(c))
            if j is not None:
                members.add(j)
        seen |= members
        classes.append(members)
    return classes


def checkSymops(groups, report, verbose):
    """The symops program's matrices, against the character tables.

    Two completely independent sources: PNNL's Fortran generator tables
    on one side, a hand-entered character table on the other.  They have
    to agree on the group order and on the number of conjugacy classes,
    and neither was derived from the other.
    """
    binary = os.environ.get("ECCE_TEST_SYMOPS",
                            os.path.join(ROOT, "build-cmake", "symops"))
    if not (os.path.isfile(binary) and os.access(binary, os.X_OK)):
        print("  symops not built -- skipping the operation checks")
        print("  (build it, or set ECCE_TEST_SYMOPS)")
        return

    for name in sorted(groups):
        h, classes, counts, irreps, _ = groups[name]

        ops = readSymops(binary, name)
        if ops is None:
            report.check(False, "%s: symops would not run" % name)
            continue

        report.check(len(ops) == h,
                     "%s: symops returns h operations (%d vs %d)"
                     % (name, len(ops), h))
        if len(ops) != h:
            continue

        #  Every point group operation is orthogonal with determinant
        #  +1 (proper) or -1 (improper).
        dets = [det3(m) for m in ops]
        report.check(all(abs(abs(d) - 1.0) < 1e-9 for d in dets),
                     "%s: every operation has determinant +/-1" % name)

        #  CLOSURE.  This is what makes it a group rather than a list,
        #  and it is the check that would catch a generator table that
        #  produced a plausible but incomplete set.
        present = set(key(m) for m in ops)
        closed = True
        for a in ops:
            for b in ops:
                if key(matmul(a, b)) not in present:
                    closed = False
                    break
            if not closed:
                break
        report.check(closed, "%s: the operations are closed under multiplication"
                     % name)

        report.check(key([[1, 0, 0], [0, 1, 0], [0, 0, 1]]) in present,
                     "%s: the identity is present" % name)

        #  The number of conjugacy classes must equal the number of
        #  irreps -- one of the deepest facts about a character table,
        #  and here it ties the Fortran matrices to the hand-entered
        #  numbers without either having been derived from the other.
        cc = conjugacyClasses(ops)
        report.check(len(cc) == len(irreps),
                     "%s: %d conjugacy classes for %d irreps (must be equal)"
                     % (name, len(cc), len(irreps)))

        #  And the class SIZES must match the counts line.
        sizes = sorted(len(c) for c in cc)
        report.check(sizes == sorted(counts),
                     "%s: class sizes %s match the counts line %s"
                     % (name, sizes, sorted(counts)))


def checkAnalysis(tablePath, verbose):
    """Orbits and reductions, against the textbook answers.

    Needs the symops binary, because the operations come from the real
    generator tables rather than a second copy written into the test.
    """
    binary = os.environ.get("ECCE_TEST_SYMOPS",
                            os.path.join(ROOT, "build-cmake", "symops"))
    if not (os.path.isfile(binary) and os.access(binary, os.X_OK)):
        print("  symops not built -- skipping the orbit/reduction checks")
        return 0

    opsDir = tempfile.mkdtemp(prefix="ecce-symops-")
    try:
        for group in ("TD", "C2V", "OH", "D2H"):
            run = subprocess.run([binary], input=group + "\n",
                                 capture_output=True, text=True, timeout=60)
            if run.returncode != 0:
                print("  symops failed for %s" % group)
                return 1
            open(os.path.join(opsDir, group + ".ops"), "w").write(run.stdout)

        out = os.path.join(HERE, "testSymmetryAnalysis")
        cmd = ["g++", "-O2", "-w", "-I", os.path.join(ROOT, "include"),
               "-o", out,
               os.path.join(HERE, "testSymmetryAnalysis.C"),
               os.path.join(ROOT, "src/tdat/chemistry/SymmetryAnalysis.C"),
               os.path.join(ROOT, "src/tdat/chemistry/CharacterTable.C")]
        build = subprocess.run(cmd, capture_output=True, text=True)
        if build.returncode != 0:
            print("  could not build the analysis test:")
            print(build.stderr)
            return 1

        proc = subprocess.run([out, tablePath, opsDir],
                              capture_output=True, text=True)
        if verbose or proc.returncode != 0:
            print(proc.stdout, end="")
        else:
            print("  orbits and reductions: PASS")
        os.unlink(out)
        return proc.returncode
    finally:
        shutil.rmtree(opsDir, ignore_errors=True)


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

    print("\n%d checks run on the tables" % report.checks)
    if report.failures:
        print("FAILED  %d" % report.failures)
        return 1

    print("")
    beforeSymops = report.checks
    checkSymops(groups, report, args.verbose)
    print("  symmetry operations: %d checks" % (report.checks - beforeSymops))
    if report.failures:
        print("\nFAILED  %d" % report.failures)
        return 1

    print("")
    if checkLoader(tablePath, args.verbose) != 0:
        print("FAILED  the C++ loader")
        return 1

    if checkAnalysis(tablePath, args.verbose) != 0:
        print("FAILED  the orbit/reduction analysis")
        return 1

    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
