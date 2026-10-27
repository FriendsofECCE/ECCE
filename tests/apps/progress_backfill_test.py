#!/usr/bin/env python3
"""A finished calculation with an output but no progress record gets one (#284).

    tests/apps/progress_backfill_test.py       own Xvfb, services, local data

The shipped examples are imported (their job store stores ecce:progress),
the record is cut out of the calculations' metadata, and an Organizer is
started on them: selecting a calculation folds its stored output through
the job monitor's [PROGRESS] entries in the background, stores the result as
the monitor would, and the Progress box shows it.  Calculations of codes
with no [PROGRESS] entries (the NWChem example's stdout is not stored) stay
without one and cause no error.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
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

import apps            # noqa: E402
import examples_test   # noqa: E402
import isolate         # noqa: E402
import xdisplay        # noqa: E402

SKIP = 77
CALCS = ["water", "benzene", "ammonia"]
failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-15:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def metaOf(calc):
    return os.path.join(calc, ".ecce-meta")


def hasProgress(calc):
    try:
        return "ecce:progress" in open(metaOf(calc)).read()
    except OSError:
        return False


def cutProgress(calc):
    lines = open(metaOf(calc)).read().split("\n")
    kept = [l for l in lines if "ecce:progress" not in l]
    with open(metaOf(calc), "w") as handle:
        handle.write("\n".join(kept))
    return len(lines) - len(kept)


def run(display, data, work):
    root = os.path.join(data, "users", "local")
    commands = os.path.join(work, "import.commands")
    with open(commands, "w") as handle:
        handle.write("examples\n")

    def waitImport(_display):
        examples_test.waitImported(root, ["Examples"], examples_test.EXPECT)

    apps.run(display, "organizer", windowTimeout=60, settle=20,
             env={"ECCE_TEST_ORGANIZER": commands}, inspect=waitImport)

    project = os.path.join(root, examples_test.FIRST)
    calcs = {c: os.path.join(project, c) for c in CALCS}
    for name, path in calcs.items():
        if not hasProgress(path):
            return fail("setup", "the import of %s stored no progress" % name)
        if cutProgress(path) != 1 or hasProgress(path):
            return fail("setup", "could not cut the record of " + name)

    second = os.path.join(work, "second.commands")
    with open(second, "w") as handle:
        for name, path in calcs.items():
            handle.write("summary file://%s\n" % path)
    box = {}

    def waitBackfill(_display):
        end = time.time() + 90
        while time.time() < end and not all(hasProgress(p)
                                            for p in calcs.values()):
            time.sleep(1)
        box["done"] = [n for n, p in calcs.items() if hasProgress(p)]
        with open(second, "a") as handle:
            handle.write("summary file://%s\nprogressbox\n" % calcs["benzene"])
        time.sleep(6)

    result = apps.run(display, "organizer", windowTimeout=60, settle=10,
                      env={"ECCE_TEST_ORGANIZER": second},
                      inspect=waitBackfill)
    log = result.log or ""
    what = "Progress records for finished calculations"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    problems = []
    missing = [c for c in CALCS if c not in box.get("done", [])]
    if missing:
        problems.append("no record made for %s" % missing)
    shown = re.findall(r"ECCE_TEST_ORGANIZER: progressbox: (shown=\S+.*)$",
                       log, re.M)
    if not shown or "shown=1" not in shown[-1]:
        problems.append("the Progress box is not shown: %s" % shown[-1:])
    elif "opt" not in shown[-1].lower():
        problems.append("the box has no optimisation lines: %s" % shown[-1])
    text = open(metaOf(calcs["benzene"])).read() if hasProgress(
        calcs["benzene"]) else ""
    if text and "opt step=" not in text.replace("\\n", "\n"):
        problems.append("the stored record has no optimisation step")
    if problems:
        fail(what, "; ".join(problems), log)
    else:
        print("ok    %s: %s" % (what, ", ".join(CALCS)))


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb") or not shutil.which("perl"):
        print("SKIP: no Xvfb or perl")
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
    work = tempfile.mkdtemp(prefix="backfill-",
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
