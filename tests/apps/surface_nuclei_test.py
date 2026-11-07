#!/usr/bin/env python3
"""Density isosurface grid against ecce-density-field at every nucleus (#892).

Fixture calc-orca-methanol-water-tzvpd: ORCA HF/def2-TZVPD, methanol then
water (the water last in the atom list), made by make_surface_fixture.py.
The methanol atoms are selected before Compute, as a user picking atoms
does: the grid must still cover every atom.

  walk     no results.h5, the per-atom walk over the stored basis
           (coefficient cutoff 0.001, float sums): 1e-3 relative
  h5       results.h5 written by ecce-results: 1e-6 relative

In both, the grid point nearest each nucleus lies within half a grid step
of it on every axis, and the grid's density there equals
ecce-density-field --at that point.  --shot DIR saves the density surface
on a white background.

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
sys.path.insert(0, HERE)

import apps                  # noqa: E402
import isolate               # noqa: E402
import orca_mo_test as base  # noqa: E402
import xdisplay              # noqa: E402

FIXTURE = os.path.join(HERE, "fixtures", "calc-orca-methanol-water-tzvpd")
SCENE = ("select 1 2 3 4 5 6\nmofield Density\nmopanel dens\n"
         "gridnuclei nuc\n")


def nuclei(out):
    rows = []
    path = os.path.join(out, "nuc.txt")
    if not os.path.exists(path):
        return rows
    for line in open(path):
        f = line.split()
        if len(f) == 9:
            rows.append((int(f[0]), f[1], tuple(map(float, f[2:5])),
                         float(f[5]), tuple(map(float, f[6:9]))))
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--shot", help="save the density surface PNG here")
    ap.add_argument("--calc", help="another ORCA calculation directory "
                    "(copied; per-atom report, no pass/fail)")
    opts = ap.parse_args()
    for tool in ("ecce-builder", "ecce-results", "ecce-density-field"):
        if not os.path.exists(os.path.join(apps.WRAPPERS, tool)):
            print("SKIP: no %s under %s" % (tool, apps.WRAPPERS))
            return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="surface-nuclei-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x768x24"
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    def field(calc, points):
        arg = []
        for p in points:
            arg += ["--at", "%.9f,%.9f,%.9f" % p]
        r = subprocess.run([os.path.join(apps.WRAPPERS, "ecce-density-field"),
                            calc] + arg, capture_output=True, text=True,
                           timeout=300)
        rows = [l.split() for l in r.stdout.splitlines()
                if l.strip() and not l.startswith("#")]
        return [float(f[3]) for f in rows]

    def compare(display, url, calc, name, tol, extra, shot=None, report=False):
        out = os.path.join(work, name)
        scene = SCENE + ("background 1 1 1\n" if shot else "")
        res = base.runBuilder(display, url, out, scene, shot=shot,
                              extraEnv=extra)
        rows = nuclei(out)
        check(not res.crashed and rows, "%s: the Builder drew the density "
              "and reported %d nuclei" % (name, len(rows)))
        if not rows:
            print((res.log or "")[-3000:])
            return
        ref = field(calc, [r[2] for r in rows])
        check(len(ref) == len(rows), "%s: ecce-density-field answered" % name)
        worst, outside = 0.0, []
        for (a, sym, p, v, n), e in zip(rows, ref):
            rel = abs(v - e) / max(abs(e), 1e-12)
            worst = max(worst, rel)
            far = max(abs(p[k] - n[k]) for k in range(3))
            if far > 0.3:
                outside.append("%s%d" % (sym, a))
            print("      %s%-3d grid %.6g  field %.6g  rel %.2e  %.2f A off"
                  % (sym, a, v, e, rel, far))
        if report:
            return
        check(not outside, "%s: every nucleus inside the grid (%s)"
              % (name, " ".join(outside) or "all"))
        check(worst <= tol, "%s: grid density at the nuclei = "
              "ecce-density-field, worst %.2e (tolerance %g)"
              % (name, worst, tol))

    cli = os.path.join(apps.WRAPPERS, "ecce-results")
    try:
        with xdisplay.Display() as display:
            subprocess.run([os.path.join(apps.INSTALL, "bin",
                                         "ecce-gateway-start")],
                           env=display.env(), timeout=180)
            if opts.calc:
                url = base.makeCalc(data, "other", source=opts.calc)
                calc = url[len("file://"):].rstrip("/")
                compare(display, url, calc, "other", 0, {}, report=True,
                        shot=opts.shot and os.path.join(opts.shot,
                                                        "other.png"))
                return 0
            url = base.makeCalc(data, "mw", source=FIXTURE)
            calc = url[len("file://"):].rstrip("/")
            os.makedirs(os.path.join(calc, "Outputs"), exist_ok=True)
            compare(display, url, calc, "walk", 1e-3,
                    {"ECCE_SURFACE_PROPS": "1"})
            r = subprocess.run([cli, "write", calc], capture_output=True,
                               text=True, timeout=300)
            check(r.returncode == 0 and "checks passed" in r.stdout,
                  "ecce-results write: results.h5 with passing checks")
            compare(display, url, calc, "h5", 1e-6, {},
                    shot=opts.shot and os.path.join(opts.shot,
                                                    "density.png"))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
