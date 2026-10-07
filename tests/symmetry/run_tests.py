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


def standalone(name, sources, args=()):
    """Compile and run one C++ test that needs no build tree."""
    out = os.path.join(HERE, name)
    cmd = (["g++", "-O2", "-w", "-I", os.path.join(ROOT, "include"),
            "-o", out, os.path.join(HERE, name + ".C")]
           + [os.path.join(ROOT, s) for s in sources])
    build = subprocess.run(cmd, capture_output=True, text=True)
    if build.returncode != 0:
        print("  could not build %s:" % name)
        print(build.stderr)
        return 1
    run = subprocess.run([out] + list(args), capture_output=True, text=True)
    print(run.stdout, end="")
    if run.stderr:
        print(run.stderr, end="")
    os.unlink(out)
    return run.returncode


def checkLinearGeneratorGroups(verbose):
    """A linear molecule's deck must name a finite point group.

    The job parser writes the code's own report of the group back onto
    the molecule, so a calculation copied from a finished Gaussian job
    arrives with "C*V" -- and Gaussian rejects PG=C*V on its own route
    card (QPErr, then l1 segfaults).  Runs the real generators on CO and
    CO2 with each spelling the codes print.
    """
    import shutil
    parsers = os.path.join(ROOT, "scripts", "parsers")
    cases = [("C*V", "C4V"), ("C(inf)v", "C4V"), ("Cinfv", "C4V"),
             ("D*H", "D4H"), ("D(inf)h", "D4H"), ("C2v", "C2V")]
    failures = 0
    for gen, tpl in (("ai.gauss16", "g16.tpl"), ("ai.gauss09", "g09.tpl")):
        for group, want in cases:
            work = tempfile.mkdtemp(prefix="linear-")
            try:
                with open(os.path.join(work, "co.param"), "w") as f:
                    f.write("Category: SCF\nTheory: RHF\nRunType: Energy\n"
                            "Charge: 0\nSymmetry: %s\n"
                            "ES.Theory.UseSymmetry: 1\n" % group)
                with open(os.path.join(work, "co.frag"), "w") as f:
                    f.write("# frag\ntitle: CO\nnum_atoms: 2\n"
                            "atom_info: symbol x y z\natom_list:\n"
                            "C 0.0 0.0 -0.564\nO 0.0 0.0 0.564\n")
                with open(os.path.join(work, "co.basis"), "w") as f:
                    f.write("useRouteCard sto-3g false\n")
                #  A copy: the generator overwrites the template it is given.
                deck = os.path.join(work, "deck")
                shutil.copy(os.path.join(parsers, tpl), deck)
                env = dict(os.environ, ECCE_HOME=ROOT)
                subprocess.run(["perl", os.path.join(parsers, gen), "-n", "co",
                                "-p", "-f", "-b", "-t", deck], cwd=work,
                               env=env, capture_output=True, text=True,
                               timeout=60)
                route = open(deck).readline()
            finally:
                shutil.rmtree(work, ignore_errors=True)
            ok = ("PG=%s," % want) in route
            if not ok or verbose:
                print("  %s %-5s %-8s -> %s" % ("ok  " if ok else "FAIL",
                                              gen[3:], group, route.strip()))
            failures += 0 if ok else 1
    print("  linear point groups in generated decks: %s"
          % ("FAIL" if failures else "PASS"))
    return 1 if failures else 0


def checkUseSymmetryRule(verbose):
    """"Use symmetry" ticked lets the code use symmetry; unticked forbids it.

    Runs the real generators over a water .param as CalcEd writes it.
    Ticked: no NoSymm/noautosym, and Gaussian names the group (PG=..,Loose)
    only when it is not C1.  Unticked: NoSymm (Gaussian), noautosym
    (NWChem), no UseSym (ORCA).  The editor must always write the key; what
    the generators do without it is not this rule.
    """
    import shutil
    parsers = os.path.join(ROOT, "scripts", "parsers")
    gauss = "useRouteCard sto-3g false\n"
    nwbasis = ('basis "ao basis" cartesian print\n  H library "sto-3g"\n'
               '  O library "sto-3g"\nEND\n')
    gens = {"ai.gauss16": ("g16.tpl", gauss), "ai.gauss09": ("g09.tpl", gauss),
            "ai.nwchem": ("nwch.tpl", nwbasis), "ai.orca": ("orca.tpl", "")}

    def expect(gen, use, group):
        if gen.startswith("ai.gauss"):
            if use == "0":
                return lambda t: "NoSymm" in t and "PG=" not in t
            if group == "C1":
                return lambda t: "NoSymm" not in t and "PG=" not in t
            return lambda t: "NoSymm" not in t and "PG=C2V,Loose" in t
        if gen == "ai.nwchem":
            want = " noautosym " if use == "0" else " autosym "
            return lambda t: re.search(r"^geometry.*%s" % want, t, re.M)
        return lambda t: ("UseSym" in t) == (use == "1")

    failures = 0
    for gen, (tpl, basis) in sorted(gens.items()):
        for use, group in (("1", "C1"), ("1", "C2v"), ("0", "C1"),
                           ("0", "C2v")):
            work = tempfile.mkdtemp(prefix="usesym-")
            try:
                with open(os.path.join(work, "w.param"), "w") as f:
                    f.write("Category: SCF\nTheory: RHF\nRunType: Energy\n"
                            "Charge: 0\nSymmetry: %s\nNumElectrons: 10\n"
                            "ChemSys.Multiplicity: 1\n"
                            "ES.Theory.UseSymmetry: %s\n" % (group, use))
                with open(os.path.join(work, "w.frag"), "w") as f:
                    f.write("title: w\ntype: molecule\nnum_atoms: 3\n"
                            "atom_info: symbol cart\natom_list:\n"
                            "O 0 0 0.117\nH 0 0.757 -0.469\n"
                            "H 0 -0.757 -0.469\n")
                with open(os.path.join(work, "w.basis"), "w") as f:
                    f.write(basis)
                deck = os.path.join(work, "deck")
                shutil.copy(os.path.join(parsers, tpl), deck)
                env = dict(os.environ, ECCE_HOME=ROOT)
                subprocess.run(["perl", os.path.join(parsers, gen), "-n", "w",
                                "-p", "-f", "-b", "-t", deck], cwd=work,
                               env=env, capture_output=True, text=True,
                               timeout=60)
                text = open(deck).read()
            finally:
                shutil.rmtree(work, ignore_errors=True)
            line = [l for l in text.splitlines()
                    if re.match(r"\s*(#|geometry|!)", l)]
            ok = bool(expect(gen, use, group)(text))
            if not ok or verbose:
                print("  %s %-10s use=%s %-4s -> %s"
                      % ("ok  " if ok else "FAIL", gen, use, group,
                         " / ".join(l.strip() for l in line)))
            failures += 0 if ok else 1
    print("  Use symmetry in generated decks: %s"
          % ("FAIL" if failures else "PASS"))
    return 1 if failures else 0


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
               os.path.join(ROOT, "src/tdat/chemistry/ShellRotation.C"),
               os.path.join(ROOT, "src/tdat/chemistry/TGBSAngFunc.C"),
               os.path.join(ROOT, "src/tdat/chemistry/BasisAngularNorm.C"),
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


