#!/usr/bin/env python3
"""Switching the Properties list between MOs and Surfaces paints only the
final panel (#1263), headless.

MOs and Surfaces are one panel in two modes. By hand, for a frame or two
after the switch the MO table was drawn over the Surfaces controls (and the
reverse). The Builder's "panelpaints" scene command clicks the list row and
records every paint of a window inside that panel for 1.5 s: a paint of a
window that the new mode hides, or at a size other than the one it settles
at, is the old layout reaching the screen.

Exit 77 (skip) without Xvfb, the built Builder or the fixture.
"""
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402
from orca_mo_test import makeCalc   # noqa: E402

SKIP = 77
STEPS = ["Surfaces", "MOs", "Surfaces", "MOs"]
failures = []


def check(ok, what, detail=""):
    print("  %s %s" % ("ok  " if ok else "FAIL", what), flush=True)
    if not ok:
        failures.append(what)
        if detail:
            print("\n".join("      | " + l for l in str(detail).splitlines()[-40:]))
    return ok


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="propswitch-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1600x1200x24"
    try:
        url = makeCalc(data, "propswitch")
        out = os.path.join(work, "run")
        os.makedirs(out, exist_ok=True)
        scene = "hold 4\n"
        for i, name in enumerate(STEPS):
            scene += "panelpaints p%d 1500 %s\n" % (i, name)
        scene += "hold 1\n"
        script = os.path.join(out, "scene")
        open(script, "w").write(scene)
        with xdisplay.Display() as display:
            log = []
            apps.startServices(display, log)
            try:
                fixture.ensureRealUserAccount()
                restore = fixture.settleUpgradeNotices()
                try:
                    result = apps.run(
                        display, "builder", args=("-context", url),
                        windowTimeout=120, settle=120,
                        ready=lambda: os.path.exists(os.path.join(
                            out, "p%d.txt" % (len(STEPS) - 1))),
                        env={"ECCE_PANEL_MODE": "detail",
                             "ECCE_OPEN_PANEL": "MOs",
                             "ECCE_VIEWER_SCENE": script,
                             "ECCE_VIEWER_SCENE_OUT": out,
                             "ECCE_TEST_DIALOG_CLOSE": "1",
                             "ECCE_TRANSPARENCY_FALLBACK_MS": "0"})
                finally:
                    restore()
            finally:
                apps.stopServices(display)
        check(not result.crashed, "the Builder did not crash",
              (result.log or "")[-1500:])
        for i, name in enumerate(STEPS):
            path = os.path.join(out, "p%d.txt" % i)
            if not check(os.path.exists(path),
                         "switch %d to %s was recorded" % (i, name),
                         (result.log or "")[-1500:]):
                continue
            lines = open(path).read().splitlines()
            print("    switch %d to %s:" % (i, name))
            for l in lines:
                print("      " + l)
            stale = []
            for l in lines:
                f = l.split()
                if f[:1] != ["paint"] or len(f) < 7:
                    continue
                if f[6] != "1" or f[2] != f[4]:
                    stale.append(l)
            check(not stale, "switch %d to %s: no window of the panel is "
                  "painted hidden or at a size it does not keep" % (i, name),
                  "\n".join(stale))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
