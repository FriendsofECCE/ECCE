#!/usr/bin/env python3
"""
The Builder's Symmetry > Find, then the Calculation Editor: the editor shows
the molecule that was stored.

Find marks the whole molecule as its symmetry-unique atoms, and the editor
rebuilds the full molecule from them with genmol.  genmol used to add an
image of every atom that was already there, so water became "H4O, 5 atoms,
12 electrons" in the editor while the Organizer said H2O.

Local data mode, calculations made by tests/filedsi/resourceTest (a C2v
water and a methane); the Builder runs Find and saves (ECCE_BUILDER_SCRIPT),
the editor reports its chemical-system fields (ECCE_CALCED_SCRIPT "info"),
and for the MOPAC methane it also saves, and the deck must hold 5 atoms.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import getpass
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

failures = []

CASES = [
    #  calculation, point group Find gives, formula, atoms, electrons
    ("proj-g16/w-c2v", "C2v", "H2O", 3, 10),
    ("proj-mopac/ch4", "Td", "CH4", 5, 10),
]


def check(ok, what, detail=""):
    print("%s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
        if detail:
            print(detail)


def makeCalculations(folder):
    """The local data folder, with the calculations resourceTest makes."""
    build = os.environ.get("ECCE_TEST_BUILD", os.path.join(REPO, "build-cmake"))
    driver = os.path.join(fixture.stateDir(), "resourceTest")
    libs = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
            "eccecipc", "ecceutil", "eccecomm", "eccercmd"]
    built = subprocess.run(
        ["g++", "-O0", "-w", "-I", os.path.join(REPO, "include"), "-o", driver,
         os.path.join(REPO, "tests", "filedsi", "resourceTest.C"), "-L" + build]
        + ["-l" + l for l in libs] * 3
        + ["-lxerces-c", "-lmosquitto", "-lssl", "-lcrypto"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if built.returncode != 0:
        return built.stdout.decode()[-1500:]
    shutil.rmtree(folder, ignore_errors=True)
    user = os.path.join(folder, "users", "local")
    os.makedirs(user)
    env = dict(os.environ, ECCE_HOME=apps.INSTALL, ECCE_LOCAL_DATA=folder,
               ECCE_REALUSER=getpass.getuser(), ECCE_NO_MESSAGING="1",
               LD_LIBRARY_PATH=build)
    for mode in ("g16", "mopac"):
        done = subprocess.run([driver, mode, user], env=env,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if done.returncode != 0:
            return done.stdout.decode()[-1500:]
    return None


def scripted(display, app, var, script, url):
    path = os.path.join(fixture.stateDir(), "%s.script" % app)
    with open(path, "w") as handle:
        handle.write("\n".join(script) + "\n")
    result = apps.run(display, app, args=("-context", url), windowTimeout=60,
                      settle=90, env={var: path, "ECCE_FEEDBACK_STDERR": "1"})
    return result.log or ""


def runCase(display, folder, calc, group, formula, atoms, electrons):
    calcdir = os.path.join(folder, "users", "local", calc)
    url = "file://" + calcdir + "/"
    name = calc.split("/")[-1]
    log = scripted(display, "builder", "ECCE_BUILDER_SCRIPT",
                   ["wait 4000", "symmetry", "save", "wait 3000", "quit"], url)
    m = re.search(r"BUILDER: pointgroup=(\S+)", log)
    check(m is not None and m.group(1) == group,
          "%s: Find gives %s (got %s)" % (name, group, m.group(1) if m else "nothing"),
          run_tests._tail(log, 15))
    steps = ["wait 3000", "ready", "info"]
    mopac = "mopac" in calc
    if mopac:
        steps += ["button save", "wait 5000"]
    steps.append("quit")
    log = scripted(display, "calced", "ECCE_CALCED_SCRIPT", steps, url)
    m = re.search(r"CALCED: formula=(\S*) atoms=(\S*) electrons=(\S*) "
                  r"symmetry=(\S*)", log)
    got = m.groups() if m else ("?", "?", "?", "?")
    check(m is not None and got[0] == formula and got[1] == str(atoms)
          and got[2] == str(electrons),
          "%s: the editor shows %s, %d atoms, %d electrons (got %s, %s atoms, "
          "%s electrons, %s)" % (name, formula, atoms, electrons, got[0], got[1],
                                 got[2], got[3]),
          run_tests._tail(log, 15))
    if mopac:
        inputs = os.path.join(calcdir, "Inputs")
        decks = [f for f in os.listdir(inputs) if f.endswith(".mop")] \
            if os.path.isdir(inputs) else []
        count = -1
        if decks:
            lines = open(os.path.join(inputs, decks[0]), errors="replace").read()
            count = len(re.findall(r"^\s*(?:C|H)\s+-?\d", lines, re.M))
        check(count == atoms, "%s: the saved deck holds %d atoms (got %d)"
              % (name, atoms, count), run_tests._tail(log, 15))


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "symfind-localdata")
        problem = makeCalculations(folder)
        if problem:
            results.fail("symmetry-find", "calculations not made:\n" + problem)
            return
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            for case in CASES:
                runCase(display, folder, *case)
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if failures:
            results.fail("symmetry-find", "%d wrong: %s"
                         % (len(failures), "; ".join(failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