def checkValenceEnergies(report):
    """The valence orbital ionisation energies, by their trends.

    These are hand-entered numbers whose third significant figure is
    not what matters, so checking them against a second copy of the
    same table would prove nothing.  What a correlation diagram
    actually needs from them is the ORDERING, and the ordering follows
    two rules that no transcription slip respects:

      * ns lies below np on the same atom, always;
      * across a period both become more negative left to right, as the
        nuclear charge rises.

    A dropped minus sign, a transposed pair of digits or a row entered
    against the wrong element breaks one of those.
    """
    path = os.path.join(CONFIG, "ValenceOrbitalEnergies")
    if not os.path.exists(path):
        report.check(False, "no ValenceOrbitalEnergies at %s" % path)
        return

    TRANSITION = ("Sc", "Ti", "V", "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn")

    order = {}
    dMetals = []
    for line in open(path):
        line = line.split("#")[0].split()
        if len(line) < 3:
            continue
        symbol, n = line[0], int(line[1])
        sEnergy = float(line[2])
        pEnergy = None
        if len(line) > 3 and line[3] != "-":
            pEnergy = float(line[3])

        report.check(sEnergy < 0.0, "%s: s energy is negative" % symbol)
        if pEnergy is not None:
            report.check(pEnergy < 0.0, "%s: p energy is negative" % symbol)
            report.check(sEnergy < pEnergy,
                         "%s: %ds (%.1f) lies below %dp (%.1f)"
                         % (symbol, n, sEnergy, n, pEnergy))
        #  The two optional columns at the end are a transition
        #  metal's d shell: its own principal number and its energy.
        if len(line) > 6 and line[5] != "-":
            dEnergy = float(line[6])
            report.check(dEnergy < 0.0, "%s: d energy is negative" % symbol)
            report.check(int(line[5]) == n - 1,
                         "%s: the d shell is one below the s and p" % symbol)
            if pEnergy is not None:
                dMetals.append((symbol, sEnergy, pEnergy, dEnergy))

        order.setdefault(n, []).append((symbol, sEnergy, pEnergy))

    #  Within a period, in file order, both levels fall.  Compared
    #  pairwise between neighbours rather than end to end, so the check
    #  names the pair that breaks it.
    #
    #  MAIN GROUP ONLY.  The rule is a main-group rule: it is the
    #  increasing nuclear charge felt by an electron in the shell
    #  being filled.  Across the transition series the electrons go
    #  into the shell BELOW, so the 4s falls only gently and the 4p
    #  barely moves at all -- cobalt and nickel are both -3.8 to the
    #  precision anyone quotes.  Applying the main-group rule to them
    #  does not find a transcription slip, it finds chemistry.
    for n in sorted(order):
        row = [entry for entry in order[n] if entry[0] not in TRANSITION]
        for i in range(1, len(row)):
            prev, cur = row[i-1], row[i]
            report.check(cur[1] < prev[1],
                         "period %d: %s s (%.1f) below %s s (%.1f)"
                         % (n, cur[0], cur[1], prev[0], prev[1]))
            if prev[2] is not None and cur[2] is not None:
                report.check(cur[2] < prev[2],
                             "period %d: %s p (%.1f) below %s p (%.1f)"
                             % (n, cur[0], cur[2], prev[0], prev[2]))

    #  The transition series has trends of its own, and they are the
    #  ones a complex's diagram depends on.
    metals = [entry for entry in dMetals if entry[0] in TRANSITION]
    for symbol, s4, p4, d3 in metals:
        report.check(s4 < p4,
                     "%s: 4s (%.1f) lies below 4p (%.1f)" % (symbol, s4, p4))

    #  THE 3d/4s CROSSOVER, WHICH IS THE POINT OF THE SERIES.
    #
    #  A first draft of this check asserted 3d below 4s for every
    #  metal and failed on scandium, titanium and vanadium -- which is
    #  not a transcription slip but the crossover itself: the 3d
    #  starts ABOVE the 4s at the left of the series, falls much
    #  faster as the nuclear charge climbs, and ends far below it.
    #  That is why scandium behaves like a main-group metal and why
    #  the late metals do not.  So the trend is the check, not a fixed
    #  ordering.
    if metals:
        report.check(metals[0][3] > metals[0][1],
                     "%s: 3d (%.1f) starts above 4s (%.1f)"
                     % (metals[0][0], metals[0][3], metals[0][1]))
        report.check(metals[-1][3] < metals[-1][1],
                     "%s: 3d (%.1f) ends below 4s (%.1f)"
                     % (metals[-1][0], metals[-1][3], metals[-1][1]))
    for i in range(1, len(metals)):
        prev, cur = metals[i-1], metals[i]
        report.check((cur[3] - cur[1]) < (prev[3] - prev[1]),
                     "3d falls faster than 4s: %s to %s" % (prev[0], cur[0]))
    for i in range(1, len(metals)):
        prev, cur = metals[i-1], metals[i]
        report.check(cur[3] < prev[3],
                     "3d falls: %s (%.1f) below %s (%.1f)"
                     % (cur[0], cur[3], prev[0], prev[3]))
        report.check(cur[1] <= prev[1],
                     "4s does not rise: %s (%.1f) at or below %s (%.1f)"
                     % (cur[0], cur[1], prev[0], prev[1]))


