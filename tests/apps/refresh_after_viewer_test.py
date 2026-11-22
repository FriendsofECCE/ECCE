#!/usr/bin/env python3
"""Organizer details pane after a Viewer tab closes (#1237), headless.

One-process mode: a Viewer opens on one stored ECCE-QM calculation, its tab
is closed, then another calculation is selected. By hand the desk then
stayed empty until the window lost and regained focus. Each step answers
`refreshtimes` (the pane's last refresh timing and its formula field) and
`paintlog` (the windows painted on the main window since `paintlog on`).
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
PROJECT_META = os.path.join(HERE, "..", "..", "tools", "screenshots", "data",
                            "project.ecce-meta")
LIF = os.path.join(HERE, "..", "..", "data", "client", "examples", "genchem",
                   "molecular-orbitals-of-diatomics", "ecceqm-LiF", "stored", "LiF")
failures = []


def check(ok, what, log=""):
    print("  %s %s" % ("ok  " if ok else "FAIL", what), flush=True)
    if not ok:
        failures.append(what)
        print("\n".join("      | " + l[-300:] for l in log.splitlines()[-15:]))


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings), flush=True)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    os.environ.pop("DISPLAY", None)
    work = tempfile.mkdtemp(prefix="refreshviewer-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    project = os.path.join(data, "users", "local", "p")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    for name in ("LiF-1", "LiF-1-1"):
        shutil.copytree(LIF, os.path.join(project, name), symlinks=True)
    a = "file://" + os.path.join(project, "LiF-1-1")
    b = "file://" + os.path.join(project, "LiF-1")
    os.environ["ECCE_LOCAL_DATA"] = data
    os.environ["ECCE_TEST_ORGANIZER_NOLAUNCH"] = "1"
    os.environ["ECCE_TIMING"] = "1"
    os.environ["ECCE_ONE_PROCESS"] = "1"
    cmds = os.path.join(work, "refresh.commands")
    open(cmds, "w").write("")

    def send(line, wait):
        with open(cmds, "a") as f:
            f.write(line + "\n")
        time.sleep(wait)

    def drive(_display):
        time.sleep(15)
        send("select " + b, 3)
        send("refreshtimes", 2)
        send("select " + a, 3)
        send("start CalculationViewer " + a, 25)
        send("tabs", 2)
        send("tabclose 1", 4)
        send("tabs", 2)
        send("paintlog on", 1)
        send("select " + b, 4)
        send("refreshtimes", 2)
        send("paintlog", 2)

    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            result = apps.run(display, "organizer", windowTimeout=60, settle=10,
                              env={"ECCE_TEST_ORGANIZER": cmds}, inspect=drive)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    log = result.log or ""
    check(not result.crashed, "the Organizer did not crash", log)
    for l in log.splitlines():
        if any(k in l for k in ("refreshtimes:", "paintlog:", "ORGANIZER: refresh",
                                "ECCE_TEST_ORGANIZER: tabs")):
            print("  | " + l[:600])
    replies = [l for l in log.splitlines()
               if "ECCE_TEST_ORGANIZER: refreshtimes: " in l]
    check(len(replies) == 2, "two refreshtimes replies (%d)" % len(replies),
          log[-3000:])
    if len(replies) == 2:
        m = re.search(r"refresh (\S+) total=(\d+) .*formula=\[(.*)\]", replies[1])
        check(bool(m) and m.group(1) == "LiF-1",
              "after the Viewer closed, the pane refreshed for LiF-1")
        if m:
            check(m.group(3) not in ("", "none"),
                  "LiF-1 formula field filled: %s" % m.group(3))
    paints = [l for l in log.splitlines() if "ECCE_TEST_ORGANIZER: paintlog: " in l]
    check(bool(paints), "paintlog answered", log[-3000:])
    if paints:
        # The paint log names a window by its wx class: the details pane is
        # a wxScrolledWindow beside the project tree.
        check("wxScrolledWindow@" in paints[-1],
              "the details pane was painted after the click: " + paints[-1][-300:])
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
