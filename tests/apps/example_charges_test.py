#!/usr/bin/env python3
"""The MOPAC ammonia example's charges and MOs open in the Builder (#429).

    tests/apps/example_charges_test.py [--build build-cmake] [--png DIR]

The example output (its .mgf beside it) is imported through the Organizer's
own import (ECCE_TEST_CALCIMPORT), then the Builder opens it and Mulliken
Charges, then MOs, are chosen in the Properties list (Builder script
"list").  The viewer must show the charges (red nitrogen, blue hydrogens)
and the Builder must still be running at the end.

Exit status 77 (skip) without Xvfb, import, python3-pil/numpy or the built
binaries.
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
OUTPUT = os.path.join(REPO, "data", "client", "examples", "general", "mopac-ammonia",
                      "ammonia.out")
CHARGES = "Mulliken Charges"


def red(path):
    """Viewer pixels of the charge red (lit and shaded)."""
    from PIL import Image
    import numpy
    img = Image.open(path)
    box = bo.viewerBox(img)
    if box is None:
        return -1
    p = numpy.asarray(img.convert("RGB").crop(box)).astype(int)
    r, g, b = p[..., 0], p[..., 1], p[..., 2]
    return int(((r > 120) & (r > g + 60) & (r > b + 60)).sum())


def importExample(s, env, wrappers):
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
        if not shutil.which(tool):
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

    s = harness.Session(build, "example-charges", {}, local=True,
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
        url = importExample(s, env, wrappers)
        if bo.check(url is not None, "the ammonia example imports: %s" % url):
            lines = ["wait 4000", "info", "shot plain",
                     "list %s" % CHARGES, "wait 1500", "overlay",
                     "shot charges",
                     "list MOs", "wait 3000", "overlay", "quit"]
            text, shots = bo.runScript(s, display, env, wrappers,
                                       ["-context", url], lines, png,
                                       "nh3", {"ECCE_PANEL_MODE": "detail"})
            states = bo.overlays(text)
            bo.check(bool(states) and bo.focused(states[0]) == [CHARGES],
                     "%s has the viewer: %s" % (
                         CHARGES, bo.focused(states[0]) if states else "-"))
            #  Plain (Jmol colours): blue nitrogen, white hydrogens.  Charges:
            #  red (negative) nitrogen, blue (positive) hydrogens.
            for name, wantRed in (("plain", False), ("charges", True)):
                if bo.check(name in shots, "%s photographed" % name):
                    n = red(shots[name])
                    bo.check((n > 300) == wantRed,
                             "%s: %d red pixels in the viewer" % (name, n))
            if "charges" in shots:
                n = bo.tinted(shots["charges"], "blue")
                bo.check(n > 30, "charges: %d blue pixels in the viewer" % n)
            #  The import stored an empty ORBSYM element; reading it ended
            #  the Builder with SIGSEGV.
            bo.check(len(states) == 2 and "ECCE_BUILDER_SCRIPT: quit" in text
                     and "ended by SIG" not in text,
                     "the Builder survives %s and then MOs" % CHARGES)
            if bo.failures:
                say(text[-3000:])
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(bo.failures) if bo.failures else "PASSED")
    return 1 if bo.failures else 0


if __name__ == "__main__":
    sys.exit(main())
