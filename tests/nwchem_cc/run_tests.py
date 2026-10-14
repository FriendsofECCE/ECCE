#!/usr/bin/env python3
"""ai.nwchem must produce a deck for MP2 and coupled-cluster theories.

Since 8.12.0 the &SCF section died and ^R matched before ^RO in &SCFTheory,
so these decks failed with "Unrecognized Category".  Needs only python3 and
perl; the param file is written by hand, no dialogs involved.
"""

import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(REPO, "scripts", "parsers")

FRAG = """title: h2o
type: molecule
num_atoms: 3
atom_info: symbol cart
atom_list:
O 0.000000 0.000000 0.117000
H 0.000000 0.757000 -0.468000
H 0.000000 -0.757000 -0.468000
"""
BASIS = 'basis "ao basis" cartesian print\n  H library "6-31G*"\n  O library "6-31G*"\nEND\n'

#  theory -> regex the task line must match
CASES = [("RMP2", "task mp2"), ("UMP2", "task mp2"),
         ("RCCSD", "task ccsd"), ("RCCSD(T)", "task ccsd\\(t\\)")]


def param(theory):
    return "\n".join([
        "title: h2o", "annotation: h2o", "parseFile: ecce.out",
        "Category: %s" % ("CC" if "CC" in theory else "MP"), "Theory: %s" % theory, "RunType: Energy",
        "Charge: 0", "Symmetry: C1", "NumElectrons: 10", "NumFrozenOrbs: 0",
        "NumOccupiedOrbs: 5", "NumVirtualOrbs: 20", "NumNormalModes: 3",
        "ChemSys.Multiplicity: 1", "BasisSet.Coordinates: Cartesian",
        "ES.Theory.UseSymmetry: 1", ""])


def generate(theory):
    work = tempfile.mkdtemp(prefix="ecce-nwcc-")
    try:
        for ext, text in (("frag", FRAG), ("basis", BASIS), ("param", param(theory))):
            with open(os.path.join(work, "calc." + ext), "w") as h:
                h.write(text)
        tpl = os.path.join(work, "nwch.nw.orig")
        shutil.copy(os.path.join(PARSERS, "nwch.tpl"), tpl)
        proc = subprocess.run(
            ["perl", os.path.join(PARSERS, "ai.nwchem"), "-n", "calc", "-p", "-f",
             "-b", "-t", tpl], cwd=work, capture_output=True, text=True,
            env=dict(os.environ, ECCE_HOME=REPO))
        out = proc.stdout + proc.stderr
        with open(tpl) as h:
            return proc.returncode, h.read(), out
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    import re
    bad = 0
    for theory, task in CASES:
        rc, deck, out = generate(theory)
        ok = rc == 0 and re.search(r"^\s*%s energy\s*$" % task, deck, re.M | re.I) \
            and "Unrecognized" not in out
        print("%s %s" % ("PASS" if ok else "FAIL", theory))
        if not ok:
            bad += 1
            print("  exit %d: %s" % (rc, out.strip()[:300]))
    print("%d of %d failed" % (bad, len(CASES)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
