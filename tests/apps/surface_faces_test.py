#!/usr/bin/env python3
"""MOs panel surfaces are not cut at the grid faces (#892).

ECCE-QM N2 (canonical MOs 1-8) and Li2 (canonical 1-3, localised 1-3) from
the General Chemistry diatomics examples.  After each Compute at the
default isovalue, no value on the six outer faces of the grid exceeds the
isovalue the panel drew (|field|, so both signs of an MO) -- the box grows
until it does -- unless the grid has reached the 40 A limit.  --shot DIR
saves N2 MO 3 and the Li2 localised MOs on a white background.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps                  # noqa: E402
import isolate               # noqa: E402
import orca_mo_test as base  # noqa: E402
import xdisplay              # noqa: E402

DIATOMICS = os.path.join(REPO, "data", "client", "examples", "genchem",
                         "molecular-orbitals-of-diatomics")
N2 = os.path.join(DIATOMICS, "ecceqm-N2", "stored", "N2")
LI2 = os.path.join(DIATOMICS, "ecceqm-Li2", "stored", "Li2")
SULFATE = os.path.join(REPO, "data", "client", "examples", "spectroscopy",
                       "sulfate-coordination", "orca-co_nh3_5_oso3", "stored",
                       "co_nh3_5_oso3")
#  (name, calculation, localised, MO numbers, field, log10 isovalue or None)
CASES = (("n2", N2, False, range(1, 9), "MO", None),
         ("li2", LI2, False, range(1, 4), "MO", None),
         ("li2loc", LI2, True, range(1, 4), "Localised MO", None),
         #  A 20-atom complex: the density at a low isovalue reaches far
         #  past the default box, and the lowered slider must close it.
         ("cluster-rho", SULFATE, False, (0,), "Density", -3.0),
         ("cluster-rho-low", SULFATE, False, (0,), "Density", -4.0))
MAX_EDGE = 40.0


def scene(field, mo, shot, iso=None):
    lines = ["mofield " + field]
    if mo:
        lines.append("moselect %d" % mo)
    lines.append("mopanel run")
    if iso is not None:
        lines.append("moiso %g" % iso)
    lines.append("gridfaces faces")
    if shot:
        lines.append("background 1 1 1")
    return "\n".join(lines) + "\n"


def faces(out):
    path = os.path.join(out, "faces.txt")
    if not os.path.exists(path):
        return None
    got = {}
    for line in open(path):
        f = line.split()
        if f:
            got[f[0]] = list(map(float, f[1:]))
    return got if {"iso", "edge", "faces"} <= set(got) else None


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--shot", help="save PNGs here")
    opts = ap.parse_args()
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="surface-faces-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x768x24"
    failures = []
    try:
        with xdisplay.Display() as display:
            subprocess.run([os.path.join(apps.INSTALL, "bin",
                                         "ecce-gateway-start")],
                           env=display.env(), timeout=180)
            for name, source, loc, mos, field, iso in CASES:
                url = base.makeCalc(data, name, source=source)
                for mo in mos:
                    tag = "%s-mo%d" % (name, mo)
                    shot = None
                    if opts.shot and ((name == "n2" and mo == 3) or loc):
                        shot = os.path.join(opts.shot, tag + ".png")
                    out = os.path.join(work, tag)
                    res = base.runBuilder(display, url, out,
                                          scene(field, mo, shot, iso), shot=shot)
                    got = faces(out)
                    if res.crashed or got is None:
                        print("FAIL  %s: no result" % tag)
                        failures.append(tag)
                        print((res.log or "")[-1500:])
                        continue
                    edge, fc = got["edge"], got["faces"]
                    worst = max(fc)
                    iso = got["iso"][0]
                    ok = worst <= iso or max(edge) >= MAX_EDGE - 0.01
                    print("%s %s: isovalue %.4g, edges %s A, largest face "
                          "value %.4g" % ("ok   " if ok else "FAIL ", tag, iso,
                                          "x".join("%.1f" % e for e in edge),
                                          worst))
                    if not ok:
                        failures.append(tag)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
