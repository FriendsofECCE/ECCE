#!/usr/bin/env python3
"""
Saving a stored calculation in the Calculation Editor regenerates its deck;
the decks must not change when the generator code is refactored (DeckWriter,
#621).  One calculation per code (ORCA, Gaussian-16, NWChem and ECCE-QM from
stored fixtures; MOPAC, xTB, Quantum ESPRESSO and CP2K by switching the ORCA
water calculation to that code) is opened headless and saved; every file the
Save leaves in Inputs/ is compared with tests/apps/fixtures/decks/<label>/.

    DECK_REGEN_OUT=<dir> tests/apps/deck_regen_test.py   write the decks there
                                                         instead of comparing

Same installed tree, isolation and services as run_tests.py.  Exit 77 (skip)
without an install or Xvfb.
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
import calced_constraints_line_test as cl  # noqa: E402

FIX = os.path.join(HERE, "fixtures")
EXPECT = os.path.join(FIX, "decks")
ORCA = os.path.join(FIX, "calc-orca-water-ccpvdz")
# label, stored calculation, code to switch to ("" keeps the stored one)
CASES = [
    ("orca", ORCA, ""),
    ("g16", os.path.join(FIX, "calc-polar-h2o-g16"), ""),
    ("nwchem", os.path.join(FIX, "calc-water-opt"), ""),
    ("ecceqm", os.path.join(REPO, "tests", "ecceqm", "fixtures", "h2o-geometry"), ""),
    ("mopac", ORCA, "MOPAC"),
    ("xtb", ORCA, "xTB"),
    ("qe", ORCA, "QuantumESPRESSO"),
    ("cp2k", ORCA, "CP2K"),
]


# Silicon in its primitive cell, for the codes that need a periodic structure.
SI = """title: si
type: crystal
num_atoms: 2
atom_info: symbol cart
lattice_vectors:
-2.715 0.0 2.715
0.0 2.715 2.715
-2.715 2.715 0.0
atom_list:
Si 0.0 0.0 0.0
Si -1.30 1.40 1.35
attr_list:
 0.000000   1   0   0   0
 0.000000   1   0   0   0
atom_type_list:
       14   2
       14   2
num_bonds: 0
bond_list:
"""
PERIODIC = ("qe", "cp2k")


def decks(calc):
    inputs = os.path.join(calc, "Inputs")
    out = {}
    if os.path.isdir(inputs):
        for name in sorted(os.listdir(inputs)):
            path = os.path.join(inputs, name)
            if os.path.isfile(path):
                out[name] = open(path, "rb").read()
    return out


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "deckregen-localdata")
        shutil.rmtree(folder, ignore_errors=True)
        project = os.path.join(folder, "users", "local", "p")
        os.makedirs(project)
        shutil.copy(cl.PROJECT_META, os.path.join(project, ".ecce-meta"))
        os.environ["ECCE_LOCAL_DATA"] = folder
        os.environ["ECCE_TEST_DIALOG_CLOSE"] = "2"
        os.environ["ECCE_TEST_DIALOG_ANSWER"] = "yes"
        write = os.environ.get("DECK_REGEN_OUT")
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            for label, source, code in CASES:
                calc = os.path.join(project, label)
                shutil.copytree(source, calc, symlinks=True)
                shutil.rmtree(os.path.join(calc, "Inputs"), ignore_errors=True)
                os.makedirs(os.path.join(calc, "Inputs"))
                # Not run, so that Save is allowed to change it.
                meta = os.path.join(calc, ".ecce-meta")
                text = open(meta, errors="surrogateescape").read()
                text = re.sub(r"(ecce:state\t\t)[^\n]*", r"\g<1>Ready", text)
                open(meta, "w", errors="surrogateescape").write(text)
                if label in PERIODIC:
                    open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w").write(SI)
                steps = ["wait 9000"]
                if code:
                    steps += ["code " + code, "wait 4000"]
                steps += ["button save", "wait 8000", "quit"]
                log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT", steps,
                                   "file://" + calc + "/")
                got = decks(calc)
                sym.check(bool(got), "%s: Save wrote a deck" % label,
                          run_tests._tail(log, 30))
                if write:
                    for fname, data in got.items():
                        os.makedirs(os.path.join(write, label), exist_ok=True)
                        open(os.path.join(write, label, fname), "wb").write(data)
                    continue
                want = {}
                edir = os.path.join(EXPECT, label)
                if os.path.isdir(edir):
                    for fname in os.listdir(edir):
                        want[fname] = open(os.path.join(edir, fname), "rb").read()
                sym.check(got == want, "%s: the deck is the stored one (%s vs %s)"
                          % (label, sorted(got), sorted(want)))
        finally:
            restorePrefs()
            for var in ("ECCE_LOCAL_DATA", "ECCE_TEST_DIALOG_CLOSE",
                        "ECCE_TEST_DIALOG_ANSWER"):
                os.environ.pop(var, None)
        if sym.failures:
            results.fail("deck-regen", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
