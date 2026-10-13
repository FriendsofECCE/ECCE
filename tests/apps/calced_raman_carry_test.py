#!/usr/bin/env python3
"""
A Raman tick survives a change of code in the Calculation Editor.

Each code keeps it under its own GUIValues key (ORCA and ECCE-QM
ES.Runtype.Vibration.UseRaman, Gaussian 16 ...ComputeRaman), and a code
change used to reset all the details dialogs.  The script ticks it on an
ECCE-QM calculation, saves, changes the code to ORCA and to Gaussian-16
(the question answered Yes through ECCE_TEST_DIALOG_ANSWER), saving in
between, and reads the target code's key back with the "guivalue" command.

Same installed tree, isolation and services as run_tests.py.  Exit 77
(skip) without an install or Xvfb.
"""

import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402

QM_CALC = os.path.join(REPO, "tests", "ecceqm", "fixtures", "o2-roks")
PROJECT_META = os.path.join(REPO, "tools", "screenshots", "data", "project.ecce-meta")
QM_KEY = "ES.Runtype.Vibration.UseRaman"
G16_KEY = "ES.Runtype.Vibration.ComputeRaman"


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "raman-localdata")
        shutil.rmtree(folder, ignore_errors=True)
        project = os.path.join(folder, "users", "local", "qm")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        shutil.copytree(QM_CALC, os.path.join(project, "o2"), symlinks=True)
        # A finished calculation is read-only and cannot change code.
        meta = os.path.join(project, "o2", ".ecce-meta")
        text = open(meta).read().replace("ecce:state\t\tComplete",
                                         "ecce:state\t\tReady")
        open(meta, "w").write(text)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        os.environ["ECCE_TEST_DIALOG_CLOSE"] = "1"
        os.environ["ECCE_TEST_DIALOG_ANSWER"] = "yes"
        try:
            steps = ["wait 9000", "ready", "runtype Vibration", "wait 3000",
                     "ready", "guivalue " + QM_KEY,
                     "gui " + QM_KEY + " 1", "guivalue " + QM_KEY,
                     "button save", "wait 4000",
                     "code ORCA", "wait 9000", "guivalue " + QM_KEY,
                     "button save", "wait 4000",
                     "code Gaussian-16", "wait 9000", "guivalue " + G16_KEY,
                     "quit"]
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT", steps,
                               "file://" + os.path.join(project, "o2") + "/")
            seen = re.findall(r"CALCED: guivalue (\S+)=(\S*)", log)
            tail = run_tests._tail(log, 40)
            sym.check(len(seen) == 4, "four guivalue lines (%s)" % (seen,), tail)
            if len(seen) == 4:
                sym.check(seen[1] == (QM_KEY, "1"),
                          "the ECCE-QM tick is set (%s)" % (seen[1],), tail)
                sym.check(seen[2] == (QM_KEY, "1"),
                          "ORCA keeps the Raman tick (%s)" % (seen[2],), tail)
                sym.check(seen[3] == (G16_KEY, "1"),
                          "Gaussian-16 keeps the Raman tick (%s)" % (seen[3],),
                          tail)
        finally:
            restorePrefs()
            for key in ("ECCE_LOCAL_DATA", "ECCE_TEST_DIALOG_CLOSE",
                        "ECCE_TEST_DIALOG_ANSWER"):
                os.environ.pop(key, None)
        if sym.failures:
            results.fail("calced-raman-carry", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
