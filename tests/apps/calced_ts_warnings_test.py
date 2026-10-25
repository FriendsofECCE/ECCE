#!/usr/bin/env python3
"""
Warnings for TS calculations built from two structures (#468), ORCA NEB-TS
through the real Calculation Editor.

The project is the HCN/HNC one of calced_qst_test.py with linear end points
(HCN, HNC on the z axis) and the bent guess; "react" was set up at
DFT/B3LYP, "prod" and "guess" have no stored level, "ts" is a new calculation
at its default theory with 6-31G.  Checked, all warnings and none blocking:
  * reactant and product picked, no guess: the level warning naming both
    levels, the linear-path warning and the passes-through warning;
  * the bent guess picked: the path warnings are gone, the level one stays;
  * Save with "Re-optimise the end points" on Automatic and differing levels:
    the %neb block holds Preopt_Ends TRUE;
  * both end points taken from calculations with no stored level: no warning,
    no Preopt_Ends, and the option "Yes" in Runtype Details forces it.

Same installed tree, isolation and services as run_tests.py.  Exit 77 (skip)
without an install or Xvfb.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import calced_qst_test as qst   # noqa: E402
import fixture                  # noqa: E402
import run_tests                # noqa: E402
import symmetry_find_test as sym  # noqa: E402

KEY = "ES.Runtype.NEB."


def deckOf(d):
    for f in os.listdir(d):
        if f.endswith(".orcain"):
            return open(os.path.join(d, f), errors="replace").read()
    return ""


def main():
    os.environ["ECCE_QST_CODE"] = "ORCA"
    os.environ["ECCE_QST_LEVEL"] = "react=DFT/RDFT/B3LYP"

    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "ts-warn-localdata")
        urls, err = qst.makeProject(folder, linear=True)
        if urls is None:
            results.fail("calced-ts-warnings", "could not build the project: %s" % err)
            return
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            steps = ["wait 9000", "ts-source other", "wait 1500",
                     "ts-pick reactant react", "wait 7000",
                     "runtype Transition State (NEB-TS)", "wait 3000", "ready",
                     "basis 6-31G", "wait 1500",
                     "ts-pick product prod", "ts-warnings",
                     "ts-pick guess guess", "ts-warnings",
                     "button save-keep", "wait 6000",
                     # end points without a stored level
                     "ts-pick reactant prod", "wait 3000",
                     "ts-pick product guess", "ts-warnings",
                     "button save-keep", "wait 6000",
                     "details runtype", "wait 8000",
                     "guiw %sPreoptEnds Yes" % KEY,
                     "button save-keep", "wait 6000", "quit"]
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT", steps,
                               urls["ts"] + "/")
            tail = run_tests._tail(log, 40)
            ws = re.findall(r"ECCE_CALCED_SCRIPT: ts-warnings: (.*)", log)
            print("ts-warnings:", *ws, sep="\n  ")
            sym.check(len(ws) == 3, "ts-warnings reports (%d)" % len(ws), tail)
            if len(ws) == 3:
                sym.check("Reactant optimised at B3LYP" in ws[0]
                          or "Reactant calculated at B3LYP" in ws[0],
                          "the level warning names the reactant's level", tail)
                sym.check(re.search(r"this calculation is \S*RHF\S*/6-31G", ws[0])
                          is not None, "... and this calculation's level", tail)
                sym.check("not minima at this level" in ws[0], "... and why", tail)
                sym.check("linear reactant and product" in ws[0].lower(),
                          "the linear path is warned about", tail)
                sym.check(re.search(r"passes \w+ through \w+", ws[0]) and ws[0].count("TS Search") == 1,
                          "the pass-through is warned about, in one line with the linear case", tail)
                sym.check("TS Search" in ws[0] and "Guess" in ws[0],
                          "... pointing to the Guess pick", tail)
                sym.check("levels-differ=yes" in ws[0], "the levels differ", tail)
                sym.check("both linear" not in ws[1] and "passes" not in ws[1]
                          and "B3LYP" in ws[1],
                          "a bent guess removes the path warnings only", tail)
                sym.check("not minima" not in ws[2] and "levels-differ=no" in ws[2],
                          "no level warning without stored levels", tail)
            dirs = re.findall(r"generator files kept in (\S+)", log)
            sym.check(len(dirs) == 3, "three saves kept their files (%s)" % dirs, tail)
            if len(dirs) == 3:
                decks = [deckOf(d) for d in dirs]
                sym.check("Preopt_Ends TRUE" in decks[0],
                          "Automatic + differing levels: Preopt_Ends TRUE", tail)
                sym.check("Preopt_Ends" not in decks[1],
                          "Automatic + same level: no Preopt_Ends", tail)
                sym.check("Preopt_Ends TRUE" in decks[2],
                          "Yes forces Preopt_Ends TRUE", tail)
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
            os.environ.pop("ECCE_QST_LEVEL", None)
        if sym.failures:
            results.fail("calced-ts-warnings", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
