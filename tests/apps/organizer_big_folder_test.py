#!/usr/bin/env python3
"""Selecting an Organizer folder with many calculations shows them all, in
a reasonable time (#675).

    tests/apps/organizer_big_folder_test.py [N | --source DIR]

With --source, DIR is copied as the folder instead of the fixture copies
and the row count is that of its sub-folders.

Copies the stored water calculation N times (default 200) into a folder,
selects the folder through the Organizer test hook "foldertable" and
checks that N rows show.  The elapsed time is printed and must stay
under LIMIT seconds.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME).
Exit 77 (skip) without an install or Xvfb.
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
N = 200
SOURCE = None
LIMIT = 20.0

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-15:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def run(display, data, work):
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "big")
    global N
    if SOURCE:
        shutil.copytree(SOURCE, project, symlinks=True)
        N = len([d for d in os.listdir(project)
                 if os.path.isdir(os.path.join(project, d))])
    else:
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        for i in range(N):
            shutil.copytree(CALC, os.path.join(project, "calc%03d" % i),
                            symlinks=True)
    script = os.path.join(work, "commands")
    with open(script, "w") as handle:
        handle.write("foldertable file://%s\n" % project)
    result = apps.run(display, "organizer", windowTimeout=60, settle=90,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_TEST_ORGANIZER_NOLAUNCH": "1"})
    log = result.log or ""
    what = "Organizer, folder of %d calculations" % N
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    m = re.search(r"foldertable .*?: ok rows=(\d+) seconds=([\d.]+)", log)
    if not m:
        return fail(what, "foldertable not answered", log)
    rows, seconds = int(m.group(1)), float(m.group(2))
    print("      %d rows in %.2f s" % (rows, seconds))
    if rows != N:
        return fail(what, "%d rows shown, expected %d" % (rows, N), log)
    if seconds > LIMIT:
        return fail(what, "took %.1f s (limit %.0f s)" % (seconds, LIMIT))
    print("ok    %s" % what)


def check(display, results, verbose=False):
    """run_tests.py entry."""
    folder = tempfile.mkdtemp(prefix="orgbig-",
                              dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(folder, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    del failures[:]
    results.checks += 1
    try:
        run(display, data, folder)
    finally:
        os.environ.pop("ECCE_LOCAL_DATA", None)
        shutil.rmtree(folder, ignore_errors=True)
    for failure in failures:
        results.fail("organizer big folder", failure)


def main():
    global N, SOURCE
    if len(sys.argv) > 2 and sys.argv[1] == "--source":
        SOURCE = os.path.abspath(sys.argv[2])
    elif len(sys.argv) > 1:
        N = int(sys.argv[1])
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
    work = tempfile.mkdtemp(prefix="orgbig-",
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
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
