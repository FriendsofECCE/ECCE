#!/usr/bin/env python3
"""
Builder > File > Save As..., without clicks.

The Builder's ECCE_TEST_SAVEAS hook opens the dialog, picks a type, saves
to a path on the Local Filesystem and exits.  A calculation type there
must be refused in the dialog (a calculation needs a project on the data
server), not created and then reported as "error parsing"; a chemical
file type must be written.  Opening the dialog must not raise a
Gtk-CRITICAL.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

# (type label prefix, file name, file expected afterwards or None if refused)
CASES = [
    ("ORCA", "calc", None),
    ("XYZ", "plain", "plain.xyz"),
]

failures = []


def check(display, target, label, name, expected):
    path = os.path.join(target, name)
    os.environ["ECCE_TEST_SAVEAS"] = "%s|%s" % (label, path)
    authPath = fixture.authFile(
        os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"),
        user=fixture.realUser())
    result = apps.run(display, "builder", args=("-pipe", authPath),
                      windowTimeout=60, settle=60)
    log = result.log or ""
    opened = log.find("ECCE_TEST_SAVEAS: open")
    after = log[opened:] if opened >= 0 else ""
    if result.crashed:
        problem = "CRASHED (%s)" % result.signalName
    elif "ECCE_TEST_SAVEAS: done" not in log:
        problem = "never finished Save As"
    elif "CRITICAL" in after:
        problem = "Gtk-CRITICAL with the dialog open"
    elif expected is None and "ECCE_TEST_SAVEAS: refused" not in after:
        problem = "calculation type not refused on the local file system"
    elif expected is None and os.path.exists(path):
        problem = "refused, but %s was created" % path
    elif "error parsing" in after:
        problem = "'error parsing' after Save As"
    elif expected is not None and not os.path.isfile(
            os.path.join(target, expected)):
        problem = "%s not written" % expected
    elif expected is not None and "dialog still open" in after:
        problem = "dialog did not close"
    else:
        print("ok    %s -> %s" % (label, name))
        return
    failures.append(label)
    print("FAIL  %s: %s\n%s" % (label, problem, run_tests._tail(log, 20)))


def main():
    def checkApp(display, name, results, verbose=False):
        target = tempfile.mkdtemp(prefix="ecce-saveas-")
        try:
            for label, name, expected in CASES:
                check(display, target, label, name, expected)
        finally:
            shutil.rmtree(target, ignore_errors=True)
        if failures:
            results.fail("saveas", "%d of %d Save As cases wrong: %s"
                         % (len(failures), len(CASES), ", ".join(failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
