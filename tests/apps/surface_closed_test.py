#!/usr/bin/env python3
"""MOs panel isosurfaces are closed at a high isovalue (#1076).

ECCE-QM water dimer (General Chemistry, Intermolecular interactions): the
total density drawn at isovalue 0.5 (small surfaces around the oxygen
nuclei) and 0.05.  Scene command "surfmesh" lists each surface's triangles;
a closed surface has no boundary edge (every edge used by two triangles).
--shot DIR saves both on a white background.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps                  # noqa: E402
import isolate               # noqa: E402
import orca_mo_test as base  # noqa: E402
import xdisplay              # noqa: E402

DIMER = os.path.join(REPO, "data", "client", "examples", "genchem",
                     "intermolecular-interactions", "ecceqm-h2o_dimer",
                     "stored", "h2o_dimer")
#  (tag, log10 isovalue)
CASES = (("rho-0.5", -0.30103), ("rho-0.05", -1.30103))


def scene(iso, shot):
    lines = ["mofield Density", "mopanel run", "moiso %g" % iso,
             "surfmesh mesh"]
    if shot:
        lines.append("background 1 1 1")
        lines.append("saveimage %s 2 white" % shot)
    return "\n".join(lines) + "\n"


def mesh(out):
    path = os.path.join(out, "mesh.txt")
    if not os.path.exists(path):
        return None
    surfaces, grid = [], None
    for line in open(path):
        f = line.split()
        if f and f[0] == "surface":
            surfaces.append(dict(zip(f[2::2], map(float, f[3::2]))))
        elif f and f[0] == "grid":
            grid = f[1:]
    return surfaces, grid


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--shot", help="save PNGs here")
    ap.add_argument("--res", type=int, help="grid points per axis")
    opts = ap.parse_args()
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    if opts.res:
        prefs = os.path.join(os.environ["ECCE_REALUSERHOME"], ".ECCE")
        os.makedirs(prefs, exist_ok=True)
        with open(os.path.join(prefs, "wxbuilder.ini"), "a") as handle:
            handle.write("[Grid]\nx=%d\ny=%d\nz=%d\nuniform=1\n"
                         % ((opts.res,) * 3))
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="surface-closed-",
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
            url = base.makeCalc(data, "h2o_dimer", source=DIMER)
            for tag, iso in CASES:
                shot = None
                if opts.shot:
                    os.makedirs(opts.shot, exist_ok=True)
                    shot = os.path.abspath(os.path.join(opts.shot,
                                                        tag + ".png"))
                out = os.path.join(work, tag)
                start = time.time()
                res = base.runBuilder(display, url, out, scene(iso, shot))
                took = time.time() - start
                got = mesh(out)
                if res.crashed or not got or not got[0]:
                    print("FAIL  %s: no surface" % tag)
                    print((res.log or "")[-1500:])
                    failures.append(tag)
                    continue
                surfaces, grid = got
                for s in surfaces:
                    ok = (s["boundary"] == 0 and s["overused"] == 0
                          and s["misoriented"] == 0 and s["triangles"] > 0)
                    print("%s %s: threshold %.4g, %d vertices, %d triangles, "
                          "%d boundary, %d overused, %d misoriented edges; "
                          "grid %s; builder run %.1f s"
                          % ("ok   " if ok else "FAIL ", tag, s["threshold"],
                             s["vertices"], s["triangles"], s["boundary"],
                             s["overused"], s["misoriented"],
                             " ".join(grid or ["?"]), took))
                    if not ok:
                        failures.append(tag)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
