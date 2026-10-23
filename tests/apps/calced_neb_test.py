#!/usr/bin/env python3
"""
ORCA NEB-TS through the real Calculation Editor (#419).

Sibling of calced_qst_test.py: the same HCN/HNC project built as ORCA
calculations, with "ts" a new calculation without a structure.  The editor
opens "ts", chooses Structure from "Other calculations (transition state)",
picks "react" as the reactant (its structure becomes the calculation's, the
Builder button is off), Transition State (NEB-TS) + Frequencies, "prod" as the
product and "guess" as the TS guess, sets the image count in Runtype Details
and saves.  The kept generator directory must hold neb_product.xyz and
neb_guess.xyz renumbered to C, N, H, and the deck must say NEB-TS Freq with the
%neb block naming them; the files must also be inputs of the calculation on
disk (they are what the launcher copies to the run directory).

Same installed tree, isolation and services as run_tests.py.  Exit 77 (skip)
without an install or Xvfb.
"""

import os
import threading
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
            steps = ["wait 9000", "ts-state",
                     "ts-source other", "wait 1500",
                     "ts-pick reactant react", "wait 7000", "ts-state",
                     "runtype Transition State (NEB-TS) + Frequencies",
                     "wait 3000", "ready", "basis 6-31G", "wait 1500",
                     "ts-pick product prod", "ts-pick guess guess",
                     "details runtype", "wait 8000",
                     "guiw %sImages 5" % KEY,
                     "button save-keep", "wait 6000", "ts-state", "quit"]
            shots = os.environ.get("ECCE_QST_SHOTS")
            if shots:
                steps.insert(-1, "shot neb-editor")
                os.makedirs(shots, exist_ok=True)
                threading.Thread(target=qst.photographer, daemon=True, args=(
                    display, fixture.stateDir(), shots, ["neb-editor"])).start()
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT", steps,
                               urls["ts"] + "/")
            tail = run_tests._tail(log, 40)
            states = re.findall(r"CALCED: ts-state (.*)", log)
            print("ts-state:", *states, sep="\n  ")
            sym.check(len(states) >= 3, "ts-state reports (%d)" % len(states), tail)
            if len(states) >= 3:
                sym.check("symmetry=off" in states[1] and "search=hidden" in states[1],
                          "Use symmetry is greyed for the NEB-TS source", tail)
                sym.check("builder=off" in states[1] and "atoms=3" in states[1],
                          "the reactant becomes the structure, Builder off", tail)
                rts = re.search(r"runtypes=(.*) line=", states[1]).group(1).split("|")
                sym.check(rts == ["Transition State (NEB-TS)",
                                  "Transition State (NEB-TS) + Frequencies"],
                          "only the NEB-TS run types are offered (%s)" % rts, tail)
                sym.check(sorted(re.search(r"candidates=(\S*)", states[1]).group(1).split(","))
                          == ["guess", "prod", "react"],
                          "the pickers list react, prod and guess, not ts", tail)
            dirs = re.findall(r"generator files kept in (\S+)", log)
            sym.check(len(dirs) == 1, "the save kept its files (%s)" % dirs, tail)
            status = re.findall(r"(Calculation saved as [^\n]*)", log)
            sym.check(len(status) == 2 and "Matched" in status[1],
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
                        coords = [t for l in open(os.path.join(d, f)).read().splitlines()[2:]
                                  for t in l.split()[1:4]]
                        sym.check(coords and all("." in t for t in coords),
                                  "%s: every coordinate has a decimal point" % f, tail)
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
                           and "/ts" in root]
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
