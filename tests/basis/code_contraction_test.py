#!/usr/bin/env python3
"""ECCE's flattened cc-pVTZ for ORCA against ORCA's own basis (#942).

    tests/basis/code_contraction_test.py <gbscodecontraction>

The oracle is ORCA 6.1.1's "BASIS SET IN INPUT FORMAT" print (Print[P_Basis]
2) for CH4/cc-pVTZ, read here with its own regex.  ECCE's library cc-pVTZ
(data/admin/basissets/cc-pVTZ.BAS) is flattened after the pattern the same output prints ("pattern {...}") is
applied, and every shell must have ORCA's exponents and, as a radial
function, the same shape (cosine of the two contractions > 1 - 1e-9).  Without
the pattern the s shells differ, which the test also checks.
"""
import math
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
BAS = os.path.join(ROOT, "data", "admin", "basissets", "cc-pVTZ.BAS")
OUT = os.path.join(ROOT, "tests", "parsers", "fixtures", "orca", "ch4_ccpvtz_basis.out")
L = "SPDFGHI"


def orca_basis(text):
    """{element: [(l, [(exponent, coefficient for a normalised primitive)])]}"""
    block = text.split("BASIS SET IN INPUT FORMAT", 1)[1]
    found = {}
    for el, body in re.findall(r"NewGTO\s+(\w+)\s*\n(.*?)\bend;", block, re.S):
        shells = []
        for line in body.splitlines():
            m = re.match(r"\s*([SPDFGHI])\s+(\d+)\s*$", line)
            if m:
                shells.append((L.index(m.group(1)), []))
                continue
            m = re.match(r"\s*\d+\s+([-+.\dEe]+)\s+([-+.\dEe]+)\s*$", line)
            if m:
                shells[-1][1].append((float(m.group(1)), float(m.group(2))))
        found[el] = shells
    return found


def orca_patterns(text):
    return {m.group(1): m.group(2) for m in
            re.finditer(r"Group\s+\d+\s+Type\s+(\S+)\s*:.*pattern \{([^}]+)\}", text)}


def library_text(el):
    """One element of ECCE's library file as importConfig's NumericalBasis."""
    lines, on = [], False
    for line in open(BAS):
        if line.startswith("atom="):
            on = line.strip() == "atom=" + el
        elif on and line.startswith("contraction"):
            shell = re.search(r"shell=(\w+)", line).group(1)
            lines.append("%s %s" % (el, shell))
        elif on:
            lines.append("  " + line.strip())
    return ("NumericalBasis\nbasis \"ao basis\" spherical print\n" +
            "\n".join(lines) + "\nEND\nEndNumericalBasis\n")


def ecce_basis(exe, pattern, el):
    r = subprocess.run([exe, pattern, el], input=library_text(el),
                       capture_output=True, text=True)
    if r.returncode != 0:
        sys.stderr.write(r.stdout + r.stderr)
        raise SystemExit("FAIL: gbscodecontraction %s" % el)
    shells, changed = [], None
    for line in r.stdout.splitlines():
        q = line.split()
        if q[0] == "changed":
            changed = q[1] == "1"
        elif q[0] == "SHELL":
            shells.append((int(q[2]), []))
        else:
            shells[-1][1].append((float(q[0]), float(q[1])))
    return changed, shells


def cosine(l, a, b):
    """a, b: [(exponent, coefficient of r^l exp(-e r^2))]"""
    def s(x, y):
        return sum(c*d*(e+f)**-(l+1.5) for e, c in x for f, d in y)
    return s(a, b)/math.sqrt(s(a, a)*s(b, b))


def compare(l, orca, ecce):
    # ORCA's coefficients belong to normalised primitives: N ~ e^((2l+3)/4)
    unnorm = [(e, c*e**((2*l+3)/4.0)) for e, c in orca]
    same = sorted(e for e, _ in orca) == sorted(e for e, _ in ecce)
    return same, cosine(l, unnorm, ecce)


def main():
    exe = sys.argv[1]
    text = open(OUT).read()
    ref, pats = orca_basis(text), orca_patterns(text)
    pattern = "|".join("%s %s" % kv for kv in sorted(pats.items()))
    fails = 0
    for el in ("H", "C"):
        for label, pat, want in (("with pattern", pattern, True),
                                 ("as stored", "", False)):
            changed, shells = ecce_basis(exe, pat, el)
            ok = len(shells) == len(ref[el]) and all(
                l1 == l2 for (l1, _), (l2, _) in zip(shells, ref[el]))
            worst = 1.0
            for (l, o), (_, e) in zip(ref[el], shells):
                same, c = compare(l, o, e)
                ok = ok and same
                worst = min(worst, c)
            ok = ok and worst > 1 - 1e-9
            print("%s %-12s %s: %d shells, worst cosine 1-%.1e, exponents %s" % (
                el, label, "matches ORCA" if ok else "differs from ORCA",
                len(shells), 1 - worst, "same" if ok else "or shape differ"))
            if ok != want or (want and not changed):
                print("FAIL: %s %s" % (el, label))
                fails += 1
    print("RESULT", "FAIL" if fails else "OK")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
