#!/usr/bin/env python3
"""
A floating panel docks without Ctrl only on the right; on the top, left and
bottom it docks only with Ctrl (#1215).  Headless: the Builder scene command
dropside runs the drop logic of a floating MO Diagram for all 8 cases.

Same installed tree and isolation as mo_explore_workflow_test.py
(ECCE_TEST_HOME, ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or
Xvfb.
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

PANEL = "MO Diagram"
SIDES = ("top", "left", "bottom", "right")


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="dockctrl-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1700x1000x24"
    failures = []

    scene = "hold 2\n"
    cases = []
    for side in SIDES:
        for mode in ("noctrl", "ctrl"):
            tag = "%s_%s" % (side, mode)
            cases.append((side, mode, tag))
            scene += ("dockpane %s float\nhold 1\ndropside %s %s %s %s\n"
                      % (PANEL, tag, side, mode, PANEL))
    scene += "hold 2\n"

    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            url = base.makeCalc(data, "water")
            out = os.path.join(work, "run")
            res = base.runBuilder(display, url, out, scene, shot=None,
                                  panel=PANEL)
            if res.crashed or os.path.exists(os.path.join(out, "FAILED")):
                failures.append("the Builder crashed or the scene failed")
            for side, mode, tag in cases:
                path = os.path.join(out, tag + ".txt")
                got = open(path).read().strip() if os.path.exists(path) else ""
                m = re.match(r"docked=(\d) floating=(\d) dir=(\d+)$", got)
                want = 1 if (side == "right" or mode == "ctrl") else 0
                ok = bool(m) and int(m.group(1)) == want and \
                    int(m.group(2)) == 1 - want
                print("%s %-6s %-7s -> %s (expected docked=%d)"
                      % ("ok   " if ok else "FAIL ", side, mode, got, want))
                if not ok:
                    failures.append("%s %s" % (side, mode))
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin",
                                         "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
