#!/usr/bin/env python3
"""Local-data-mode atom and diatomic calculations for the radial
distribution tests, from real ECCE-QM runs.  Run from the repository
root with ecce-qm findable (ECCE_QM=build-cmake/src/qm/ecce-qm) and
ECCE_BASIS_DIR=data/admin/basissets:

  H   UHF doublet, cc-pVQZ (the engine refuses the library's aug- files: diffuse part only)
  Ne  RHF, cc-pVTZ
  Ar  RHF, 6-31G*
  H2, N2  RHF, 6-31G* (bond along z, N2 along x)
  HCN     RHF, 6-31G*, atoms C H N on the z axis (a line whose atoms are not in order)

Names on the command line limit the run to those cases and keep the rest.
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
         "ne": ["ne", "--basis", "cc-pVTZ"],
         "ar": ["ar"],                 # 6-31G*
         "h2": ["h2"],
         "n2": ["n2"],
         "hcn": ["hcn"]}

only = sys.argv[1:]
if not only:
    shutil.rmtree(DST, ignore_errors=True)
for name, args in CASES.items():
    if only and name not in only:
        continue
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
