#!/usr/bin/env python3
"""Stored DirDyVTST, Polyrate and Amica calculations (#873): those codes are
gone, but the Organizer must still list their calculations and select them
without crashing.

    tests/apps/organizer_retired_codes_test.py

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
CALC = os.path.join(HERE, "fixtures", "calc-retired-rate-code")
CONTROL = os.path.join(HERE, "fixtures", "calc-water-opt")
CODES = ("DirDyVTST", "Polyrate", "Amica")

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-15:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def run(display, data, work):
    project = os.path.join(data, "users", "local", "alpha")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    names = []
    for code in CODES:
        folder = os.path.join(project, "old-" + code.lower())
        shutil.copytree(CALC, folder, symlinks=True)
        meta = os.path.join(folder, ".ecce-meta")
        text = open(meta).read().replace("application\t\tDirDyVTST",
                                         "application\t\t" + code)
        open(meta, "w").write(text)
        names.append(folder)
    control = os.path.join(project, "control")
    shutil.copytree(CONTROL, control, symlinks=True)
    script = os.path.join(work, "commands")
    open(script, "w").close()

    def select_each(_display):
        with open(script, "a") as handle:
            handle.write("foldertable file://%s\n" % project)
            for folder in names + [control]:
                handle.write("panelmessage file://%s\n" % folder)
        time.sleep(8)

    result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                      inspect=select_each,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_TEST_ORGANIZER_NOLAUNCH": "1",
                           "ECCE_ORGANIZER_OPEN": "file://" + control})
    log = result.log or ""
    what = "Organizer, calculations of removed codes"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    table = re.findall(r"ECCE_TEST_ORGANIZER: foldertable\b.*?: (.*)$", log,
                       re.M)
    want = len(CODES) + 1
    if not table or "rows=%d" % want not in table[-1]:
        fail("folder lists the calculations",
             "expected rows=%d, got %s" % (want, table[-1:] or "no answer"),
             log)
    else:
        print("ok    folder lists %d calculations" % want)
    got = re.findall(r"ECCE_TEST_ORGANIZER: panelmessage\b.*?: (.*)$", log,
                     re.M)
    for code, answer in zip(CODES + ("control",), got[-want:]):
        # A retired code's calculation need not open fully (the minimal
        # fixture has no Parameters/Inputs, and main answered the same before
        # #873): it must answer, and the Organizer must survive.
        if answer.startswith("ok") or (code != "control" and answer.startswith("exception")):
            print("ok    select %s calculation (%s)" % (code, answer[:40]))
        else:
            fail("select " + code, "answer: %s" % answer, log)
    if len(got) < want:
        fail(what, "panelmessage answered %d of %d" % (len(got), want), log)


def check(display, results, verbose=False):
    """run_tests.py entry."""
    folder = tempfile.mkdtemp(prefix="orgretired-",
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
        results.fail("organizer retired codes", failure)


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
    work = tempfile.mkdtemp(prefix="orgretired-",
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
