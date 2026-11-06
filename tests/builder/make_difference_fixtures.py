#!/usr/bin/env python3
"""Local-data-mode calculations for the density-difference tests (#726),
from real ECCE-QM runs, all cc-pVTZ so the fragments share the complex's
basis.  Run from the repository root with ecce-qm findable
(ECCE_QM=build-cmake/src/qm/ecce-qm) and ECCE_BASIS_DIR=data/admin/basissets:

  h2    RHF, H-H 0.74 A along z
  h2-a  UHF doublet, one H at the first position of h2
  h2-b  UHF doublet, one H at the second position of h2
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
DST = os.path.join(HERE, "difference-fixtures")
CASES = {"h2": ["h2", "--basis", "cc-pVTZ"],
         "h2-a": ["h2-a", "--basis", "cc-pVTZ", "--multiplicity", "2", "--method", "uhf"],
         "h2-b": ["h2-b", "--basis", "cc-pVTZ", "--multiplicity", "2", "--method", "uhf"]}

shutil.rmtree(DST, ignore_errors=True)
for name, args in CASES.items():
    work = tempfile.mkdtemp(prefix="difference-fixture-")
    try:
        subprocess.check_call(
            [sys.executable, os.path.join(ROOT, "tests", "modiagram", "makecalc.py"),
             "ecceqm", args[0], "0", os.path.join(work, "raw"), "--no-rotate",
             "--spherical-flag"] + args[1:] + (["--method", "hf"] if "--method" not in args else []))
        conv.SRC = work
        conv.convert(name, "raw", os.path.join(DST, name))
    finally:
        shutil.rmtree(work, ignore_errors=True)
print("ok")
