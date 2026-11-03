#!/usr/bin/env python3
"""A calculation renamed or moved on disk behind the Organizer's back:
selecting its stale tree node must say it is no longer on disk (and where a
folder of that name now is), not ask for a password (#683); one that only
cannot be read must say why, not that it was moved.

    tests/apps/organizer_missing_node_test.py

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME).
Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import time
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
    project = os.path.join(data, "users", "local", "alpha")
    beta = os.path.join(data, "users", "local", "beta")
    for folder in (project, beta):
        os.makedirs(folder)
        shutil.copy(PROJECT_META, os.path.join(folder, ".ecce-meta"))
    calc = os.path.join(project, "water")
    moved = os.path.join(project, "ice")
    locked = os.path.join(beta, "steam")
    for folder in (calc, moved, locked):
        shutil.copytree(CALC, folder, symlinks=True)
    sub = os.path.join(project, "sub")
    os.makedirs(sub)
    script = os.path.join(work, "commands")
    open(script, "w").close()

    def behind_its_back(_display):
        with open(script, "a") as handle:
            # Load every node before the disk changes under them.
            for url in (project, moved, beta, locked, project):
                handle.write("panelmessage file://%s\n" % url)
        time.sleep(4)
        os.rename(calc, os.path.join(project, "water-renamed"))
        os.rename(moved, os.path.join(sub, "ice"))
        os.chmod(beta, 0o600)
        with open(script, "a") as handle:
            for url in (calc, moved, locked):
                handle.write("panelmessage file://%s\n" % url)
        time.sleep(4)
        os.chmod(beta, 0o755)

    result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                      inspect=behind_its_back,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_TEST_ORGANIZER_NOLAUNCH": "1",
                           "ECCE_ORGANIZER_OPEN": "file://" + calc})
    os.chmod(beta, 0o755)
    log = result.log or ""
    what = "Organizer, calculation gone or unreadable"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    got = re.findall(r"ECCE_TEST_ORGANIZER: panelmessage\b.*?: (ok.*)$", log,
                     re.M)
    if len(got) < 8:
        return fail(what, "panelmessage not answered", log)
    renamed, down, unreadable = got[-3:]
    if "password" in renamed or "no longer on disk" not in renamed \
            or calc not in renamed or "refresh" not in renamed:
        fail("renamed calculation", "wrong panel text: %s" % renamed, log)
    else:
        print("ok    renamed calculation")
    if "no longer on disk" not in down or os.path.join(sub, "ice") not in down:
        fail("calculation moved into a sibling folder",
             "wrong panel text: %s" % down, log)
    else:
        print("ok    calculation moved into a sibling folder")
    # A folder that cannot be read is not one that was moved or removed.
    if "no longer" in unreadable or "Permission denied" not in unreadable:
        fail("unreadable calculation", "wrong panel text: %s" % unreadable,
             log)
    else:
        print("ok    unreadable calculation")


def check(display, results, verbose=False):
    """run_tests.py entry."""
    folder = tempfile.mkdtemp(prefix="orgmissing-",
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
        results.fail("organizer missing node", failure)


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
    work = tempfile.mkdtemp(prefix="orgmissing-",
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
