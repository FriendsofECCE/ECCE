#!/usr/bin/env python3
"""The Builder's default tool panes stay docked on small screens, headless.

    tests/apps/tool_column_test.py [--png DIR] [--sizes 1024x600,...]

A calculation that has not been run opens with the building tools: Open
structures, Build, Coordinates or Selection, Symmetry and the Log.  On a
laptop screen these do not fit the right-hand column at their full height;
the last of them (Symmetry) used to get no height at all and its window was
drawn where it was created, over the 3-D viewer.  For each screen size and
the classic and list + detail layouts every shown tool pane must be docked,
a child of the frame, inside the window, at least MIN_HEIGHT high, and clear
of the viewer.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS); exit 77 (skip) without an install or Xvfb.
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps         # noqa: E402
import fixture      # noqa: E402
import pbc_ux_test  # noqa: E402

SIZES = ("1024x600", "1366x768", "1400x900")
MODES = ("classic", "detail")
TOOLS = ["Open_structures", "Build", "Coordinates", "Selection", "Symmetry",
         "Atom_Table"]
MIN_HEIGHT = 30


def rects(st):
    """{pane name: (x, y, w, h)} of every shown pane."""
    out = {}
    for line in st.get("raw", []):
        name = line.split('"')[1]
        x, y, w, h = (int(v) for v in line.split('"')[2].split()[1:5])
        out[name] = (x, y, w, h)
    return out


def problems(st, label):
    if st is None:
        return ["%s: no state written" % label]
    found = []
    sym = st["tools"].get("Symmetry")
    if not sym or not sym["shown"]:
        found.append("%s: Symmetry not shown (%s)"
                     % (label, ", ".join(st["shown"])))
    cw, ch = st["client"]
    viewer = rects(st).get("Viewer")
    for name, t in sorted(st["tools"].items()):
        if not t["shown"]:
            continue
        where = "%s, %s %dx%d+%d+%d" % (label, name, t["w"], t["h"],
                                        t["x"], t["y"])
        if t["floating"]:
            found.append(where + ": floating")
        if not t["child"]:
            found.append(where + ": window left the frame")
        if t["h"] < MIN_HEIGHT:
            found.append(where + ": no room in the column")
        if (t["x"] < 0 or t["y"] < 0 or t["x"] + t["w"] > cw or
                t["y"] + t["h"] > ch):
            found.append(where + ": outside the %dx%d window" % (cw, ch))
        if viewer:
            vx, vy, vw, vh = viewer
            if (t["x"] < vx + vw and vx < t["x"] + t["w"] and
                    t["y"] < vy + vh and vy < t["y"] + t["h"]):
                found.append(where + ": covers the viewer")
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--png", help="write one screenshot per case here")
    parser.add_argument("--sizes", default=",".join(SIZES))
    args = parser.parse_args()

    import isolate
    import xdisplay

    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    if not apps.installed("builder"):
        print("SKIP  no installed builder")
        return 77
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"

    failures, checks = [], 0
    for size in args.sizes.split(","):
        xdisplay.SCREEN = size + "x24"
        with xdisplay.Display() as display:
            log = []
            apps.startServices(display, log)
            try:
                if not all(apps.serviceState(display).values()):
                    print("FAIL  services did not come up: %s" % log)
                    return 1
                fixture.ensureRealUserAccount()
                restore = fixture.settleUpgradeNotices()
                try:
                    url, error = pbc_ux_test.install("tool-column",
                                                     pbc_ux_test.notRun)
                    if error:
                        print("FAIL  cannot install fixture: " + error)
                        return 1
                    auth = fixture.authFile(
                        os.path.join(fixture.stateHome(), ".ECCE",
                                     "auth.pipe"), user=fixture.USER)
                    for mode in MODES:
                        checks += 1
                        label = "%s, %s layout" % (size, mode)
                        script = ["columntab 0"]
                        if args.png:
                            os.makedirs(args.png, exist_ok=True)
                            script.append("xshot " + os.path.join(
                                args.png, "tools-%s-%s.png" % (size, mode)))
                        script.append("panestate state " + " ".join(TOOLS))
                        work, found, out = pbc_ux_test.runScene(
                            display, "tools-%s-%s" % (size, mode), script,
                            url, ("-pipe", auth, "-context", url),
                            env={"ECCE_PANEL_MODE": mode})
                        found += problems(pbc_ux_test.readState(
                            os.path.join(work, "state.txt")), label)
                        print("%s  %s" % ("FAIL" if found else "ok  ", label))
                        for p in found:
                            print("      " + p)
                        failures += found
                    fixture.remove("tool-column")
                finally:
                    restore()
            finally:
                apps.stopServices(display)
    print("%s  %d cases, %d problems" % ("FAIL" if failures else "PASS",
                                         checks, len(failures)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
