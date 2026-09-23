#!/usr/bin/env python3
"""
Adjudicate a disputed basis set against the codes' own built-in copies.

Step 6 of the re-sourcing plan (issue #116).  Comparing ECCE against BSE
only ever says THAT they differ, never which is right -- there was no
reference.  The codes ship their own copies of these basis sets, so:

    run the same single atom four ways
      1. ORCA's built-in basis, by name
      2. Gaussian's built-in basis, by name
      3. ECCE's stored data, written out explicitly
      4. BSE's data, written out explicitly

If the two built-ins agree with each other they are an independent
reference, and whichever of ECCE and BSE reproduces them is correct.
If the built-ins disagree, there is no reference and the difference is a
genuine ambiguity in the literature rather than an error in either.

A single atom is used on purpose: it isolates the element that actually
differs, and removes geometry as a variable.
"""

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
import urllib.parse
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from bse2bas import convert                                     # noqa: E402
from diffreport import parse_bas, parse_text, SYM               # noqa: E402

ORCA = "/opt/orca/orca_6_1_1_linux_x86-64_shared_openmpi418_nodmrg/orca"
LIB = os.path.expanduser("~/.ECCE/dataserver/htdocs/Ecce/system/"
                         "GaussianBasisSetLibrary")
LETTER = "SPDFGHI"

#  Ground-state multiplicity, so a single atom is a valid calculation.
MULT = {"H": 2, "He": 1, "C": 3, "N": 4, "O": 3, "S": 3, "Ne": 1, "Ar": 1,
        "Kr": 1, "Xe": 1, "Sc": 2, "Ti": 3, "V": 4, "Cr": 7, "Mn": 6,
        "Fe": 5, "Co": 4, "Ni": 3, "Cu": 2, "Zn": 1, "Br": 2, "Mo": 7,
        "I": 2, "Ag": 2, "Ca": 1, "Mg": 1, "Al": 2, "Si": 3, "P": 4}


def orca_newgto(element, shells):
    """One atom's shells as an ORCA NewGTO block.

    ORCA takes one coefficient per line, so a general contraction has to
    be split into a separate block per coefficient column, and a column's
    zero entries dropped -- which is what wrORCAGBS.pm does.
    """
    out = ["NewGTO %s" % element]
    for shell, nprim, ncoef, rows in shells:
        #  Each coefficient column is its own contracted function, and the
        #  shell label says which angular momentum each column carries:
        #  "SS" is two s functions sharing primitives, "SP" is one s and
        #  one p.  Taking shell[0] for every column turns an SP shell into
        #  two s functions -- which silently produced an oxygen atom at
        #  -331 Hartree instead of -74.8 before this was fixed.
        letters = list(shell) if len(shell) == ncoef else [shell[0]] * ncoef
        for column in range(ncoef):
            letter = letters[column] if column < len(letters) else shell[0]
            if letter not in LETTER:
                return None
            kept = [(r[0], r[column + 1]) for r in rows
                    if len(r) > column + 1 and float(r[column + 1]) != 0.0]
            if not kept:
                continue
            out.append(" %s %d" % (letter, len(kept)))
            for i, (e, c) in enumerate(kept, 1):
                out.append("  %d %.10f %.10f" % (i, e, c))
    out.append("end")
    return "\n".join(out)


def run_orca(element, multiplicity, basis_line, basis_block):
    body = "! RHF UHF\n" if multiplicity > 1 else "! RHF\n"
    body = "! %s\n" % ("UHF" if multiplicity > 1 else "RHF")
    if basis_line:
        body = "! %s %s\n" % ("UHF" if multiplicity > 1 else "RHF", basis_line)
    if basis_block:
        body += "%%basis\n%s\nend\n" % basis_block
    body += "* xyz 0 %d\n%s 0.0 0.0 0.0\n*\n" % (multiplicity, element)
    with tempfile.TemporaryDirectory() as directory:
        path = os.path.join(directory, "a.inp")
        open(path, "w").write(body)
        try:
            proc = subprocess.run([ORCA, path], capture_output=True,
                                  text=True, timeout=600, cwd=directory)
        except subprocess.TimeoutExpired:
            return None, "timeout"
        for line in proc.stdout.splitlines():
            if "FINAL SINGLE POINT ENERGY" in line:
                return float(line.split()[4]), None
        err = next((l.strip() for l in proc.stdout.splitlines()
                    if "Error" in l or "aborting" in l), "no energy")
        return None, err[:70]


