#!/usr/bin/env python3
"""
Organizer > File > Import Calculation from Output File, without the dialog.

The Organizer's ECCE_TEST_CALCIMPORT hook imports a given file into a
project "calcimport-test" in the user's home and prints the outcome.
Outputs of codes with an importer must be imported as that code; outputs
of codes without one (MOPAC, GROMACS, Quantum ESPRESSO) must be refused
with "Unrecognized output file format".  An imported calculation must be
selected in the tree afterwards, and Reset for Rerun on it must be refused
(ECCE_TEST_RESETIMPORTED) with a message, leaving it imported.

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

DATA = os.path.join(HERE, "..", "parsers", "fixtures")

# file -> code it must be imported as, or None for "refused"
CASES = [
    ("nwchem/h2o_opt_stdout.out", "NWChem"),
    ("gaussian-16/co_freq.log", "Gaussian-16"),
    ("orca/h2o_sym.out", "ORCA"),
    ("mopac/ch4_opt.out", None),
    ("gromacs/water_md.log", None),
    ("qe/h2o_gamma.pwout", None),
]

failures = []


def resetCheck(log):
    """Run Mgmt > Reset for Rerun on the imported calculation (the hook
    ECCE_TEST_RESETIMPORTED) must be refused with the explanation, and
    leave the calculation Loaded.  Returns a problem string or None."""
    if not re.search(r"ECCE_TEST_RESETIMPORTED: refused 1: Imported results "
                     r"cannot be reset.*Duplicate for Rerun", log):
        return "Reset for Rerun on an imported calculation gave no message"
    if "ECCE_TEST_RESETIMPORTED: state after Loaded" not in log:
        return "imported calculation was reset"
    return None


def check(display, name, expected):
    os.environ["ECCE_TEST_RESETIMPORTED"] = "1"
    os.environ["ECCE_TEST_CALCIMPORT"] = os.path.abspath(
        os.path.join(DATA, name))
    authPath = fixture.authFile(
        os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"),
        user=fixture.realUser())
    result = apps.run(display, "organizer", args=("-pipe", authPath),
                      windowTimeout=60, settle=90)
    log = result.log or ""
    done = re.search(r"ECCE_TEST_CALCIMPORT: (\w+), code (\S+): (.*)", log)
    if result.crashed:
        problem = "CRASHED (%s)" % result.signalName
    elif not done:
        problem = "never finished the import"
    elif expected is None and not (done.group(1) == "refused" and
                                   "Unrecognized output file format"
                                   in done.group(3)):
        problem = "not refused: " + done.group(0)
    elif expected is not None and (done.group(1) != "imported" or
                                   done.group(2) != expected):
        problem = "expected %s: %s" % (expected, done.group(0))
    elif expected is not None and resetCheck(log):
        problem = resetCheck(log)
    elif expected is not None and not re.search(
            r"ECCE_TEST_CALCIMPORT: selected \S*/calcimport-test/[^/\s]+\s",
            log):
        # The tree must move to the new calculation, not stay on the
        # folder that was selected before.
        problem = "imported calculation not selected in the tree"
    else:
        print("ok    %s: %s" % (name, done.group(0)))
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
            results.fail("calcimport", "%d of %d imports wrong: %s"
                         % (len(failures), len(CASES), ", ".join(failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
