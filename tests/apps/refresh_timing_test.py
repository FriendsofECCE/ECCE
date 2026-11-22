#!/usr/bin/env python3
"""Organizer details pane (#1237): selecting calculations fills the pane's
fields first and does the progress backfill and thumbnail afterwards.  The
hook `refreshtimes` answers the last refresh's timing line and the pane's
formula field; the test selects three stored calculations in turn and checks
both, and that no refresh takes more than 5 s.

    tests/apps/refresh_timing_test.py     own Xvfb, services, local data

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS). Exit 77 (skip) without an install or Xvfb.
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
FIX = os.path.join(HERE, "fixtures")
PROJECT_META = os.path.join(HERE, "..", "..", "tools", "screenshots", "data",
                            "project.ecce-meta")
failures = []


def check(ok, what, log=""):
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
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
    work = tempfile.mkdtemp(prefix="refreshtiming-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    project = os.path.join(data, "users", "local", "p")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    names = (("nw", "calc-water-opt"), ("g16", "calc-polar-h2o-g16"),
             ("orca", "calc-orca-water-ccpvdz"))
    for name, fix in names:
        shutil.copytree(os.path.join(FIX, fix), os.path.join(project, name), symlinks=True)
    os.environ["ECCE_LOCAL_DATA"] = data
    os.environ["ECCE_TEST_ORGANIZER_NOLAUNCH"] = "1"
    os.environ["ECCE_TIMING"] = "1"
    cmds = os.path.join(work, "refresh.commands")
    open(cmds, "w").write("")

    def drive(_display):
        time.sleep(15)
        for _ in range(2):
            for name, _fix in names:
                with open(cmds, "a") as f:
                    f.write("select file://%s\n" % os.path.join(project, name))
                time.sleep(2)
                with open(cmds, "a") as f:
                    f.write("refreshtimes\n")
                time.sleep(2)

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
    replies = [l for l in log.splitlines() if "ECCE_TEST_ORGANIZER: refreshtimes: " in l]
    check(len(replies) == 6, "six refreshtimes replies (%d)" % len(replies), log)
    for l in replies:
        m = re.search(r"refresh (\S+) total=(\d+) props=\d+ launch=\d+ backfill=\d+ thumb=\d+ formula=\[(.*)\]", l)
        check(bool(m), "timing line and formula present: " + l[-120:], log)
        if m:
            check(int(m.group(2)) < 5000, "%s refreshed in %s ms" % (m.group(1), m.group(2)), log)
            check(m.group(3) not in ("", "none"), "%s formula field filled: %s" % (m.group(1), m.group(3)), log)
    check("ORGANIZER: refresh " in log, "ECCE_TIMING printed the stderr line", log)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
