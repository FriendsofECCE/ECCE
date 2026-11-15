#!/usr/bin/env python3
"""ORCA basis named only on the route card still gets a results.h5 (#1055).

    tests/resultsh5/routecard_basis_test.py <ecce-results>

The fixture is a real ORCA 6.1.1 HF/def2-SVP water run with PrintBasis and no
stored basis; the writer takes the basis from the output's printed block and
the file must pass tr(PS) = N and C'SC = 1.  Without the block it must refuse.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FIX = os.path.join(ROOT, "tests", "resultsh5", "fixtures", "calc-orca-water-routecard")


def main():
    cli = sys.argv[1]
    tmp = tempfile.mkdtemp(prefix="routecard-")
    env = dict(os.environ, HOME=tmp, ECCE_REALUSERHOME=tmp, ECCE_REALUSER="routecard")
    env.setdefault("ECCE_HOME", ROOT)
    fails = 0
    try:
        calc = os.path.join(tmp, "calc")
        shutil.copytree(FIX, calc)
        h5 = os.path.join(tmp, "o.h5")
        subprocess.run([cli, "write", calc, h5], env=env, capture_output=True)
        show = subprocess.run([cli, "show", h5], env=env, capture_output=True,
                              text=True).stdout
        line = next((l for l in show.splitlines() if l.startswith("checks")), "")
        m = re.search(r"tr\(PS\) ([-\d.]+)", line)
        ok = line.startswith("checks ok") and m and abs(float(m.group(1)) - 10) < 1e-4
        print("%s printed basis: %s" % ("ok:  " if ok else "FAIL:", line or "no file"))
        fails += not ok

        out = os.path.join(calc, "Outputs", "orca.orcaout")
        text = open(out).read().replace("BASIS SET IN INPUT FORMAT", "BASIS SET")
        open(out, "w").write(text)
        h5b = os.path.join(tmp, "n.h5")
        subprocess.run([cli, "write", calc, h5b], env=env, capture_output=True)
        ok = not os.path.exists(h5b)
        print("%s no printed basis: no file written" % ("ok:  " if ok else "FAIL:"))
        fails += not ok
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("RESULT", "FAIL" if fails else "OK")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
