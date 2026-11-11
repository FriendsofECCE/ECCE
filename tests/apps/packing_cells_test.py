#!/usr/bin/env python3
"""A cell shows its boundary atoms at every equivalent position (#859).

    tests/apps/packing_cells_test.py [--build build-cmake] [--png DIR]

The Packing in solids examples are imported through the Organizer and opened
in the Builder.  The Builder script's "info" reports the atoms of the
fragment and the display-only cell images; the images must not change the
atom count.  Spheres drawn: sc 8, bcc 9, fcc 14.  With --png, each crystal
is photographed in Ball And Stick and in CPK on white.  Exit status 77
(skip) without Xvfb, import, python3-pil/numpy or the built binaries.
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
import harness  # noqa: E402
from harness import say  # noqa: E402

GUI = ("builder", "organizer")
EX = os.path.join(REPO, "data", "client", "examples", "genchem")
PK = os.path.join(EX, "packing-in-solids")
# name, atoms in the fragment, images drawn beside them; a bonded periodic
# structure (a water molecule in a box) keeps the centred box and has none
CASES = (("h2o_box", 3, 0), ("po_sc", 1, 7), ("na_bcc", 2, 7), ("al_fcc", 4, 10),
         ("cu_fcc", 4, 10), ("mg_hcp", 2, 7))


def importExample(s, env, wrappers, OUTPUT):
    """Imports the example; returns the calculation's URL or None."""
    log = os.path.join(s.state, "organizer-import.log")
    proc = subprocess.Popen(
        [os.path.join(wrappers, "ecce-organizer")],
        env=dict(env, ECCE_TEST_CALCIMPORT=OUTPUT, ECCE_TEST_IMPORTREPORT="1"),
        cwd=os.path.join(s.home, "bin"), stdout=open(log, "w"),
        stderr=subprocess.STDOUT, start_new_session=True)
    url = None
    deadline = time.time() + 300
    try:
        while time.time() < deadline:
            done = proc.poll() is not None
            with open(log, errors="replace") as handle:
                text = handle.read()
            m = re.search(r"ECCE_TEST_CALCIMPORT: selected (\S+)", text)
            if m and "ECCE_TEST_IMPORTREPORT: state" in text:
                url = m.group(1)
                break
            if done:
                break
            time.sleep(1)
    finally:
        bo.stop(proc)
    if not url:
        with open(log, errors="replace") as handle:
            say(handle.read()[-3000:])
    return url


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    try:
        import numpy  # noqa: F401
        from PIL import Image  # noqa: F401
    except ImportError:
        harness.skip("python3-pil and python3-numpy are needed")
    harness.prerequisites(build, ())
    for exe in GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "packing-cells", {}, local=True,
                        keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    say("binaries: %s" % ", ".join(
        os.path.realpath(os.path.join(s.home, "bin", e)) for e in GUI))

    display = None
    try:
        if not s.services(True):
            bo.check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        for name, natoms, nimages in CASES:
            output = (os.path.join(HERE, "fixtures", "pwout", name + ".pwout")
                      if name == "h2o_box" else
                      os.path.join(PK, "qe-" + name, name + ".pwout"))
            url = importExample(s, env, wrappers, output)
            if not bo.check(url is not None, "%s imports: %s" % (name, url)):
                continue
            lines = ["wait 4000", "info"]
            if args.png:
                lines += ["background 1 1 1", "rotatex 15", "rotate 20"]
                for style, tag in (("Ball And Stick", "bs"), ("CPK", "cpk")):
                    lines += ["style " + style, "viewall", "wait 800",
                              "shot " + tag]
            if name == "cu_fcc":
                # Replicate 2x2x2: 32 real atoms; the block's faces get
                # 2^z - 1 images per atom with z coordinates on a lower face.
                lines += ["pbc replicate 2 2 2", "wait 1500", "info"]
            lines += ["quit"]
            text, shots = bo.runScript(s, display, env, wrappers,
                                       ["-context", url], lines, png,
                                       "cell-" + name,
                                       {"ECCE_PANEL_MODE": "detail"})
            m = re.search(r"BUILDER: atoms=(\d+) .*\n(?:.*\n)*?BUILDER: cell_images=(\d+)", text)
            if bo.check(m is not None, "%s: info reported" % name):
                bo.check((int(m.group(1)), int(m.group(2))) == (natoms, nimages),
                         "%s: %s atoms and %s images, expected %d and %d"
                         % (name, m.group(1), m.group(2), natoms, nimages))
            if name == "cu_fcc":
                r = re.findall(r"BUILDER: atoms=(\d+) .*\n(?:.*\n)*?BUILDER: cell_images=(\d+)", text)
                got = tuple(int(x) for x in r[1]) if len(r) > 1 else None
                bo.check(got == (32, 31),
                         "cu_fcc replicated 2x2x2: atoms and images %s, expected (32, 31)" % (got,))
            # Crystals (no bonds) start with the box at the origin.
            c = re.search(r"lattice_centered=(\d) corner=(\S+) (\S+) (\S+)", text)
            if bo.check(c is not None, "%s: lattice reported" % name):
                crystal = name != "h2o_box"
                at0 = all(abs(float(c.group(i))) < 1e-3 for i in (2, 3, 4))
                bo.check(c.group(1) == ("0" if crystal else "1")
                         and at0 == crystal,
                         "%s: centred=%s, corner at the origin: %s"
                         % (name, c.group(1), at0))
            if bo.failures:
                say(text[-2000:])
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(bo.failures) if bo.failures else "PASSED")
    return 1 if bo.failures else 0


if __name__ == "__main__":
    sys.exit(main())
