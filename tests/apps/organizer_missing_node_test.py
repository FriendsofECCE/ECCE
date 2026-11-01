#!/usr/bin/env python3
"""A calculation renamed on disk behind the Organizer's back: selecting its
stale tree node must say it no longer exists, not ask for a password (#683).

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
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    calc = os.path.join(project, "water")
    shutil.copytree(CALC, calc, symlinks=True)
    script = os.path.join(work, "commands")
    open(script, "w").close()

    def behind_its_back(_display):
        os.rename(calc, os.path.join(project, "water-renamed"))
        with open(script, "a") as handle:
            # The opened calculation is already selected; move off it first.
            handle.write("panelmessage file://%s\n" % project)
            handle.write("panelmessage file://%s\n" % calc)
        time.sleep(4)

    result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                      inspect=behind_its_back,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_TEST_ORGANIZER_NOLAUNCH": "1",
                           "ECCE_ORGANIZER_OPEN": "file://" + calc})
    log = result.log or ""
    what = "Organizer, renamed calculation"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    got = re.findall(r"ECCE_TEST_ORGANIZER: panelmessage\b.*?: (ok.*)$", log,
                     re.M)
    if len(got) < 2:
        return fail(what, "panelmessage not answered", log)
    text = got[-1]
    if "password" in text or "no longer exists" not in text \
            or "refresh" not in text:
        return fail(what, "wrong panel text: %s" % text, log)
    print("ok    %s" % what)


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
