#!/usr/bin/env python3
"""results.h5 and the Density Map (#690), headless.

    tests/apps/results_h5_test.py            own Xvfb, services, local data

An ORCA calculation (calc-orca-water-ccpvdz) with no results.h5, as stored
before ECCE wrote one, is opened in the Builder: the Density Map reads the
stored properties. `ecce-results write` then makes the file (checks passed),
and the same Density Map reads results.h5 and shows the same plane (same
colour range).
(A file marked failed is ignored: results_calc_test covers that.)

The end-to-end write at the end of a real job store is checked by
tests/launch/ecceqm_test.py (launch_ecceqm).

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS). Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                   # noqa: E402
import isolate                # noqa: E402
import orca_mo_test as base   # noqa: E402
import xdisplay               # noqa: E402

SCENE = "hold 2\nlist Density Map\nhold 4\ndmstate\n"


def state(log):
    """(orbitals source, colour range) of the last dmstate."""
    blocks = re.findall(r"DENSITYMAP: dmstate\n((?:.*\n)*?range .*)", log)
    if not blocks:
        return None, None
    last = blocks[-1]
    src = re.search(r"^orbitals (.*)$", last, re.M)
    rng = re.search(r"^range (\S+) (\S+)", last, re.M)
    return (src.group(1) if src else None,
            (float(rng.group(1)), float(rng.group(2))) if rng else None)


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")) or \
       not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-results")):
        print("SKIP: no ecce-builder / ecce-results under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="results-h5-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1700x1000x24"
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    def densityMap(display, url, name):
        out = os.path.join(work, name)
        res = base.runBuilder(display, url, out, SCENE, panel="MOs")
        check(not res.crashed, "%s: the Builder did not crash" % name)
        return state(res.log or "")

    cli = os.path.join(apps.WRAPPERS, "ecce-results")
    try:
        with xdisplay.Display() as display:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                           env=display.env(), timeout=180)
            url = base.makeCalc(data, "water")
            calc = url[len("file://"):].rstrip("/")
            #  the fixture is trimmed to Props and Parameters; a stored
            #  calculation has its Outputs folder
            os.makedirs(os.path.join(calc, "Outputs"), exist_ok=True)
            h5 = os.path.join(calc, "Outputs", "results.h5")
            if os.path.exists(h5):
                os.remove(h5)

            src0, rng0 = densityMap(display, url, "before")
            check(src0 == "properties" and rng0 is not None,
                  "no results.h5: the Density Map reads the properties (%s, %s)" % (src0, rng0))

            r = subprocess.run([cli, "write", calc], capture_output=True, text=True, timeout=300)
            print(r.stdout + r.stderr)
            check(r.returncode == 0 and os.path.exists(h5) and "checks passed" in r.stdout,
                  "ecce-results write stored results.h5 with passing checks")
            show = subprocess.run([cli, "show", h5], capture_output=True, text=True).stdout
            check(re.search(r"^checks ok: ", show, re.M) is not None,
                  "the file carries checks = ok")

            src1, rng1 = densityMap(display, url, "after")
            check(src1 == "results.h5", "results.h5 present: the Density Map reads it (%s)" % src1)
            same = (rng0 and rng1 and
                    all(abs(a - b) <= 1e-6 * max(abs(a), abs(b), 1e-12) for a, b in zip(rng0, rng1)))
            check(bool(same), "same plane from both: colour range %s against %s" % (rng0, rng1))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