def checkAutosymThreshold(report):
    """The symmetry-search threshold, against the real autosym.

    It does not behave the way it reads: a LOOSER threshold finds LOWER
    symmetry, not higher.  0.05 A -- a perfectly reasonable-looking
    choice, and the one the MO diagram was first written with -- costs
    water its C2 axis and builds the whole correlation diagram in Cs
    with no error anywhere.

    Pinned here because the value lives in two places now (the Symmetry
    panel's default field and MoDiagramPanel) and because a future
    autosym change that shifted this would be invisible otherwise.
    """
    binary = os.path.join(ROOT, "build-cmake", "autosym")
    if not (os.path.isfile(binary) and os.access(binary, os.X_OK)):
        binary = "/opt/ecce/bin/autosym"
    if not (os.path.isfile(binary) and os.access(binary, os.X_OK)):
        print("  autosym not available -- skipping the threshold check")
        return

    WATER = [("O", 8, 0.0, 0.0, 0.1173),
             ("H", 1, 0.0, 0.7572, -0.4692),
             ("H", 1, 0.0, -0.7572, -0.4692)]
    d = 0.6276
    METHANE = [("C", 6, 0.0, 0.0, 0.0),
               ("H", 1, d, d, d), ("H", 1, d, -d, -d),
               ("H", 1, -d, d, -d), ("H", 1, -d, -d, d)]

    def detect(atoms, threshold):
        lines = ["%d" % len(atoms), "%g" % threshold]
        for symbol, z, x, y, zz in atoms:
            lines.append("%-16s" % symbol)
            lines.append("%d %G %G %G" % (z, x, y, zz))
        run = subprocess.run([binary], input="\n".join(lines) + "\n",
                             capture_output=True, text=True, timeout=60)
        if run.returncode != 0 or not run.stdout.strip():
            return None
        return run.stdout.split("\n")[0].strip()

    #  The value both callers use.
    report.check(detect(WATER, 0.01) == "C2V",
                 "autosym finds C2V for water at 0.01 A (got %s)"
                 % detect(WATER, 0.01))
    report.check(detect(METHANE, 0.01) == "TD",
                 "autosym finds TD for methane at 0.01 A (got %s)"
                 % detect(METHANE, 0.01))

    #  And the trap itself, asserted so that anyone who loosens the
    #  threshold sees why they should not.
    report.check(detect(WATER, 0.05) == "CS",
                 "a looser 0.05 A threshold gives water only CS -- the "
                 "threshold works backwards, do not raise it (got %s)"
                 % detect(WATER, 0.05))



def checkFragments(tablePath, verbose):
    """The two outer columns, against the textbook answers for CH4 and H2O."""
    binary = os.environ.get("ECCE_TEST_SYMOPS",
                            os.path.join(ROOT, "build-cmake", "symops"))
    if not (os.path.isfile(binary) and os.access(binary, os.X_OK)):
        print("  symops not built -- skipping the fragment-column checks")
        return 0

    out = os.path.join(HERE, "testMoFragments")
    cmd = ["g++", "-O2", "-w", "-I", os.path.join(ROOT, "include"),
           "-o", out,
           os.path.join(HERE, "testMoFragments.C"),
           os.path.join(ROOT, "src/tdat/chemistry/MoFragments.C"),
           os.path.join(ROOT, "src/tdat/chemistry/MoComposition.C"),
           os.path.join(ROOT, "src/tdat/chemistry/SymmetryAnalysis.C"),
           os.path.join(ROOT, "src/tdat/chemistry/ShellRotation.C"),
           os.path.join(ROOT, "src/tdat/chemistry/TGBSAngFunc.C"),
           os.path.join(ROOT, "src/tdat/chemistry/BasisAngularNorm.C"),
           os.path.join(ROOT, "src/tdat/chemistry/CharacterTable.C"),
           os.path.join(ROOT, "src/tdat/chemistry/MoDiagram.C"),
         os.path.join(ROOT, "src/tdat/chemistry/Huckel.C")]
    build = subprocess.run(cmd, capture_output=True, text=True)
    if build.returncode != 0:
        print("  could not build the fragment test:")
        print(build.stderr)
        return 1

    env = dict(os.environ)
    env["ECCE_HOME"] = ROOT
    env["PATH"] = os.path.dirname(binary) + os.pathsep + env.get("PATH", "")
    proc = subprocess.run([out, tablePath], capture_output=True, text=True,
                          env=env)
    if verbose or proc.returncode != 0:
        print(proc.stdout, end="")
    else:
        print("  fragment columns: PASS")
    os.unlink(out)
    return proc.returncode



