#!/usr/bin/env python3
"""Duplicate for Rerun and Duplicate Setup with Last Geometry add the copy
under its project and select it; nothing else in the tree moves.

    tests/apps/organizer_duplicate_test.py

It used to reload the project (every expanded sibling project collapsed and
the selection fell to the root) and then select the copy.  The Organizer's
ECCE_TEST_ORGANIZER "duplicate" command runs the menu item and reports the
tree's selection and collapse events and the tree afterwards.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data",
                            "project.ecce-meta")
CALC = os.path.join(HERE, "fixtures", "calc-water-opt")

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-15:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def run(display, data, work):
    root = os.path.join(data, "users", "local")
    for name in ("alpha", "beta"):
        project = os.path.join(root, name)
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        shutil.copytree(CALC, os.path.join(project, "water"), symlinks=True)
    url = "file://" + root
    # beta is opened (expanded) first; duplicating in alpha must not close it.
    commands = ["summary %s/beta/water" % url,
                "duplicate %s/alpha/water" % url,
                "duplicate %s/alpha/water last" % url]
    script = os.path.join(work, "commands")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_ORGANIZER_OPEN": "%s/alpha/water" % url})
    log = result.log or ""
    what = "Organizer duplicate"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    got = re.findall(r"ECCE_TEST_ORGANIZER: (duplicate .+?): (ok.*)$", log,
                     re.M)
    if len(got) < 2:
        return fail(what, "%d of 2 commands answered" % len(got), log)
    for n, (command, outcome) in enumerate(got):
        events, tree = outcome[3:].split(" |", 1)
        names = re.findall(r"\[?\d+:\S+", tree)
        problems = []
        if "collapsed" in events or "sel:root" in events:
            problems.append("tree moved: %s" % events.strip())
        selected = [x for x in names if x.startswith("[")]
        if len(selected) != 1 or not selected[0].startswith("[2:water"):
            problems.append("copy not the selection: %s" % tree)
        if sum(1 for x in names if x.lstrip("[").startswith("2:water")) \
                != 3 + n:
            problems.append("projects do not hold the copies: %s" % tree)
        if "[1:alpha" in names or "1:alpha" not in names:
            problems.append("alpha not expanded below the root: %s" % tree)
        if "1:beta" not in names:
            problems.append("beta lost: %s" % tree)
        if problems:
            fail(what + " " + command, "; ".join(problems), log)
        else:
            print("ok    %s: %s" % (command, events.strip() or "no events"))


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
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="orgdup-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            run(display, data, work)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
