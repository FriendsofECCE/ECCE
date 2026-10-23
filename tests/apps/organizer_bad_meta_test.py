#!/usr/bin/env python3
"""A calculation whose .ecce-meta lacks ecce:propertyCollection and
ecce:runLogDocument must not take the Organizer down (#489).

    tests/apps/organizer_bad_meta_test.py

Calculations written by scripts lack those two entries.  The Organizer
reopens the last selection at start, so a crash on selection meant it
crashed at every start.  Here the calculation is opened at start (as the
restore does) and then selected again; it must be listed, answer the
summary command, and the warning naming it must be logged.

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
DROPPED = ("ecce:propertyCollection", "ecce:runLogDocument")

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-15:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def run(display, data, work):
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "alpha")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    calc = os.path.join(project, "water")
    shutil.copytree(CALC, calc, symlinks=True)
    meta = os.path.join(calc, ".ecce-meta")
    with open(meta) as handle:
        lines = handle.readlines()
    kept = [l for l in lines if not any(d in l for d in DROPPED)]
    if len(kept) != len(lines) - 2:
        return fail("fixture", "expected to drop 2 meta lines, dropped %d"
                    % (len(lines) - len(kept)))
    with open(meta, "w") as handle:
        handle.writelines(kept)
    script = os.path.join(work, "commands")
    with open(script, "w") as handle:
        handle.write("summary file://%s\ntree\n" % calc)
    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_TEST_ORGANIZER_NOLAUNCH": "1",
                           "ECCE_ORGANIZER_OPEN": "file://" + calc})
    log = result.log or ""
    what = "Organizer, calculation without property/run-log metadata"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    if "Unhandled unknown exception" in log:
        return fail(what, "unhandled exception", log)
    answered = re.findall(r"ECCE_TEST_ORGANIZER: (summary|tree)\b.*?: (ok.*)$",
                          log, re.M)
    if len(answered) < 2:
        return fail(what, "%d of 2 commands answered" % len(answered), log)
    if "[2:water" not in answered[-1][1]:
        return fail(what, "the calculation is not listed/selected", log)
    for entry in DROPPED:
        if not re.search(r"WARNING: calculation \S*water has no %s" % entry,
                         log):
            return fail(what, "no warning naming the calculation for %s"
                        % entry, log)
    print("ok    %s" % what)


def check(display, results, verbose=False):
    """run_tests.py entry."""
    folder = tempfile.mkdtemp(prefix="orgmeta-",
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
        results.fail("organizer bad meta", failure)


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
    work = tempfile.mkdtemp(prefix="orgmeta-",
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
