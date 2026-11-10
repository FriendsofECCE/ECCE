#!/usr/bin/env python3
"""Calculations and molden files for the plane-integral check (#749), from
real ECCE-QM runs.  Run from the repo root with
ECCE_QM=<build>/src/qm/ecce-qm and ECCE_BASIS_DIR=data/admin/basissets:

  make_planeintegral_fixtures.py

  c2h4  RHF (ethene, pi orbital)      hf  RHF (HF, bond along x)

Both go to tests/tdat/planeintegral-fixtures: NAME/ is the calculation the
lineprofile tool reads, NAME.molden the same wavefunction for the oracle.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "builder"))
import make_densitymap_fixtures as conv   # noqa: E402

DST = os.path.join(HERE, "planeintegral-fixtures")
qm = os.environ.get("ECCE_QM", "ecce-qm")

for name, mol in (("c2h4", "c2h4"), ("hf", "hf")):
    work = tempfile.mkdtemp(prefix="planeintegral-fixture-")
    try:
        out = subprocess.check_output(
            [sys.executable, os.path.join(ROOT, "tests", "modiagram", "makecalc.py"),
             "ecceqm", mol, "0", os.path.join(work, "raw"), "--no-rotate",
             "--method", "hf", "--multiplicity", "1", "--spherical-flag", "--keep"],
            universal_newlines=True)
        kept = re.search(r"workdir kept: (\S+)", out).group(1)
        conv.SRC = work
        conv.convert(name, "raw", os.path.join(DST, name))
        subprocess.check_call([qm, "--molden", os.path.join(DST, name + ".molden"),
                               "-o", os.devnull, "ecceqm.qmin"], cwd=kept)
        shutil.rmtree(kept, ignore_errors=True)
    finally:
        shutil.rmtree(work, ignore_errors=True)
print("ok")
