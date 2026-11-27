#!/usr/bin/env python3
"""Binding pockets to the Vina search box through the Builder script hook (#1320).

1IEP chain A with imatinib is stored as a Vina calculation.  The Builder
script runs `findpockets` (one line per pocket) and `vinabox pocket=1` (the
box set from the best pocket, as a selection would set it), and the box must
sit on the imatinib site.  No Vina run is needed, so the test is short.

    tests/apps/pocket_box_test.py [--build build-cmake] [--pdb 1iep.pdb] [--keep]

Exit status 77 (skip) without Xvfb, the PDB entry or the build.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import checks  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402
from vina_viewer_test import complexPdb  # noqa: E402

CHECKS = checks.Checks(7, "pocket box")
check = CHECKS


def ligandCentroid(pdb):
    xyz = [(float(l[30:38]), float(l[38:46]), float(l[46:54])) for l in open(pdb)
           if l.startswith("HETATM") and l[17:20] == "STI" and l[21] == "A"]
    return [sum(p[k] for p in xyz) / len(xyz) for k in range(3)]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--pdb", default=os.path.join(REPO, "tests", "dock", "fixtures", "1iep.pdb"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    if not os.path.exists(args.pdb):
        harness.skip("the PDB entry %s is not here" % args.pdb)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "pocketbox", {"Vina": shutil.which("vina") or "/bin/true"},
                        local=True, keep=args.keep)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    gw = os.path.join(REPO, "packaging", "gateway")
    for entry in os.listdir(gw):
        if entry.startswith("ecce-"):
            harness.link(os.path.join(gw, entry), os.path.join(s.home, "bin", entry))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    os.makedirs(work, exist_ok=True)
    os.makedirs(rundir, exist_ok=True)
    cplx = os.path.join(work, "1iep-a.pdb")
    complexPdb(args.pdb, cplx)
    want = ligandCentroid(args.pdb)

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return CHECKS.report()
        name = "pocketbox-%d" % int(time.time())
        stub = os.path.join(work, name + ".vinain")
        open(stub, "w").write("# placeholder\n")
        url, out = s.create(name, "vina_es", stub, "vina.vinain", rundir,
                            extra=("procs=2", "code=Vina", "pdb=" + cplx))
        if not check(url is not None, "calculation created"):
            say(out)
            return CHECKS.report()
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita"})
        text, _shots = bo.runScript(
            s, display, env, wrappers, ["-context", url],
            ["wait 8000",
             "findpockets",
             "vinabox pocket=99",
             "vinabox pocket=1",
             "vinaboxinfo",
             "vinabox pocket=1 3",
             "quit"], os.path.join(s.state, "png"), "pocketbox")
        for line in text.splitlines():
            if "ECCE_BUILDER_SCRIPT: " in line and ("pocket" in line or "vinabox" in line):
                say("    " + line.strip()[:260])
        fp = re.findall(r"ECCE_BUILDER_SCRIPT: findpockets: pockets=(\d+) ; (.*)", text)
        rows = fp[0][1].split(" ; ") if fp else []
        check(bool(fp) and int(fp[0][0]) == len(rows) >= 1,
              "findpockets lists a line per pocket: %s" % (fp[0][0] if fp else "absent"))
        check(bool(rows) and re.match(r"#1 score [\d.]+ volume \d+ centre .* extent .* lining \d+", rows[0]),
              "the line carries rank, score, volume, centre, extent, lining")
        bad = re.findall(r"vinabox pocket=99: (.*)", text)
        check(bool(bad) and bad[0].startswith("FAIL: there is no pocket 99"),
              "a pocket that does not exist fails: %s" % (bad[0] if bad else "absent"))
        box = re.findall(r"vinaboxinfo: drawn=(\d) auto=(\d) centre (\S+) (\S+) (\S+) size (\S+) (\S+) (\S+)", text)
        check(bool(box) and box[0][0] == "1" and box[0][1] == "0",
              "the box is drawn and no longer automatic")
        if box:
            c = [float(v) for v in box[0][2:5]]
            dist = sum((c[k] - want[k]) ** 2 for k in range(3)) ** 0.5
            check(dist < 3.0, "box centre %.1f A from the imatinib centroid" % dist)
            check(all(8 < float(v) < 40 for v in box[0][5:8]),
                  "box size follows the pocket: %s" % " ".join(box[0][5:8]))
        else:
            check(False, "box centre near imatinib")
            check(False, "box size follows the pocket")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    return CHECKS.report()


if __name__ == "__main__":
    sys.exit(main())