def checkShellRotation(verbose):
    """ShellRotation::buildD() against ECCE's real angular tables (#151).

    Needs a configured CMake build tree (JCode/CodeFactory/XML), unlike
    most of this file's checks -- see testShellRotation.C for why:
    the whole point is the table ECCE actually SHIPS (Gaussian-16's
    MOOrdering), not one invented for the test.
    """
    build = os.environ.get("ECCE_TEST_BUILD", os.path.join(ROOT, "build-cmake"))
    if not os.path.isdir(build):
        print("  skipped: no build tree at %s (set ECCE_TEST_BUILD)" % build)
        return 0

    libs = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
            "ecceutil", "eccecomm", "eccecipc", "eccercmd"]
    out = os.path.join(HERE, "testShellRotation")
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(ROOT, "include"),
            "-o", out,
            os.path.join(HERE, "testShellRotation.C"),
            os.path.join(ROOT, "src/tdat/chemistry/ShellRotation.C"),
            os.path.join(ROOT, "src/tdat/chemistry/SymmetryAnalysis.C"),
            os.path.join(ROOT, "src/tdat/chemistry/CharacterTable.C"),
            os.path.join(ROOT, "src/tdat/chemistry/BasisFlatten.C"),
            "-L" + build]
           + ["-l" + l for l in libs]*3 + ["-lxerces-c", "-lmosquitto", "-lssl", "-lcrypto"])
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        print("  could not build testShellRotation:")
        print(proc.stderr)
        return 1

    env = dict(os.environ)
    env["ECCE_HOME"] = ROOT
    env.setdefault("ECCE_REALUSERHOME", os.path.expanduser("~"))
    run = subprocess.run([out], capture_output=True, text=True, env=env)
    print(run.stdout, end="")
    if run.stderr:
        print(run.stderr, end="")
    os.unlink(out)
    return run.returncode


def checkFullOrbitalIrrep(verbose):
    """The full-point-group ORBSYM oracle (#147/#132) against a REAL
    Gaussian-16 calculation, orbital by orbital.

    Needs a build tree (JCode/XML) and the real symops binary -- see
    testFullOrbitalIrrep.C's header comment for exactly what is real
    here (a real fort.7 MO punch file, run through ECCE's own
    gaussian-16.mo/gaussian-16.orbocc parsers) and what fixtures/g16mo/
    generate.py documents about how the checked-in fixture was made.
    """
    build = os.environ.get("ECCE_TEST_BUILD", os.path.join(ROOT, "build-cmake"))
    if not os.path.isdir(build):
        print("  skipped: no build tree at %s (set ECCE_TEST_BUILD)" % build)
        return 0
    symops = os.environ.get("ECCE_TEST_SYMOPS", os.path.join(ROOT, "build-cmake", "symops"))
    if not (os.path.isfile(symops) and os.access(symops, os.X_OK)):
        print("  symops not built -- skipping the full-orbital-irrep oracle")
        return 0

    libs = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
            "ecceutil", "eccecomm", "eccecipc", "eccercmd"]
    out = os.path.join(HERE, "testFullOrbitalIrrep")
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(ROOT, "include"),
            "-o", out,
            os.path.join(HERE, "testFullOrbitalIrrep.C"),
            os.path.join(ROOT, "src/tdat/chemistry/ShellRotation.C"),
            os.path.join(ROOT, "src/tdat/chemistry/SymmetryAnalysis.C"),
            os.path.join(ROOT, "src/tdat/chemistry/CharacterTable.C"),
            os.path.join(ROOT, "src/tdat/chemistry/BasisFlatten.C"),
            "-L" + build]
           + ["-l" + l for l in libs]*3 + ["-lxerces-c", "-lmosquitto", "-lssl", "-lcrypto"])
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        print("  could not build testFullOrbitalIrrep:")
        print(proc.stderr)
        return 1

    env = dict(os.environ)
    env["ECCE_HOME"] = ROOT
    env.setdefault("ECCE_REALUSERHOME", os.path.expanduser("~"))
    env["ECCE_TEST_SYMOPS"] = symops

    #  (fixture, natoms, nbasis, minLabelled) -- CH4/Td (s+p+d,
    #  cartesian) and benzene/D6h (#151's live case: the degenerate
    #  e1g/e2u sets ORCA's D2h-subgroup labelling cannot reach at all).
    #
    #  minLabelled is a REGRESSION FLOOR, not a target: it is exactly
    #  what fullLabelSpectrum() labels today (Cr(CO)6 fix, #132
    #  follow-up -- connected-components degenerate grouping at 1e-5
    #  Ha instead of a same-anchor chain at 1e-3). Before that fix,
    #  SF6 labelled only 91 of 109 (t1g/t2g/eg sets close enough in
    #  energy to merge across irreps) and benzene 88 of 102; if this
    #  ever drops below the floor, the grouping tolerance has been
    #  loosened back and Cr(CO)6-shaped molecules will silently lose
    #  their labels again, the same failure mode that motivated the
    #  fix and left ~68 of that molecule's orbitals as bare numbers.
    rc = 0
    for fixture, natoms, nbasis, minLabelled in (
            ("ch4-td.txt", 5, 23, 23),
            ("c6h6-d6h.txt", 12, 102, 94),
            ("sf6-oh.txt", 7, 109, 109),
            ("ch4-td-5d.txt", 5, 22, 22)):
        path = os.path.join(HERE, "fixtures", "g16mo", fixture)
        run = subprocess.run([out, path, str(natoms), str(nbasis)],
                             capture_output=True, text=True, env=env)
        print(run.stdout, end="")
        if run.stderr:
            print(run.stderr, end="")
        if run.returncode != 0:
            rc = run.returncode

        m = re.search(r"(\d+) labelled, (\d+) agree, (\d+) disagree",
                      run.stdout)
        if m is None:
            print("  %s: could not find the labelled/agree/disagree line" %
                  fixture)
            rc = 1
        else:
            labelled = int(m.group(1))
            ok = labelled >= minLabelled
            print("  %-16s degenerate-grouping floor: %d labelled >= %d %s" %
                  (fixture, labelled, minLabelled, "ok" if ok else "FAIL"))
            if not ok:
                rc = 1
    os.unlink(out)
    return rc


