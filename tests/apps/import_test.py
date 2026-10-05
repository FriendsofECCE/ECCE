#!/usr/bin/env python3
"""
Builder > File > Add Structure from File, without the file dialog.

The Builder's ECCE_TEST_IMPORT hook runs the import on a given path, presses
OK in any prompt it raises (units for XYZ) and exits.  Each file must load
the expected number of atoms; a path that does not exist must be reported
("Cannot read"), not crash -- EDSIFactory::getResource() returns NULL for it
and the handler used to dereference that.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

DATA = os.path.join(HERE, "..", "fragreaders", "data")

# file -> atoms expected, or None for "must fail with Cannot read"
CASES = [
    ("glycine.pdb", 10),
    ("glycine.ent", 10),
    ("chains.pdb", 11),
    ("benzene.xyz", 12),
    ("benzene.car", 12),
    ("no-such-file.pdb", None),
    # The shape the file dialog produced when its directory was lost.
    ("*/" + os.path.abspath(os.path.join(DATA, "benzene.xyz")), None),
]

failures = []


def check(display, name, expected):
    path = name if "/" in name else os.path.abspath(os.path.join(DATA, name))
    os.environ["ECCE_TEST_IMPORT"] = path
    authPath = fixture.authFile(
        os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"),
        user=fixture.realUser())
    result = apps.run(display, "builder", args=("-pipe", authPath),
                      windowTimeout=60, settle=60)
    log = result.log or ""
    done = re.search(r"ECCE_TEST_IMPORT: done, (\d+) atoms", log)
    if result.crashed:
        problem = "CRASHED (%s)" % result.signalName
    elif not done:
        problem = "never finished the import"
    elif expected is None and "Cannot read" not in log:
        problem = "no 'Cannot read' error"
    elif expected is not None and int(done.group(1)) != expected:
        problem = "%s atoms, expected %d" % (done.group(1), expected)
    else:
        print("ok    %s" % name)
        return
    failures.append(name)
    print("FAIL  %s: %s\n%s" % (name, problem, run_tests._tail(log, 20)))


def main():
    # Reuse run_tests.main() for isolation, Xvfb and services: it calls
    # checkApp() once for the selected app, which runs the cases instead.
    def checkApp(display, name, results, verbose=False):
        for case, expected in CASES:
            check(display, case, expected)
        if failures:
            results.fail("import", "%d of %d imports failed: %s"
                         % (len(failures), len(CASES), ", ".join(failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
