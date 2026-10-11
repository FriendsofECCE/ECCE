#!/usr/bin/env python3
"""
The Calculation Editor on a calculation that has no basisSetDocument property.

The property is what names the calculation's basis set document; a hand-edited
or damaged calculation can lack it.  Opening such a calculation used to end in
"Unhandled unknown exception" and no window.  It must open and say, as for a
new calculation, that a basis set still has to be chosen.

Local data mode, a Gaussian-16 calculation made by tests/filedsi/resourceTest;
the property is deleted from its .ecce-meta, and the editor is driven with
ECCE_CALCED_SCRIPT ("info").  A control with the property present must say the
same thing, so the check is about the missing property and nothing else.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402

CALC = "proj-g16/w-c2v"
WANT = "Not ready: choose a basis set"


def dropBasisProperty(calcdir):
    """Delete the calculation's basisSetDocument record; True if it was there.

    A directory's own properties are in its .ecce-meta under the name "."."""
    sidecar = os.path.join(calcdir, ".ecce-meta")
    with open(sidecar, errors="surrogateescape") as handle:
        lines = handle.read().split("\n")
    keep = [l for l in lines
            if not (l.split("\t")[0] == "."
                    and l.split("\t")[1:2] and l.split("\t")[1].endswith(":basisSetDocument"))]
    with open(sidecar, "w", errors="surrogateescape") as handle:
        handle.write("\n".join(keep))
    return len(keep) != len(lines)


def runCase(display, folder, label):
    calcdir = os.path.join(folder, "users", "local", CALC)
    url = "file://" + calcdir + "/"
    log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                       ["wait 3000", "ready", "info", "quit"], url)
    sym.check("Unhandled" not in log, "%s: the editor opens" % label,
              run_tests._tail(log, 15))
    sym.check("CALCED: formula=" in log, "%s: the editor shows the molecule" % label,
              run_tests._tail(log, 15))
    sym.check(WANT in log, "%s: it says a basis set must be chosen" % label,
              run_tests._tail(log, 15))


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "nobasis-localdata")
        problem = sym.makeCalculations(folder)
        if problem:
            results.fail("calced-no-basis", "calculations not made:\n" + problem)
            return
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            runCase(display, folder, "with the property")
            removed = dropBasisProperty(os.path.join(folder, "users", "local", CALC))
            sym.check(removed, "the fixture had a basisSetDocument property to remove")
            runCase(display, folder, "property missing")
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if sym.failures:
            results.fail("calced-no-basis", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