def checkSubgroupCrossCheck(verbose):
    """The independent, no-gensym-frame cross-check (#147/#132) against
    REAL ORCA calculations: SymmetryAnalysis::subgroupLabelSpectrum()
    reproduces ORCA's own abelian-subgroup labels (D2 for CH4/Td, D2H
    for benzene/D6h -- #151), built directly from diagonal +/-1
    operations in ORCA's own (stored) frame, with NO autosym/gensym
    involved on that side at all -- and every fullLabelSpectrum() full-
    group label subduces to it. See testSubgroupCrossCheck.C.
    """
    build = os.environ.get("ECCE_TEST_BUILD", os.path.join(ROOT, "build-cmake"))
    if not os.path.isdir(build):
        print("  skipped: no build tree at %s (set ECCE_TEST_BUILD)" % build)
        return 0
    symops = os.environ.get("ECCE_TEST_SYMOPS", os.path.join(ROOT, "build-cmake", "symops"))
    if not (os.path.isfile(symops) and os.access(symops, os.X_OK)):
        print("  symops not built -- skipping the subgroup cross-check")
        return 0

    libs = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
            "ecceutil", "eccecomm", "eccecipc", "eccercmd"]
    out = os.path.join(HERE, "testSubgroupCrossCheck")
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(ROOT, "include"),
            "-o", out,
            os.path.join(HERE, "testSubgroupCrossCheck.C"),
            os.path.join(ROOT, "src/tdat/chemistry/ShellRotation.C"),
            os.path.join(ROOT, "src/tdat/chemistry/SymmetryAnalysis.C"),
            os.path.join(ROOT, "src/tdat/chemistry/CharacterTable.C"),
            os.path.join(ROOT, "src/tdat/chemistry/BasisFlatten.C"),
            "-L" + build]
           + ["-l" + l for l in libs]*3 + ["-lxerces-c", "-lmosquitto", "-lssl", "-lcrypto"])
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        print("  could not build testSubgroupCrossCheck:")
        print(proc.stderr)
        return 1

    env = dict(os.environ)
    env["ECCE_HOME"] = ROOT
    env.setdefault("ECCE_REALUSERHOME", os.path.expanduser("~"))
    env["ECCE_TEST_SYMOPS"] = symops

    rc = 0
    #  (fixture, natoms, nbasis, orbitals that must get a full label).
    #  The diatomics (#132): C4v/D4h stand in for the linear groups, the
    #  axis is on z, spherical d -- all 28 must label, including the
    #  delta set C4v splits into B1 + B2 and N2's 0.9 mEh core pair.
    for fixture, natoms, nbasis, minFull in (("ch4-orca-td.txt", 5, 22, 1),
                                             ("c6h6-orca-d6h.txt", 12, 96, 1),
                                             ("co-orca-cinfv.txt", 2, 28, 28),
                                             ("n2-orca-dinfh.txt", 2, 28, 28)):
        path = os.path.join(HERE, "fixtures", "g16mo", fixture)
        run = subprocess.run([out, path, str(natoms), str(nbasis), str(minFull)],
                             capture_output=True, text=True, env=env)
        print(run.stdout, end="")
        if run.stderr:
            print(run.stderr, end="")
        if run.returncode != 0:
            rc = run.returncode
    os.unlink(out)
    return rc


