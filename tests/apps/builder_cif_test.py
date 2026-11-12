#!/usr/bin/env python3
"""The Builder opens a CIF file (#358): alpha-quartz (P3_121, Z = 3) gives
9 atoms and the cell a = b = 4.9134, c = 5.4052 A, gamma = 120, read back
through the script hook's info command.

Exit status 77 (skip) without Xvfb or the built Builder.
"""
import argparse
import math
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_edit_bugs_test as eb  # noqa: E402
import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = eb.check


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "builder"), os.X_OK):
        harness.skip("builder is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "cif", {}, local=True, keep=args.keep)
    for exe in bo.GUI + ("autosym", "cleansym", "genmollat"):
        if os.path.exists(os.path.join(build, exe)):
            harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        cif = os.path.join(REPO, "tests", "fixtures", "cif", "quartz.cif")
        text = eb.runBuilder(s, env, wrappers, "cif1",
                             ["wait 3000", "info", "quit"], None,
                             structure=cif)
        m = re.search(r"BUILDER: atoms=(\d+) lattice=(\w+)", text)
        ok = check(m and m.group(1) == "9" and m.group(2) == "yes",
                   "quartz.cif opens with 9 atoms and a lattice (%s)"
                   % ((m.groups() if m else None),))
        vec = [tuple(float(x) for x in v) for v in re.findall(
            r"BUILDER: lattice_vector=(\S+) (\S+) (\S+)", text)]
        if check(len(vec) == 3, "three lattice vectors"):
            ln = [math.sqrt(sum(c * c for c in v)) for v in vec]
            gam = math.degrees(math.acos(
                sum(a * b for a, b in zip(vec[0], vec[1])) / (ln[0] * ln[1])))
            check(abs(ln[0] - 4.9134) < 2e-3 and abs(ln[1] - 4.9134) < 2e-3
                  and abs(ln[2] - 5.4052) < 2e-3 and abs(gam - 120) < 0.01,
                  "cell %.4f %.4f %.4f gamma %.2f" % (ln[0], ln[1], ln[2], gam))
        if not ok:
            say("\n".join("      | " + l for l in text.splitlines()[-30:]))
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(eb.failures) if eb.failures else "PASSED")
    return 1 if eb.failures else 0


if __name__ == "__main__":
    sys.exit(main())