def run_g16(element, multiplicity, basis_name):
    body = ("#P %s/%s SP\n\nt\n\n0 %d\n%s 0.0 0.0 0.0\n\n"
            % ("UHF" if multiplicity > 1 else "RHF", basis_name,
               multiplicity, element))
    env = dict(os.environ, g16root="/opt/gaussian")
    with tempfile.TemporaryDirectory() as directory:
        path = os.path.join(directory, "a.gjf")
        open(path, "w").write(body)
        try:
            proc = subprocess.run(
                "source /opt/gaussian/g16/bsd/g16.profile >/dev/null 2>&1; "
                "g16 < %s" % path, shell=True, executable="/bin/bash",
                capture_output=True, text=True, timeout=600, cwd=directory,
                env=env)
        except subprocess.TimeoutExpired:
            return None, "timeout"
        for line in proc.stdout.splitlines():
            m = re.match(r"\s*SCF Done:\s+E\(\S+\)\s*=\s*(\S+)", line)
            if m:
                return float(m.group(1)), None
        return None, "no energy"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("basis", help="ECCE's name for the basis set")
    ap.add_argument("element")
    ap.add_argument("--orca-name", help="the basis name ORCA knows it by")
    ap.add_argument("--g16-name", help="the basis name Gaussian knows it by")
    ap.add_argument("--multiplicity", type=int)
    args = ap.parse_args()

    element = args.element
    multiplicity = args.multiplicity or MULT.get(element)
    if not multiplicity:
        print("no ground-state multiplicity known for %s; pass "
              "--multiplicity" % element, file=sys.stderr)
        return 2

    baseline = json.load(open(os.path.join(HERE, "baseline.json")))
    namemap = json.load(open(os.path.join(HERE, "namemap.json")))
    filename = bse_name = None
    for entries in baseline.values():
        if args.basis in entries:
            filename = entries[args.basis]["files"][0]
    for bucket in ("direct", "alias", "curated"):
        if args.basis in namemap[bucket]:
            bse_name = namemap[bucket][args.basis]
    if not filename or not bse_name:
        print("unknown basis: %s" % args.basis, file=sys.stderr)
        return 2

    shipped = parse_bas(os.path.join(LIB, filename))
    url = ("https://www.basissetexchange.org/api/basis/%s/format/json/"
           % urllib.parse.quote(bse_name))
    with urllib.request.urlopen(url, timeout=90) as response:
        regen = parse_text(convert(json.load(response), SYM))

    print("%s / %s  (multiplicity %d)" % (args.basis, element, multiplicity))
    print("  ECCE file : %s" % filename)
    print("  BSE name  : %s\n" % bse_name)

    results = {}
    if args.orca_name:
        e, err = run_orca(element, multiplicity, args.orca_name, None)
        results["ORCA built-in"] = (e, err)
    if args.g16_name:
        e, err = run_g16(element, multiplicity, args.g16_name)
        results["G16 built-in"] = (e, err)
    for label, source in (("ECCE data", shipped), ("BSE data", regen)):
        block = orca_newgto(element, source.get(element, []))
        if block is None:
            results[label] = (None, "shell type not expressible in NewGTO")
        else:
            results[label] = run_orca(element, multiplicity, None, block)

    for label in ("ORCA built-in", "G16 built-in", "ECCE data", "BSE data"):
        if label not in results:
            continue
        energy, err = results[label]
        print("  %-14s %s" % (label,
                              "%.9f" % energy if energy is not None
                              else "FAILED (%s)" % err))

    built = [results[k][0] for k in ("ORCA built-in", "G16 built-in")
             if k in results and results[k][0] is not None]
    if len(built) == 2:
        print("\n  built-ins differ by %.2e Hartree" % abs(built[0] - built[1]))
    for label in ("ECCE data", "BSE data"):
        if results.get(label, (None,))[0] is None or not built:
            continue
        print("  %-14s vs ORCA built-in: %.2e Hartree"
              % (label, abs(results[label][0] - built[0])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
