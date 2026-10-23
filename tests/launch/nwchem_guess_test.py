#!/usr/bin/env python3
"""NWChem: a finished job's final MO vectors as the guess of the next one (#407).

Water, SCF/STO-3G, through the real Launch, gensub, eccejobmonitor and
eccejobstore:

  1. job A runs and its nwchem.movecs is stored with the calculation, byte
     for byte what NWChem wrote; before the run "Duplicate with Final
     Orbitals" is not available, after it is;
  2. job B is made as that menu item makes it (copy, reset, movecs attached as
     guess.movecsin); the deck made by the real ai.nwchem says
     "vectors input guess.movecsin output nwchem.movecs", the file survives
     the removal of the old deck and is staged to the run directory, and
     NWChem reads it and needs fewer SCF cycles than A.

    tests/launch/nwchem_guess_test.py [--build build-cmake] [--central] [--keep]

Exit status 77 (CTest SKIP) without NWChem or the built binaries.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402

PARSERS = os.path.join(REPO, "scripts", "parsers")
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def findNW():
    return shutil.which("nwchem") or ""


def makeDeck(work, name, guess):
    """The deck as the Calculation Editor makes it, from the real ai.nwchem."""
    folder = os.path.join(work, name)
    os.makedirs(folder)
    with open(os.path.join(folder, "w.param"), "w") as f:
        f.write("title: w\nannotation: water\nCategory: SCF\nTheory: RHF\nRunType: Energy\nCharge: 0\n"
                "Symmetry: C1\nNumElectrons: 10\nChemSys.Multiplicity: 1\n"
                "MemorySize: 1\nNopen: 0\n")
        if guess:
            f.write("ES.Theory.SCF.OrbitalGuessFile: guess.movecsin\n")
    with open(os.path.join(folder, "w.frag"), "w") as f:
        f.write("fragment\ntitle: w\nnum_atoms: 3\natom_info: symbol\n"
                "atom_list:\n"
                "O 0.0 0.0 0.1173\nH 0.0 0.7572 -0.4692\nH 0.0 -0.7572 -0.4692\n")
    with open(os.path.join(folder, "w.basis"), "w") as f:
        f.write("useRouteCard STO-3G false\n")
    deck = os.path.join(folder, "deck")
    shutil.copy(os.path.join(PARSERS, "nwch.tpl"), deck)
    subprocess.run(["perl", os.path.join(PARSERS, "ai.nwchem"), "-n", "w", "-p",
                    "-f", "-b", "-t", deck], cwd=folder,
                   env=dict(os.environ, ECCE_HOME=REPO), check=True,
                   capture_output=True)
    #  A library basis block in place of the Calculation Editor's basis page.
    text = open(deck).read().replace("useRouteCard STO-3G false",
                                     "basis\n * library sto-3g\nend")
    open(deck, "w").write(text)
    return deck


def calcDir(s, url):
    path = url.rstrip("/")
    if path.startswith("file://"):
        return path[len("file://"):]
    parts = path.split("/Ecce/users/", 1)[1]
    return os.path.join(s.fixture.stateDir(), "htdocs", "Ecce", "users", parts)


def cycles(output):
    """Iterations of the SCF table (its last one, should there be several)."""
    n = None
    for line in output.splitlines():
        m = re.match(r"\s+(\d+)\s+-?\d+\.\d+\s+\d\.\d+D[-+]\d+\s", line)
        if m:
            n = int(m.group(1))
    return n


def energy(output):
    m = re.findall(r"Total SCF energy =\s+(-?\d+\.\d+)", output)
    return float(m[-1]) if m else None


def runJob(s, name, deck, rundir):
    url, out = s.create(name, "nwchem_es", deck, "nwch.nw", rundir)
    if not check(url is not None, "%s created" % name):
        say(out)
    return url


def finish(s, url, name):
    rc, out = s.launch(url)
    if not check(rc == 0, "%s launched" % name):
        say(out)
        return None
    state = s.waitState(url, seconds=300)
    if not check(state == "completed", "%s completed (last: %s)" % (name, state)):
        rc, why = s.driver("reason", url)
        say(why)
        return None
    return calcDir(s, url)


def log(calc):
    return open(os.path.join(calc, "Outputs", "nwch.nwout"), errors="replace").read()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--central", action="store_true")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    nw = findNW()
    if not nw:
        harness.skip("NWChem is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "launchjob"), os.X_OK):
        harness.skip("launchjob is not built")

    s = harness.Session(build, "nwguess", {"NWChem": nw},
                        local=not args.central, keep=args.keep)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    os.makedirs(rundir, exist_ok=True)
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1

        # ---- A: the source -------------------------------------------------
        urlA = runJob(s, "water-a", makeDeck(work, "a", False), rundir)
        if not urlA:
            return 1
        rc, out = s.driver("available", urlA)
        check(out.strip().startswith("no"), "before the run there are no orbitals to use")
        calcA = finish(s, urlA, "A")
        if not calcA:
            return 1
        stored = os.path.join(calcA, "Outputs", "nwchem.movecs")
        if check(os.path.exists(stored), "nwchem.movecs was stored in Outputs/"):
            check(os.path.getsize(stored) > 500, "and it is a full file (%d bytes)"
                  % os.path.getsize(stored))
            ran = [os.path.join(d, "nwchem.movecs") for d, _, fs in os.walk(rundir)
                   if "nwchem.movecs" in fs]
            check(ran and open(ran[0], "rb").read() == open(stored, "rb").read(),
                  "byte for byte what NWChem wrote")
        rc, out = s.driver("available", urlA)
        check(out.strip() == "yes", "now Duplicate with Final Orbitals is available (%s)" % out.strip())
        base = cycles(log(calcA))
        say("  A: %s cycles, default guess" % base)

        # ---- B: the duplicate, as the menu item makes it --------------------
        rc, out = s.driver("dup3", urlA)
        if not check(rc == 0, "A duplicated with its final orbitals"):
            say(out)
            return 1
        urlB = out.strip().splitlines()[-1]
        rc, out = s.driver("hasguess", urlB)
        check(out.startswith("yes source=water-a"), "B records its source (%s)" % out.strip())
        calcB = calcDir(s, urlB)
        guess = os.path.join(calcB, "Inputs", "guess.movecsin")
        check(os.path.exists(guess) and open(guess, "rb").read() == open(stored, "rb").read(),
              "Inputs/guess.movecsin is A's nwchem.movecs, byte for byte")
        rc, out = s.driver("uncreate", urlB)
        rc, out = s.driver("hasguess", urlB)
        check(out.startswith("yes"), "the guess survives the removal of the deck")
        deckB = makeDeck(work, "b", True)
        text = open(deckB).read()
        check("vectors input guess.movecsin output nwchem.movecs" in text,
              "the generated deck reads guess.movecsin and writes nwchem.movecs")
        rc, out = s.driver("restart", urlB, deckB, "nwch.nw")
        check(rc == 0, "the regenerated deck is stored")
        #  The duplicate shares A's run directory; take A's vectors file out of it so
        #  that only the staged guess.movecsin can supply the orbitals.
        for d, _, fs in os.walk(rundir):
            if "nwchem.movecs" in fs:
                os.unlink(os.path.join(d, "nwchem.movecs"))
        calcB = finish(s, urlB, "B")
        if calcB:
            outB = log(calcB)
            n = cycles(outB)
            say("  B: %s cycles from A's vectors file (A: %s)" % (n, base))
            check(re.search(r"input vectors\s+(\S*)guess\.movecsin", outB) or
                  "guess.movecsin" in outB, "NWChem read the initial guess from guess.movecsin")
            check(n is not None and base is not None and n < base,
                  "B needed fewer SCF cycles than A")
            ea, eb = energy(log(calcA)), energy(outB)
            check(ea is not None and eb is not None and abs(ea - eb) < 1e-6,
                  "the same energy (%s, %s)" % (ea, eb))
            check(os.path.exists(os.path.join(calcB, "Outputs", "nwchem.movecs")),
                  "B kept its own nwchem.movecs")

    finally:
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)

    say("")
    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