def checkDiatomicComposition(verbose):
    """The composition fallback (#132) against REAL N2 MOs.

    A homonuclear diatomic's fragment is a single atom, which spans no
    irrep of the molecule's group at all -- there is nothing for
    irrep-matching to connect. Composition (which SHELL an orbital is
    built from, from its own MO coefficients) is the only thing left,
    and this drives the real pipeline (MoFragments::build(),
    composeLevels(), classify()/connect()/placeFragments()) on N2's
    real ORCA orbitals with no symmetry labels handed to the centre
    column -- exactly what MoDiagramPanel::build() does for any
    homonuclear diatomic -- and checks the textbook answer against it:
    2sigma_g/2sigma_u to N 2s, 3sigma_g/1pi_u/1pi_g*/3sigma_u* to N 2p.
    See testDiatomicComposition.C.
    """
    build = os.environ.get("ECCE_TEST_BUILD", os.path.join(ROOT, "build-cmake"))
    if not os.path.isdir(build):
        print("  skipped: no build tree at %s (set ECCE_TEST_BUILD)" % build)
        return 0
    symops = os.environ.get("ECCE_TEST_SYMOPS", os.path.join(ROOT, "build-cmake", "symops"))
    if not (os.path.isfile(symops) and os.access(symops, os.X_OK)):
        print("  symops not built -- skipping the diatomic composition check")
        return 0

    libs = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
            "ecceutil", "eccecomm", "eccecipc", "eccercmd"]
    out = os.path.join(HERE, "testDiatomicComposition")
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(ROOT, "include"),
            "-o", out,
            os.path.join(HERE, "testDiatomicComposition.C"),
            os.path.join(ROOT, "src/tdat/chemistry/MoFragments.C"),
            os.path.join(ROOT, "src/tdat/chemistry/MoComposition.C"),
            os.path.join(ROOT, "src/tdat/chemistry/MoDiagram.C"),
            os.path.join(ROOT, "src/tdat/chemistry/ShellRotation.C"),
            os.path.join(ROOT, "src/tdat/chemistry/SymmetryAnalysis.C"),
            os.path.join(ROOT, "src/tdat/chemistry/CharacterTable.C"),
            os.path.join(ROOT, "src/tdat/chemistry/BasisFlatten.C"),
            "-L" + build]
           + ["-l" + l for l in libs]*3 + ["-lxerces-c", "-lmosquitto", "-lssl", "-lcrypto"])
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        print("  could not build testDiatomicComposition:")
        print(proc.stderr)
        return 1

    env = dict(os.environ)
    env["ECCE_HOME"] = ROOT
    env.setdefault("ECCE_REALUSERHOME", os.path.expanduser("~"))
    #  MoFragments::symmetryOperations() resolves $ECCE_HOME/bin/symops
    #  and falls back to a bare "symops" on PATH when that does not
    #  exist -- it does not, in a repo checkout -- so the real binary
    #  is reached the same way checkFragments() reaches it.
    env["PATH"] = os.path.dirname(symops) + os.pathsep + env.get("PATH", "")

    run = subprocess.run([out, os.path.join(HERE, "fixtures", "g16mo")],
                         capture_output=True, text=True, env=env)
    print(run.stdout, end="")
    if run.stderr:
        print(run.stderr, end="")
    os.unlink(out)
    return run.returncode


