#!/usr/bin/env python3
"""Local-data-mode hydrogen and neon calculations for the radial
distribution tests, from real ECCE-QM runs.  Run from the repository
root with ecce-qm findable (ECCE_QM=build-cmake/src/qm/ecce-qm) and
ECCE_BASIS_DIR=data/admin/basissets:

  H   UHF doublet, cc-pVQZ (the engine refuses the library's aug- files: diffuse part only)
  Ne  RHF, cc-pVTZ
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import make_densitymap_fixtures as conv   # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
DST = os.path.join(HERE, "radialdist-fixtures")
CASES = {"h": ["h", "--basis", "cc-pVQZ", "--multiplicity", "2"],
         "ne": ["ne", "--basis", "cc-pVTZ"]}

shutil.rmtree(DST, ignore_errors=True)
for name, args in CASES.items():
    work = tempfile.mkdtemp(prefix="radialdist-fixture-")
    try:
        subprocess.check_call(
            [sys.executable, os.path.join(ROOT, "tests", "modiagram", "makecalc.py"),
             "ecceqm", args[0], "0", os.path.join(work, "raw"), "--no-rotate",
             "--method", "hf", "--spherical-flag"] + args[1:])
        conv.SRC = work     # convert() reads SRC/<src>
        conv.convert(name, "raw", os.path.join(DST, name))
    finally:
        shutil.rmtree(work, ignore_errors=True)
print("ok")
