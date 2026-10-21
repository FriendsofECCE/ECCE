#!/usr/bin/env python3
"""
ORCA NEB-TS through the real Calculation Editor (#419).

Sibling of calced_qst_test.py: the same three-calculation HCN/HNC project,
built as ORCA calculations.  The editor opens "react" (HCN, this calculation
is the reactant), chooses Transition State (NEB-TS) + Frequencies, picks "prod"
(HNC written H, N, C) as the product and "ts" as the TS guess in the Runtype
Details, and saves.  The kept generator directory must hold neb_product.xyz and
neb_guess.xyz renumbered to C, N, H, and the deck must say NEB-TS Freq with the
%neb block naming them; the files must also be inputs of the calculation on
disk (they are what the launcher copies to the run directory).

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


def xyzOrder(path):
    lines = open(path).read().splitlines()
    return [l.split()[0] for l in lines[2:] if l.strip()]


def main():
    os.environ["ECCE_QST_CODE"] = "ORCA"

    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "neb-localdata")
        urls, err = qst.makeProject(folder)
        if urls is None:
            results.fail("calced-neb", "could not build the project: %s" % err)
            return
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            steps = ["wait 9000", "ready",
                     "runtype Transition State (NEB-TS) + Frequencies",
                     "wait 3000", "ready", "basis 6-31G", "wait 1500",
                     "details runtype", "wait 8000", "qst-candidates",
                     "guiw %sProduct %s" % (KEY, urls["prod"]),
                     "guiw %sGuess %s" % (KEY, urls["ts"]),
                     "guiw %sImages 5" % KEY,
                     "button save-keep", "wait 6000", "quit"]
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT", steps,
                               urls["react"] + "/")
            tail = run_tests._tail(log, 40)
            cands = re.findall(r"CALCED: qst-candidate (\S+)\t(\S+)\t(\S+)", log)
            sym.check(sorted(c[0] for c in cands) == ["prod", "ts"],
                      "the dialog is offered prod and ts, not react (%s)"
                      % [c[0] for c in cands], tail)
            dirs = re.findall(r"generator files kept in (\S+)", log)
            sym.check(len(dirs) == 1, "the save kept its files (%s)" % dirs, tail)
            status = re.findall(r"(Calculation saved as [^\n]*)", log)
            sym.check(len(status) == 1 and "Matched" in status[0],
                      "the status line reports the atom matching (%s)" % status, tail)
            if len(dirs) == 1:
                d = dirs[0]
                for f in ("neb_product.xyz", "neb_guess.xyz"):
                    ok = os.path.exists(os.path.join(d, f))
                    sym.check(ok, "%s is written next to the deck" % f, tail)
                    if ok:
                        order = xyzOrder(os.path.join(d, f))
                        sym.check(order == ["C", "N", "H"],
                                  "%s atoms C, N, H (renumbered) (%s)" % (f, order), tail)
                deck = ""
                for f in os.listdir(d):
                    if f.endswith(".orcain"):
                        deck = open(os.path.join(d, f), errors="replace").read()
                print("----- deck\n%s" % deck)
                sym.check(re.search(r"^! .*\bNEB-TS Freq\b", deck, re.M) is not None,
                          "the route line says NEB-TS Freq", tail)
                sym.check('NEB_End_XYZFile "neb_product.xyz"' in deck
                          and 'NEB_TS_XYZFile "neb_guess.xyz"' in deck
                          and "Nimages 5" in deck, "the %neb block names both files", tail)
            staged = []
            for root, _, files in os.walk(folder):
                staged += [os.path.join(root, f) for f in files
                           if f in ("neb_product.xyz", "neb_guess.xyz")
                           and "/react" in root]
            sym.check(len(staged) == 2, "both files are inputs of the calculation "
                      "(%s)" % [os.path.basename(s) for s in staged], tail)
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if sym.failures:
            results.fail("calced-neb", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
