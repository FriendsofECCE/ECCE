#!/usr/bin/env python3
"""Calculations for the ELF/LOL/NCI checks (#799, #800), from real ECCE-QM runs,
and the same wavefunctions as molden files for Multiwfn.  Run from the repo root
with ECCE_QM=build-cmake/src/qm/ecce-qm and ECCE_BASIS_DIR=data/admin/basissets:

  make_densityfields_fixtures.py [MOLDEN_DIR]

  water   RHF 6-31G*          n2  RHF 6-31G*
  dimer   RHF 6-31G* (S22 water dimer)
  o2      UHF triplet 6-31G*  (alpha and beta MO sets)

The calculations go to tests/tdat/densityfields-fixtures; the molden files, if
a directory is given, to it (not committed).
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
ROOT = os.path.dirname(ROOT)
sys.path.insert(0, os.path.join(ROOT, "tests", "builder"))
import make_densitymap_fixtures as conv   # noqa: E402

DST = os.path.join(HERE, "densityfields-fixtures")
CASES = {"water": ["h2o", "hf", "1"], "n2": ["n2", "hf", "1"],
         "dimer": ["h2o2", "hf", "1"], "o2": ["o2", "uhf", "3"]}
qm = os.environ.get("ECCE_QM", "ecce-qm")
molden = sys.argv[1] if len(sys.argv) > 1 else None

for name, (mol, method, mult) in CASES.items():
    work = tempfile.mkdtemp(prefix="densityfields-fixture-")
    try:
        out = subprocess.check_output(
            [sys.executable, os.path.join(ROOT, "tests", "modiagram", "makecalc.py"),
             "ecceqm", mol, "0", os.path.join(work, "raw"), "--no-rotate",
             "--method", method, "--multiplicity", mult, "--spherical-flag", "--keep"],
            universal_newlines=True)
        kept = re.search(r"workdir kept: (\S+)", out).group(1)
        conv.SRC = work
        conv.convert(name, "raw", os.path.join(DST, name))
        if molden:
            os.makedirs(molden, exist_ok=True)
            subprocess.check_call([qm, "--molden", os.path.join(molden, name + ".molden"),
                                   "-o", os.devnull, "ecceqm.qmin"], cwd=kept)
        shutil.rmtree(kept, ignore_errors=True)
    finally:
        shutil.rmtree(work, ignore_errors=True)
print("ok")
