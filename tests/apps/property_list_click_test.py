#!/usr/bin/env python3
"""A click in the Builder's Properties list selects the row clicked (#965).

The list scrolls once a calculation has more property panels than fit in
the share of the column it gets, and a click then selected the row below
the one under the pointer.  The clicks here go through the X server
(xdotool) at the row rectangles the Builder itself reports ("ptree" writes
one "row x y w h <text>" line per row in screen pixels), so what is tested
is the real hit test on a scrolled list, not a hook.

Exit 77 (skip) without Xvfb, xdotool, the built Builder or the fixture.
"""
import os
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402
from orca_mo_test import makeCalc   # noqa: E402

SKIP = 77
STEPS = 5            # rows clicked, one per "ptree" after the first
failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + str(detail)) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def readTree(path):
    """{'rows': [(name, x, y, w, h)], 'sel': str, 'win': str}"""
    out = {"rows": [], "sel": "", "win": ""}
    if not os.path.exists(path):
        return None
    for line in open(path):
        f = line.split()
        if not f:
            continue
        if f[0] == "row" and len(f) >= 6:
            out["rows"].append((" ".join(f[5:]), int(f[1]), int(f[2]),
                                int(f[3]), int(f[4])))
        elif f[0] == "treewin":
            out["win"] = line.strip()
        elif f[0] == "tree":
            out["sel"] = line.split(None, 1)[1].strip() if len(f) > 1 else ""
    return out


def waitFor(path, deadline):
    while time.time() < deadline:
        if os.path.exists(path) and os.path.getsize(path) > 0:
            time.sleep(0.4)
            return readTree(path)
        time.sleep(0.3)
    return None


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return SKIP
    for tool in ("Xvfb", "xdotool"):
        if not shutil.which(tool):
            print("SKIP: no %s" % tool)
            return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="proplist-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    #  A short screen: the list gets a share of the column, so with every
    #  property panel listed it scrolls, which is the case that failed.
    xdisplay.SCREEN = "1280x700x24"
    try:
        url = makeCalc(data, "proplist")
        out = os.path.join(work, "run")
        #  "shot sN" is the handshake: the Builder writes sN.ready and
        #  waits for sN.go, which is when the click goes in.
        scene = "hold 4\nptree p0\n"
        for i in range(1, STEPS + 1):
            scene += "shot s%d\nptree p%d\n" % (i, i)
        scene += "hold 2\n"
        clicked = []
        panels = set()

        def clicker(disp):
            env = dict(os.environ, DISPLAY=disp.name)
            state = waitFor(os.path.join(out, "p0.txt"), time.time() + 180)
            if state is None:
                return
            for line in open(os.path.join(out, "p0.txt")):
                if line.startswith("entry "):
                    panels.add(line.split("/", 1)[1].strip())
            #  Half the steps after a wheel scroll in the list, which is
            #  how a user reaches a row the list does not show.
            for n in range(1, STEPS + 1):
                if n == 3:
                    #  A wheel scroll instead of a click: the next step
                    #  clicks on the rows as the list draws them after it.
                    f = state["win"].split()
                    ready = os.path.join(out, "s3.ready")
                    end = time.time() + 120
                    while not os.path.exists(ready) and time.time() < end:
                        time.sleep(0.3)
                    subprocess.run(["xdotool", "mousemove",
                                    str(int(f[1]) + 30), str(int(f[2]) + 40),
                                    "click", "5", "click", "5"], env=env,
                                   stderr=subprocess.DEVNULL)
                    time.sleep(1.5)
                    open(os.path.join(out, "s3.go"), "w").close()
                    state = waitFor(os.path.join(out, "p3.txt"),
                                    time.time() + 60)
                    if state is None:
                        break
                    print("      after the wheel: %s" % state["win"])
                    for r in state["rows"]:
                        print("      wheel row %-26s %4d %4d %3dx%-3d" % r)
                    continue
                #  Only a row drawn inside the tree can be clicked: the
                #  rectangles of a scrolled list run past its bottom edge.
                f = state["win"].split()
                top, height = int(f[2]), int(f[4])
                #  Headings carry no panel, so a click on one opens or
                #  closes the group instead of selecting: not this check.
                rows = [r for r in state["rows"]
                        if r[2] >= top and r[2] + r[4] <= top + height
                        and r[0] != state["sel"] and r[0] in panels]
                if not rows:
                    clicked.append(("no row inside the tree", state["win"]))
                    break
                name, x, y, w, h = rows[(n - 1) % len(rows)]
                ready = os.path.join(out, "s%d.ready" % n)
                end = time.time() + 120
                while not os.path.exists(ready) and time.time() < end:
                    time.sleep(0.3)
                if os.environ.get("ECCE_LIST_SHOT"):
                    f = state["win"].split()
                    subprocess.run(["import", "-display", disp.name,
                                    "-window", "root", "-crop",
                                    "%sx%s+%s+%s" % (f[3], f[4], f[1], f[2]),
                                    "+repage",
                                    os.environ["ECCE_LIST_SHOT"] + "-%d.png" % n],
                                   check=False)
                while not os.path.exists(ready) and time.time() < end:
                    time.sleep(0.3)
                #  Press and release apart, as a hand does: the panel the
                #  press opens relays the column out in between.
                subprocess.run(["xdotool", "mousemove", str(x + 10),
                                str(y + h // 2), "mousedown", "1"], env=env,
                               stderr=subprocess.DEVNULL)
                time.sleep(float(os.environ.get("ECCE_CLICK_HOLD", "0.0")))
                subprocess.run(["xdotool", "mouseup", "1"], env=env,
                               stderr=subprocess.DEVNULL)
                time.sleep(1.5)
                open(os.path.join(out, "s%d.go" % n), "w").close()
                state = waitFor(os.path.join(out, "p%d.txt" % n),
                                time.time() + 60)
                if state is None:
                    clicked.append((name, "?"))
                    break
                clicked.append((name, state["sel"]))

        os.makedirs(out, exist_ok=True)
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
                        windowTimeout=120, settle=20, inspect=clicker,
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
        first = readTree(os.path.join(out, "p0.txt"))
        if not check(first is not None, "the Properties list reported its rows",
                     (result.log or "")[-1500:]):
            return 1
        print("      p0.txt:\n" + "".join("      | " + l for l in open(os.path.join(out, "p0.txt"))))
        for name, x, y, w, h in first["rows"]:
            print("      row %-28s %4d %4d %3dx%-3d" % (name, x, y, w, h))
        check(len(clicked) >= 1, "clicks were made", clicked)
        for name, got in clicked:
            check(got == name, "a click on %r selects %r" % (name, name),
                  "selected %r" % got)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
