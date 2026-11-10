#!/usr/bin/env python3
"""ORCA cc-pVTZ examples: results.h5 passes its checks (#942).

    tests/resultsh5/orca_segmented_test.py <ecce-results>

ORCA's cc-pVTZ is segmented where ECCE stores the general contraction; the
stored copies carry ORCA's BASISPATTERN, so `ecce-results write` must flatten
the basis the coefficients belong to: tr(PS) = N and "checks ok" for each.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EX = os.path.join(ROOT, "data", "client", "examples")
CALCS = [("computational/h2-dissociation/orca-h2_rhf_scan", "h2_rhf_scan", [2]),
         ("computational/h2-dissociation/orca-h2_uhf_scan", "h2_uhf_scan", [1, 1]),
         ("computational/h2-dissociation/orca-h2_uhf_scan_out", "h2_uhf_scan_out", [1, 1]),
         ("computational/h2-dissociation/orca-h2_ccsd_scan", "h2_ccsd_scan", [2]),
         ("genchem/bond-formation/orca-h2_bond_curve", "h2_bond_curve", [2]),
         # def2: segmented in both, must stay as it was
         ("spectroscopy/carbonyl-compounds/orca-formaldehyde", "formaldehyde", [16])]


def main():
    cli = sys.argv[1]
    tmp = tempfile.mkdtemp(prefix="orca-seg-")
    env = dict(os.environ, HOME=tmp, ECCE_REALUSERHOME=tmp, ECCE_REALUSER="orcaseg")
    env.setdefault("ECCE_HOME", ROOT)
    fails = 0
    try:
        for folder, calc, want in CALCS:
            src = os.path.join(EX, folder, "stored", calc)
            dst = os.path.join(tmp, calc)
            shutil.copytree(src, dst)
            out = os.path.join(tmp, calc + ".h5")
            subprocess.run([cli, "write", dst, out], env=env, capture_output=True)
            show = subprocess.run([cli, "show", out], env=env, capture_output=True,
                                  text=True).stdout
            line = next((l for l in show.splitlines() if l.startswith("checks")), "")
            got = [float(x) for x in re.findall(r"tr\(PS\) ([-\d.]+)", line)]
            ok = (line.startswith("checks ok") and len(got) == len(want) and
                  all(abs(g - w) < 1e-4*w for g, w in zip(got, want)))
            print("%s %s: %s" % ("ok:  " if ok else "FAIL:", calc, line or "no checks line"))
            fails += not ok
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("RESULT", "FAIL" if fails else "OK")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
