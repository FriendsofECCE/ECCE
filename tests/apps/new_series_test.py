#!/usr/bin/env python3
"""Organizer "New Series from This..." (#621), through its test hook
`newseries TEMPLATE_URL SOURCE...`: one ORCA calculation and two xyz files
give two new Ready calculations beside the template, with generated decks,
and the Organizer's tree holds them.

    tests/apps/new_series_test.py       own Xvfb, services, local data

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS). Exit 77 (skip) without an install or Xvfb.
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
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
FIX = os.path.join(HERE, "fixtures", "calc-orca-water-ccpvdz")
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
    work = tempfile.mkdtemp(prefix="newseries-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    project = os.path.join(data, "users", "local", "p")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    tpl = os.path.join(project, "tpl")
    shutil.copytree(FIX, tpl, symlinks=True)
    os.makedirs(os.path.join(tpl, "Inputs"), exist_ok=True)
    xyz = []
    for name, body in (("a", "3\n\nO 0 0 0.1173\nH 0 0.7572 -0.4692\nH 0 -0.7572 -0.4692\n"),
                       ("b", "2\ncharge=-1\nO 0 0 0\nH 0 0 0.97\n")):
        path = os.path.join(work, name + ".xyz")
        open(path, "w").write(body)
        xyz.append(path)
    os.environ["ECCE_LOCAL_DATA"] = data
    cmds = os.path.join(work, "series.commands")
    open(cmds, "w").write("newseries file://%s %s\n" % (tpl, " ".join(xyz)))

    def wait(_display):
        end = time.time() + 120
        while time.time() < end:
            if all(os.path.exists(os.path.join(project, n, "Inputs", "orca.orcain"))
                   for n in ("a", "b")):
                break
            time.sleep(1)
        time.sleep(5)

    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            result = apps.run(display, "organizer", windowTimeout=60, settle=10,
                              env={"ECCE_TEST_ORGANIZER": cmds}, inspect=wait)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    log = result.log or ""
    check(not result.crashed, "the Organizer did not crash", log)
    for n, line in (("a", "* xyz 0 1"), ("b", "* xyz -1 1")):
        deck = os.path.join(project, n, "Inputs", "orca.orcain")
        text = open(deck).read() if os.path.exists(deck) else ""
        check(line in text, "%s: a deck with '%s'" % (n, line), log)
    check("a: made" in log and "b: made" in log, "the report says both were made", log)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
