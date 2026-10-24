#!/usr/bin/env python3
"""The Teaching example set is complete on disk (#447); no install needed.

    tests/apps/examples_teaching_test.py     (ctest: apps_examples_teaching)

Each calculation is an ECCE-QM input with the output of a real run that holds
what the viewer needs for the MO diagram and the orbital/density contour maps
(#215): converged SCF, orbital energies and occupations, the basis labels and
the MO coefficients.  The importer (apps_examples, `examples teaching`) is
the same code path as for the other sets.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SET = os.path.join(ROOT, "data", "client", "examples", "teaching")
# calculation -> (multiplicity, method keyword, occupied orbitals expected)
CALCS = {"h2": (1, "b3lyp", 1), "he2": (1, "b3lyp", 2), "n2": (1, "b3lyp", 7),
         "o2": (3, "b3lyp", 9), "f2": (1, "b3lyp", 9), "co": (1, "b3lyp", 7),
         "hf": (1, "b3lyp", 5)}
# Quantum ESPRESSO crystals: calculation -> (file base, element, a in Angstrom,
# atoms-per-conventional-cell, ecutwfc)
CRYSTALS = {"al-fcc": ("al", "Al", 4.050, 4, 40), "na-bcc": ("na", "Na", 4.291, 2, 50),
            "po-sc": ("po", "Po", 3.359, 1, 60)}
bad = []

lines = open(os.path.join(SET, "set.txt")).read().splitlines()
if len(lines) < 2 or lines[0] != "Teaching" or not lines[1].strip():
    bad.append("set.txt must be the name and one line of description")
found = sorted(d for d in os.listdir(SET) if os.path.isdir(os.path.join(SET, d)))
if found != sorted(list(CALCS) + list(CRYSTALS)):
    bad.append("calculations %s, expected %s" % (found, sorted(list(CALCS) + list(CRYSTALS))))
if len(found) > 20:
    bad.append("%d calculations; a set holds at most 20" % len(found))

for name, (mult, method, nocc) in CALCS.items():
    d = os.path.join(SET, name)
    try:
        qin = open(os.path.join(d, name + ".qmin")).read()
        out = open(os.path.join(d, name + ".qmout")).read()
    except OSError as e:
        bad.append("%s: %s" % (name, e))
        continue
    if not re.search(r"^multiplicity\s+%d$" % mult, qin, re.M) \
            or not re.search(r"^method\s+%s$" % method, qin, re.M):
        bad.append("%s: input is not %s, multiplicity %d" % (name, method, mult))
    if not re.search(r"^converged yes$", out, re.M) or "end_of_output" not in out:
        bad.append("%s: output is not a converged, finished run" % name)
    orb = re.search(r"begin orbitals.*?\n(.*?)end orbitals", out, re.S)
    coef = re.search(r"begin mo_coefficients.*?\n(.*?)end mo_coefficients", out, re.S)
    labels = re.search(r"begin ao_labels.*?\n(.*?)end ao_labels", out, re.S)
    if not (orb and coef and labels):
        bad.append("%s: output lacks orbitals, ao_labels or mo_coefficients" % name)
        continue
    rows = [l.split() for l in orb.group(1).strip().splitlines()]
    occupied = sum(1 for r in rows if float(r[2]) > 0)
    nbf = len(labels.group(1).strip().splitlines())
    if occupied != nocc:
        bad.append("%s: %d occupied orbitals, expected %d" % (name, occupied, nocc))
    cl = coef.group(1).strip().splitlines()
    if len(cl) != len(rows) or any(len(l.split()) != nbf + 1 for l in cl):
        bad.append("%s: MO coefficient block is not %d x %d" % (name, len(rows), nbf))

for name, (base, el, a, nlat, ecut) in CRYSTALS.items():
    d = os.path.join(SET, name)
    try:
        pin = open(os.path.join(d, base + ".pwin")).read()
        out = open(os.path.join(d, base + ".pwout")).read()
    except OSError as e:
        bad.append("%s: %s" % (name, e))
        continue
    if not re.search(r"^  ecutwfc = %d\.?0*$" % ecut, pin, re.M) or \
            not re.search(r"^  %s\s+\S+\s+\S+\.(UPF|upf)$" % el, pin, re.M):
        bad.append("%s: input is not %s at ecutwfc %d" % (name, el, ecut))
    # the cell volume per atom fixes a for the lattice: V = a^3 / nlat
    vol = re.search(r"unit-cell volume\s+=\s+([\d.]+) \(a\.u\.\)\^3", out)
    if vol:
        got = float(vol.group(1)) * 0.529177 ** 3
        if abs(got - a ** 3 / nlat) > 0.05:
            bad.append("%s: cell volume %.2f A^3, expected %.2f" % (name, got, a ** 3 / nlat))
    else:
        bad.append("%s: no unit-cell volume in the output" % name)
    if not re.search(r"^!\s+total energy", out, re.M) or "JOB DONE" not in out \
            or "convergence has been achieved" not in out:
        bad.append("%s: output is not a converged, finished pw.x run" % name)

if bad:
    print("FAIL  " + "\n      ".join(bad))
    sys.exit(1)
print("ok    Teaching set: %s" % ", ".join(sorted(list(CALCS) + list(CRYSTALS))))
