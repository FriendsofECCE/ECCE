#!/usr/bin/env python3
"""ELF, LOL and NCI as 3-D views in the Builder (#799, #800), headless.

    tests/apps/density_fields_3d_test.py

Opens the S22 water dimer (ECCE-QM, RHF 6-31G*, tests/tdat/densityfields-
fixtures) and draws each view as the MOs panel does (scene command `field`).
Each must build a surface (triangles in the exported isosurface).  NCI must
also show the hydrogen bond: on the s = 0.5 shell between the donor H and the
acceptor O the grid holds sign(lambda2)rho < 0 (blue), and the colour bar
carries the three words.  Same installed tree and isolation as the other
apps tests (ECCE_TEST_HOME, ECCE_TEST_WRAPPERS); exit 77 without them.
"""
import argparse
import math
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402
import orca_mo_test as omo   # noqa: E402

DIMER = os.path.join(ROOT, "tests", "tdat", "densityfields-fixtures", "dimer")
RES = 44
# Acceptor O4 and the donor H3 of the S22 water dimer (Angstrom), as in chemsys.mvm.
O_ACC = (1.350625, 0.111469, 0.0)
H_DON = (-0.599677, 0.040712, 0.0)
failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-25:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def triangles(path):
    if not os.path.exists(path):
        return -1
    return sum(1 for l in open(path) if l.startswith("tri "))


def hbondColours(dump):
    """sign(lambda2)rho of the nodes on the s = 0.5 shell between H and O."""
    rows = open(dump).read().split("\n")
    n = [int(v) for v in rows[0].split()[:3]]
    o = [float(v) for v in rows[0].split()[3:6]]
    c = [float(v) for v in rows[0].split()[6:9]]
    step = [(c[k] - o[k]) / (n[k] - 1) for k in range(3)]
    found = []
    r = 1
    for ix in range(n[0]):
        for iy in range(n[1]):
            for iz in range(n[2]):
                s, col = (float(v) for v in rows[r].split())
                r += 1
                p = (o[0] + ix * step[0], o[1] + iy * step[1], o[2] + iz * step[2])
                # within 0.45 A of the H...O line, not near either nucleus
                v = [O_ACC[k] - H_DON[k] for k in range(3)]
                w = [p[k] - H_DON[k] for k in range(3)]
                L2 = sum(x * x for x in v)
                t = sum(v[k] * w[k] for k in range(3)) / L2
                if not 0.2 < t < 0.8:
                    continue
                d = math.sqrt(sum((w[k] - t * v[k]) ** 2 for k in range(3)))
                if d < 0.45 and abs(s - 0.5) < 0.15 and abs(col) < 0.05:
                    found.append(col)
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    options = parser.parse_args()
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    settings = isolate.apply(apps.INSTALL)
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="fields3d-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x768x24"
    try:
        with xdisplay.Display(screen="1366x768x24") as display:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                           env=display.env(), timeout=180)
            try:
                url = omo.makeCalc(data, "dimer", source=DIMER)
                dump = os.path.join(work, "nci.dump")
                for name in ("elf", "lol", "nci"):
                    out = os.path.join(work, name)
                    scene = "field %s %d\nidleframes 6\nexportiso %s\n" % (name, RES, name)
                    if name == "nci":
                        scene += "espdump %s\n" % dump
                    res = omo.runBuilder(display, url, out, scene)
                    n = triangles(os.path.join(out, name + "-iso.txt"))
                    if n <= 0:
                        why = os.path.join(out, "FAILED")
                        fail(name, "no surface (%d triangles) %s" % (n, open(why).read()
                             if os.path.exists(why) else ""), res.log)
                    else:
                        print("ok    %s: %d triangles" % (name, n))
                if os.path.exists(dump):
                    cols = hbondColours(dump)
                    if len(cols) < 5 or min(cols) > -0.005:
                        fail("nci hydrogen bond",
                             "%d shell nodes between H and O, most negative "
                             "sign(l2)rho %s; want blue (< -0.005)"
                             % (len(cols), min(cols) if cols else None))
                    else:
                        print("ok    nci: H-bond shell sign(l2)rho down to %.4f "
                              "(%d nodes)" % (min(cols), len(cols)))
                else:
                    fail("nci", "no grid dump")
                # The panel: the entries, the bar words, the cut-off control.
                out = os.path.join(work, "panel")
                scene = ("mofield NCI (3D)\nmopanel nci\nhold 3\nmobar nci\n"
                         "mofield ELF (3D)\nmopanel elf\nmobar elf\n")
                res = omo.runBuilder(display, url, out, scene,
                                     extraEnv={"ECCE_PANEL_FULLSCREEN": "1"})
                nci = omo.readState(out, "nci") or {}
                elf = omo.readState(out, "elf") or {}
                info = nci.get("espbar_info", "")
                if nci.get("espbar_shown") != "1" or not all(
                        w in info for w in ("blue", "green", "red")):
                    fail("nci colour bar", "state %r" % nci, res.log)
                elif nci.get("espbar_min", "").strip() != "-0.0350" or \
                        nci.get("espbar_max", "").strip() != "+0.0200":
                    fail("nci colour bar", "range %s .. %s" % (
                        nci.get("espbar_min"), nci.get("espbar_max")))
                else:
                    print("ok    nci colour bar: %s" % info.replace(" | ", " / "))
                if elf.get("espbar_shown") == "1":
                    fail("elf colour bar", "shown for ELF")
            finally:
                subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                               env=display.env(), timeout=120)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
