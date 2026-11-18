#!/usr/bin/env python3
"""
A Details button must not open an empty dialog (#676).

A Gaussian DFT "Absorbance" run type has no Runtype Details panel, so the
editor's Runtype Details button must be disabled and refusing to launch the
dialog (which would open empty).  The Theory Details button, whose dialog does
have fields, stays enabled as a control.  Energy has the counterpoise option
(#1091), so there the Runtype Details button is enabled.  The CalcEd script command "enabled"
reports the button states and "details" clicks one, respecting its enabled
state.

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
STATE = re.compile(r"theory-details=(\w+) runtype-details=(\w+)")


CHOICE = re.compile(r"CALCED: theory=(.*) runtype=(.*) launch=")


def makeCalc(project, name, theory, runtype):
    source = os.path.join(FIX, "calc-polar-h2o-g16")
    path = os.path.join(project, name)
    shutil.copytree(source, path, symlinks=True)
    # No stored run type values, so the button state comes from what the
    # run type dialog itself exports.
    params = os.path.join(path, "Parameters")
    os.makedirs(params, exist_ok=True)
    with open(os.path.join(params, "SetupParams"), "w") as handle:
        handle.write("ES.Theory.SCF.ConvergenceAlgorithm|Quadratic||1|0|menu_input\n")
    meta = os.path.join(path, ".ecce-meta")
    with open(meta, errors="surrogateescape") as handle:
        text = handle.read()
    text = re.sub(r"(ecce:runtype\t\t)[^\n]*", r"\g<1>" + runtype, text)
    text = re.sub(r"(ecce:theory\t\t)[^\n]*", r"\g<1>" + theory, text)
    with open(meta, "w", errors="surrogateescape") as handle:
        handle.write(text)
    return path


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "detailsguard-localdata")
        shutil.rmtree(folder, ignore_errors=True)
        project = os.path.join(folder, "users", "local", "p")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            # Absorbance is not offered for RHF; the editor would silently
            # switch to Energy, so the guard case needs a DFT theory.
            calc = makeCalc(project, "absorbance", "DFT/RDFT", "Absorbance")
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                               ["wait 9000", "info", "enabled",
                                "details runtype", "quit"],
                               "file://" + calc + "/")
            choice = CHOICE.search(log)
            sym.check(choice is not None and choice.group(2) == "Absorbance",
                      "the editor keeps the Absorbance run type",
                      choice.group(0) if choice else run_tests._tail(log, 30))
            states = STATE.findall(log)
            sym.check(len(states) == 1,
                      "the editor reported the Details buttons",
                      run_tests._tail(log, 30))
            if states:
                sym.check(states[0] == ("on", "off"),
                          "Runtype Details is disabled for Absorbance (theory stays on)",
                          str(states[0]))
            sym.check("FAIL: button disabled" in log,
                      "invoking Runtype Details opens no dialog",
                      run_tests._tail(log, 30))

            calc = makeCalc(project, "energy", "SCF/RHF", "Energy")
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                               ["wait 9000", "info", "enabled", "quit"],
                               "file://" + calc + "/")
            choice = CHOICE.search(log)
            sym.check(choice is not None and choice.group(2) == "Energy",
                      "the editor keeps the Energy run type",
                      choice.group(0) if choice else run_tests._tail(log, 30))
            states = STATE.findall(log)
            sym.check(states == [("on", "on")],
                      "Runtype Details is enabled for Energy (counterpoise)",
                      str(states) + "\n" + run_tests._tail(log, 30))
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if sym.failures:
            results.fail("calced-details-guard", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