def checkOracle(tablePath, verbose):
    """The one check that does not need to know the answer.

    Every molecular orbital is a combination of the basis functions, so
    the irreps the whole basis spans and the irreps the code reports
    for its orbitals are the same multiset.  One side is computed from
    the geometry and the group; the other is read from the
    calculation's own output.  Two programs, different data, different
    routes -- and it works on a molecule nobody wrote an expected
    answer for.

    Runs MOPAC on a handful of molecules and compares.  Skipped where
    MOPAC is absent, which is a skip and not a pass.
    """
    binary = os.environ.get("ECCE_TEST_SYMOPS",
                            os.path.join(ROOT, "build-cmake", "symops"))
    if not (os.path.isfile(binary) and os.access(binary, os.X_OK)):
        print("  symops not built -- skipping the basis/labels oracle")
        return 0
    if shutil.which("mopac") is None:
        print("  mopac not installed -- skipping the basis/labels oracle")
        return 0

    sys.path.insert(0, os.path.join(ROOT, "tools", "modiagram"))
    try:
        from frommopac import (run as runMopac, parse, symmetrise,
                               basisFromOutput)
    except ImportError as why:
        print("  could not load the MOPAC helper: %s" % why)
        return 1

    d, dd = 1.0219, 0.6276
    CASES = {
        "CH4":  [("C", 0, 0, 0)] + [("H",) + c for c in
                 ((dd, dd, dd), (dd, -dd, -dd), (-dd, dd, -dd), (-dd, -dd, dd))],
        "H2O":  [("O", 0, 0, 0.1173), ("H", 0, 0.7572, -0.4692),
                 ("H", 0, -0.7572, -0.4692)],
        "NH3":  [("N", 0, 0, 0.1173), ("H", 0, 0.9377, -0.2737),
                 ("H", 0.8121, -0.4689, -0.2737),
                 ("H", -0.8121, -0.4689, -0.2737)],
        "CCl4": [("C", 0, 0, 0)] + [("Cl",) + c for c in
                 ((d, d, d), (d, -d, -d), (-d, d, -d), (-d, -d, d))],
        "N2":   [("N", 0, 0, 0.5488), ("N", 0, 0, -0.5488)],
    }

    #  A tiny driver, because the check itself lives in C++.
    src = os.path.join(HERE, "testOracle.C")
    out = os.path.join(HERE, "testOracle")
    build = subprocess.run(
        ["g++", "-O2", "-w", "-I", os.path.join(ROOT, "include"), "-o", out,
         src,
         os.path.join(ROOT, "src/tdat/chemistry/MoFragments.C"),
         os.path.join(ROOT, "src/tdat/chemistry/MoComposition.C"),
         os.path.join(ROOT, "src/tdat/chemistry/SymmetryAnalysis.C"),
         os.path.join(ROOT, "src/tdat/chemistry/ShellRotation.C"),
         os.path.join(ROOT, "src/tdat/chemistry/TGBSAngFunc.C"),
         os.path.join(ROOT, "src/tdat/chemistry/BasisAngularNorm.C"),
         os.path.join(ROOT, "src/tdat/chemistry/CharacterTable.C"),
         os.path.join(ROOT, "src/tdat/chemistry/MoDiagram.C"),
         os.path.join(ROOT, "src/tdat/chemistry/Huckel.C")],
        capture_output=True, text=True)
    if build.returncode != 0:
        print("  could not build the oracle driver:")
        print(build.stderr)
        return 1

    env = dict(os.environ)
    env["ECCE_HOME"] = ROOT
    env["PATH"] = os.path.dirname(binary) + os.pathsep + env.get("PATH", "")

    bad = 0

    #  Real calculations first, from checked-in fixtures.  The MOPAC
    #  cases below are generated on the fly, which keeps them honest
    #  but limits them to a semiempirical valence basis: four functions
    #  an atom, s and p, no spherical harmonics.  A real basis set is a
    #  different exercise, and it is the path ECCE actually uses.
    fixtures = os.path.join(HERE, "fixtures")
    if os.path.isdir(fixtures):
        for entry in sorted(os.listdir(fixtures)):
            if not entry.endswith(".oracle"):
                continue
            proc = subprocess.run([out], stdin=open(os.path.join(fixtures, entry)),
                                  capture_output=True, text=True, env=env)
            said = proc.stdout.strip()
            ok = proc.returncode == 0

            #  "Cannot be compared" is a third outcome, and it is
            #  printed every time rather than counted as either.  A
            #  code that ran a job in a lower group than the structure
            #  has leaves two sets of names with nothing to do with
            #  each other; calling that a failure would be wrong, and
            #  swallowing it would be worse.
            if said.startswith("not comparable"):
                print("  %-22s SKIP  %s" % (entry[:-7], said))
                continue
            if verbose or not ok:
                print("  %-22s %s" % (entry[:-7], said))
            if not ok:
                bad += 1

    for name, atoms in CASES.items():
        group, clean = symmetrise(atoms)
        text = runMopac(name, atoms)
        _, energies, labels = parse(text)
        perAtom, shellOf = basisFromOutput(text, len(atoms))

        lines = [group, str(len(atoms))]
        for (symbol, _, _, _), (_, x, y, z) in zip(atoms, clean):
            lines.append("%s %.6f %.6f %.6f" % (symbol, x, y, z))
        lines.append(" ".join(str(n) for n in perAtom))
        lines.append(" ".join(str(n) for n in shellOf))
        lines.append(" ".join(labels))

        proc = subprocess.run([out], input="\n".join(lines) + "\n",
                              capture_output=True, text=True, env=env)
        said = proc.stdout.strip()
        ok = proc.returncode == 0
        if said.startswith("not comparable"):
            print("  %-6s SKIP  %s" % (name, said))
            continue
        if verbose or not ok:
            print("  %-6s %s" % (name, said))
        if not ok:
            bad += 1
    os.unlink(out)

    if not bad and not verbose:
        extra = len([e for e in os.listdir(fixtures) if e.endswith(".oracle")]) \
                if os.path.isdir(fixtures) else 0
        print("  basis spans what the code reports: PASS (%d molecules, "
              "%d of them real calculations)" % (len(CASES) + extra, extra))
    return 1 if bad else 0



