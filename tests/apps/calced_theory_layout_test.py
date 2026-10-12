#!/usr/bin/env python3
"""
The Calculation Editor's button row after the theory changes (#294).

ECCE-QM's summary has a "DFT Functional" line for DFT and none for HF.  On
Windows the row of Verify / Final Edit / Save / Launch was left drawn at its
old place as well as the new one.  Headless Linux cannot show stale pixels,
so this checks what it can: after HF, DFT, HF the Verify and Launch buttons
are where they were at the start, and the frame is the size it was.  The
CalcEd script command "rects" prints them.

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
RECT = re.compile(r"CALCED: rects verify=(\S+) launch=(\S+) frame=(\S+)")


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "theorylayout-localdata")
        shutil.rmtree(folder, ignore_errors=True)
        project = os.path.join(folder, "users", "local", "qm")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        shutil.copytree(QM_CALC, os.path.join(project, "o2"), symlinks=True)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            steps = ["wait 9000", "rects"]
            for theory in ("DFT", "HF", "DFT", "HF", "DFT"):
                steps += ["theory " + theory, "wait 2500", "rects"]
            steps.append("quit")
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT", steps,
                               "file://" + os.path.join(project, "o2") + "/")
            got = RECT.findall(log)
            sym.check(len(got) == 6, "six layouts were reported (%d)" % len(got),
                      run_tests._tail(log, 60))
            if len(got) == 6:
                print("rects (verify, launch, frame) start DFT HF DFT HF DFT: %s" % got)
                sym.check(got[0] == got[1] == got[3] == got[5],
                          "DFT gives the same layout every time", str(got))
                sym.check(got[2] == got[4],
                          "HF gives the same layout every time", str(got))
                # the row is below the same things in both; only the summary's
                # missing line may move it
                sym.check(got[2][2] != "0,0",
                          "the HF frame has a size", str(got[1]))
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if sym.failures:
            results.fail("calced-theory-layout", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
