#!/usr/bin/env python3
"""
The Calculation Editor's Theory Details and Runtype Details buttons (#427).

A calculation at its defaults stores no dialog values.  The buttons used to
be enabled only once values existed, so on open the Runtype Details button
was grey, while choosing the same run type again enabled it.  For each code
the enabled state on open must equal the state after re-choosing the current
theory and run type.  The CalcEd script command "enabled" reports them.

Same installed tree, isolation and services as run_tests.py.  Exit 77
(skip) without an install or Xvfb.
"""

import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
REPO = os.path.dirname(REPO)
sys.path.insert(0, HERE)

import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402

PROJECT_META = os.path.join(REPO, "tools", "screenshots", "data", "project.ecce-meta")
FIX = os.path.join(HERE, "fixtures")
# name, fixture, run type to set in the copy (None = as stored)
CASES = [("orca-geovib", os.path.join(FIX, "calc-orca-water-ccpvdz"), "GeoVib"),
         ("orca-energy", os.path.join(FIX, "calc-orca-water-ccpvdz"), None),
         ("nwchem", os.path.join(FIX, "calc-water-opt"), None),
         ("g16", os.path.join(FIX, "calc-polar-h2o-g16"), None),
         ("ecceqm", os.path.join(REPO, "tests", "ecceqm", "fixtures", "h2o-geovib"), None)]
STATE = re.compile(r"theory-details=(\w+) runtype-details=(\w+)")
CHOICE = re.compile(r"CALCED: theory=(.*) runtype=(.*) launch=")


def makeCalc(project, name, source, runtype):
    path = os.path.join(project, name)
    shutil.copytree(source, path, symlinks=True)
    # Theory values only: a calculation whose run type is at its defaults
    # stores none for it (the state that showed the bug).
    params = os.path.join(path, "Parameters")
    os.makedirs(params, exist_ok=True)
    with open(os.path.join(params, "SetupParams"), "w") as handle:
        handle.write("ES.Theory.SCF.ConvergenceAlgorithm|Quadratic||1|0|menu_input\n")
    if runtype:
        meta = os.path.join(path, ".ecce-meta")
        with open(meta, errors="surrogateescape") as handle:
            text = handle.read()
        text = re.sub(r"(ecce:runtype\t\t)[^\n]*", r"\g<1>" + runtype, text)
        with open(meta, "w", errors="surrogateescape") as handle:
            handle.write(text)
    return path


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "detailsenabled-localdata")
        shutil.rmtree(folder, ignore_errors=True)
        project = os.path.join(folder, "users", "local", "p")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            for label, source, runtype in CASES:
                calc = makeCalc(project, label, source, runtype)
                url = "file://" + calc + "/"
                # read the choices, then choose them again
                first = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                                     ["wait 9000", "info", "enabled", "quit"], url)
                choice = CHOICE.search(first)
                sym.check(choice is not None, "%s: the editor opens" % label,
                          run_tests._tail(first, 30))
                if choice is None:
                    continue
                theory, runtype = choice.group(1), choice.group(2)
                opened = STATE.findall(first)
                again = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                                     ["wait 9000", "theory " + theory,
                                      "wait 3000", "runtype " + runtype,
                                      "wait 4000", "enabled", "quit"], url)
                after = STATE.findall(again)
                print("%s (%s, %s): on open %s, after re-choosing %s"
                      % (label, theory, runtype, opened, after))
                sym.check(len(opened) == 1 and len(after) == 1,
                          "%s: both states were reported" % label,
                          run_tests._tail(again, 30))
                if len(opened) == 1 and len(after) == 1:
                    sym.check(opened == after,
                              "%s: Details buttons on open equal those after "
                              "re-choosing" % label, "%s vs %s" % (opened, after))
                    sym.check(opened[0] == ("on", "on"),
                              "%s: both Details buttons are enabled" % label,
                              str(opened))
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if sym.failures:
            results.fail("calced-details-enabled", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