def checkCanvas(verbose):
    """Paint the real canvas, without a person looking at it.

    The model has been checked all along; the LAYOUT had not, because
    reaching it meant building a package, installing it, starting the
    services and opening a calculation.  A second implementation of the
    layout grew up in tools/modiagram for iterating on, and two
    implementations of one picture drift: when this was first run the
    canvas still printed a tick as -5.55112e-17, drew no electrons on
    the fragment columns, and wrote its two footer lines on top of each
    other -- all three fixed in the renderer and never here.

    It cannot say whether the picture is good.  It can say whether it
    painted at all, and whether it survives the cases that quietly
    produce nothing: an empty column, a spectrum with no energy range,
    a window too small to lay out in.

    Needs wx and a display; Xvfb is enough.
    """
    if shutil.which("wx-config") is None:
        print("  wx-config not found -- skipping the canvas check")
        return 0
    if shutil.which("xvfb-run") is None and not os.environ.get("DISPLAY"):
        print("  no display and no xvfb-run -- skipping the canvas check")
        return 0

    flags = subprocess.run(["wx-config", "--cxxflags"],
                           capture_output=True, text=True).stdout.split()
    libs = subprocess.run(["wx-config", "--libs", "core,base"],
                          capture_output=True, text=True).stdout.split()

    out = os.path.join(HERE, "testCanvas")
    build = subprocess.run(
        ["g++", "-O2", "-w", "-I", os.path.join(ROOT, "include"),
         "-I", os.path.join(ROOT, "src", "apps", "builder")] + flags +
        ["-o", out, os.path.join(HERE, "testCanvas.C"),
         os.path.join(ROOT, "src/tdat/chemistry/MoDiagram.C"),
         #  MoDiagram asks the character table which irreps change
         #  sign in the molecular plane, for the pi-only view.
         os.path.join(ROOT, "src/tdat/chemistry/CharacterTable.C")] + libs,
        capture_output=True, text=True)
    if build.returncode != 0:
        print("  could not build the canvas test:")
        print(build.stderr[-1500:])
        return 1

    command = [out]
    if not os.environ.get("DISPLAY"):
        command = ["xvfb-run", "-a"] + command
    proc = subprocess.run(command, capture_output=True, text=True)
    if verbose or proc.returncode != 0:
        print(proc.stdout, end="")
        if proc.returncode != 0:
            print(proc.stderr[-800:], end="")
    else:
        print("  the canvas paints: PASS")
    os.unlink(out)
    return proc.returncode



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

    beforeVoie = report.checks
    checkValenceEnergies(report)
    checkAutosymThreshold(report)
    print("  valence orbital energies: %d checks"
          % (report.checks - beforeVoie))

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
    if checkCanvas(args.verbose) != 0:
        print("FAILED  the canvas")
        return 1

    print("")
    if checkOracle(tablePath, args.verbose) != 0:
        print("FAILED  the basis/labels oracle")
        return 1

    print("")
    if checkFragments(tablePath, args.verbose) != 0:
        print("FAILED  the fragment columns")
        return 1

    print("")
    if checkLoader(tablePath, args.verbose) != 0:
        print("FAILED  the C++ loader")
        return 1

    if checkAnalysis(tablePath, args.verbose) != 0:
        print("FAILED  the orbit/reduction analysis")
        return 1

    if standalone("testMoDiagram",
                  ["src/tdat/chemistry/MoDiagram.C",
                   "src/tdat/chemistry/CharacterTable.C"]) != 0:
        print("FAILED  the diagram layout")
        return 1

    #  The engine that supplies a middle column when no calculation
    #  does.  Checked against what the method is obliged to give --
    #  degeneracies the geometry forces, and a non-bonding orbital
    #  left exactly where its atom put it -- rather than against
    #  somebody else's table of numbers, which would check the
    #  parameters and not the code.
    if standalone("testAlignFrames",
                  ["src/tdat/chemistry/SymmetryAnalysis.C",
                   "src/tdat/chemistry/ShellRotation.C",
                   "src/tdat/chemistry/TGBSAngFunc.C",
                   "src/tdat/chemistry/BasisAngularNorm.C",
                   "src/tdat/chemistry/CharacterTable.C"]) != 0:
        print("FAILED  frame alignment")
        return 1

    print("")
    if checkShellRotation(args.verbose) != 0:
        print("FAILED  shell rotation matrices")
        return 1

    print("")
    if checkFullOrbitalIrrep(args.verbose) != 0:
        print("FAILED  the full-point-group ORBSYM oracle")
        return 1

    print("")
    if checkSubgroupCrossCheck(args.verbose) != 0:
        print("FAILED  the ORCA subgroup cross-check")
        return 1

    print("")
    if checkDiatomicComposition(args.verbose) != 0:
        print("FAILED  the diatomic composition fallback")
        return 1

    if standalone("testHuckel",
                  ["src/tdat/chemistry/Huckel.C",
                   "src/tdat/chemistry/MoFragments.C",
                   "src/tdat/chemistry/MoComposition.C",
                   "src/tdat/chemistry/SymmetryAnalysis.C",
                   "src/tdat/chemistry/ShellRotation.C",
                   "src/tdat/chemistry/TGBSAngFunc.C",
                   "src/tdat/chemistry/BasisAngularNorm.C",
                   "src/tdat/chemistry/CharacterTable.C",
                   "src/tdat/chemistry/MoDiagram.C"]) != 0:
        print("FAILED  extended Huckel")
        return 1

    print("")
    if standalone("testLowdinComposition",
                  ["src/tdat/chemistry/MoComposition.C",
                   "src/tdat/chemistry/MoFragments.C",
                   "src/tdat/chemistry/Huckel.C",
                   "src/tdat/chemistry/SymmetryAnalysis.C",
                   "src/tdat/chemistry/ShellRotation.C",
                   "src/tdat/chemistry/TGBSAngFunc.C",
                   "src/tdat/chemistry/BasisAngularNorm.C",
                   "src/tdat/chemistry/CharacterTable.C",
                   "src/tdat/chemistry/MoDiagram.C"]) != 0:
        print("FAILED  Lowdin composition")
        return 1

    print("")
    if standalone("testMoLigandField",
                  ["src/tdat/chemistry/MoLigandField.C",
                   "src/tdat/chemistry/MoComposition.C",
                   "src/tdat/chemistry/MoFragments.C",
                   "src/tdat/chemistry/SymmetryAnalysis.C",
                   "src/tdat/chemistry/ShellRotation.C",
                   "src/tdat/chemistry/TGBSAngFunc.C",
                   "src/tdat/chemistry/BasisAngularNorm.C",
                   "src/tdat/chemistry/CharacterTable.C",
                   "src/tdat/chemistry/MoDiagram.C",
                   "src/tdat/chemistry/Huckel.C"]) != 0:
        print("FAILED  the ligand-field interaction model")
        return 1

    print("")
    if standalone("testLinearPointGroup", []) != 0 or \
            checkLinearGeneratorGroups(args.verbose) != 0:
        print("FAILED  linear point groups")
        return 1

    if checkUseSymmetryRule(args.verbose) != 0:
        print("FAILED  the Use symmetry rule in generated decks")
        return 1

    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
