#!/usr/bin/env python3
"""
CalcEd script gui/guiw keep a dependent field's sensitivity right (#1111).

ORCA's Solvation model and solvent follow "Use implicit solvation".  The
script's gui/guiw skip the dialog, so the model used to stay stored as
insensitive and ai.orca refused the empty model.  Checked: with solvation on
the save works, the input has CPCM(water) and SetupParams stores the model
as sensitive; with it switched off again the model is stored insensitive and
is not in the input.  Also #1112: the Organizer's "setup basis assign
ma-def2-svp" gives an ORCA calculation with C, H and O that basis.

Same installed tree, isolation and services as run_tests.py.  Exit 77
(skip) without an install or Xvfb.
"""

import os
import re
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402

PROJECT_META = os.path.join(REPO, "tools", "screenshots", "data", "project.ecce-meta")
SOURCE = os.path.join(HERE, "fixtures", "calc-orca-water-ccpvdz")
METHANOL = os.path.join(HERE, "fixtures", "calc-orca-methanol-water-tzvpd")
P = "ES.Theory.SCF."


def makeCalc(project, name, source):
    path = os.path.join(project, name)
    shutil.copytree(source, path, symlinks=True)
    meta = os.path.join(path, ".ecce-meta")
    text = open(meta, errors="surrogateescape").read().replace(
        "ecce:state\t\tComplete", "ecce:state\t\tReady")
    open(meta, "w", errors="surrogateescape").write(text)
    # The fixture has no Inputs folder; without it the save cannot store the deck.
    os.makedirs(os.path.join(path, "Inputs"), exist_ok=True)
    return path


def decks(calc):
    found = []
    for root, _dirs, names in os.walk(calc):
        for n in names:
            if n.endswith(".orcain"):
                found.append(open(os.path.join(root, n), errors="replace").read())
    return "\n".join(found)


def stored(calc, key):
    path = os.path.join(calc, "Parameters", "SetupParams")
    for line in open(path, errors="replace") if os.path.exists(path) else []:
        f = line.rstrip("\n").split("|")
        if f[0] == key:
            return f
    return None


def runScript(display, calc, steps):
    return sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                        ["wait 9000", "ready"] + steps + ["quit"],
                        "file://" + calc + "/")


def solvation(display, project):
    on = makeCalc(project, "solv-on", SOURCE)
    log = runScript(display, on, [
        "details theory", "ready",
        "guiw " + P + "UseSolvation 1", "guiw " + P + "SolvationModel CPCM",
        "guiw " + P + "Solvent water", "button save", "wait 8000"])
    tail = run_tests._tail(log, 40)
    sym.check("not generated" not in log and "not supported" not in log,
              "solvation on: the save generates the input", tail)
    sym.check(re.search(r"CPCM\(water\)", decks(on), re.I) is not None,
              "solvation on: the input has CPCM(water)", decks(on)[:600])
    model = stored(on, P + "SolvationModel")
    sym.check(model is not None and model[1] == "CPCM" and model[3] == "1",
              "solvation on: the model is stored as sensitive (%s)" % (model,), tail)

    off = makeCalc(project, "solv-off", SOURCE)
    log = runScript(display, off, [
        "details theory", "ready",
        "guiw " + P + "UseSolvation 1", "guiw " + P + "SolvationModel CPCM",
        "guiw " + P + "Solvent water", "guiw " + P + "UseSolvation 0",
        "button save", "wait 8000"])
    tail = run_tests._tail(log, 40)
    sym.check("not generated" not in log, "solvation off: the save works", tail)
    sym.check(decks(off) != "" and re.search(r"CPCM|SMD", decks(off)) is None,
              "solvation off: there is a deck without the model", decks(off)[:600])
    model = stored(off, P + "SolvationModel")
    sym.check(model is not None and model[3] == "0",
              "solvation off: the model stays insensitive (%s)" % (model,), tail)


def basisPick(display, project):
    """#1112: pick a named basis from outside the quick menu."""
    calc = makeCalc(project, "meoh", METHANOL)
    work = tempfile.mkdtemp(prefix="ecce-1112-")
    cmds = os.path.join(work, "commands")
    open(cmds, "w").close()

    def append(line, wait):
        with open(cmds, "a") as handle:
            handle.write(line + "\n")
        time.sleep(wait)

    def inspect(_display):
        append("start CalculationEditor file://" + calc, 25)
        append("tabs", 2)
        # library names are lower case; assign also saves
        append("setup basis assign ma-def2-svp", 15)
        append("setup basis", 3)
        append("close", 3)
        append("answer no", 10)

    try:
        import one_process_test as op
        op.setLayout(display, "normal")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1", "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": os.environ["ECCE_LOCAL_DATA"]},
                          inspect=inspect)
        log = result.log or ""
        areas = re.findall(r"ECCE_TEST_ORGANIZER: setup[^:]*: (area=basis.*)", log)
        tail = run_tests._tail(log, 40)
        sym.check(bool(areas) and "all_atoms:_ma-def2-svp" in areas[-1].lower(),
                  "#1112: ma-def2-svp is assigned to all atoms (C, H, O) (%s)"
                  % (areas[-1] if areas else "no basis line"), tail)
        sym.check("ma-def2-svp" in decks(calc).lower(),
                  "#1112: the generated input uses ma-def2-SVP", decks(calc)[:600])
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "solvation-localdata")
        shutil.rmtree(folder, ignore_errors=True)
        project = os.path.join(folder, "users", "local", "p")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            solvation(display, project)
            basisPick(display, project)
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if sym.failures:
            results.fail("calced-script-solvation", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
