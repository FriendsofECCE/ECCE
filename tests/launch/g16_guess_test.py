#!/usr/bin/env python3
"""Gaussian 16: a finished job's checkpoint as the guess of the next one (#407).

Water, HF/STO-3G, through the real Launch, gensub, eccejobmonitor and
eccejobstore:

  1. job A runs and its g16.chk is stored with the calculation, byte for byte
     what Gaussian wrote; before the run "Duplicate with Final Orbitals" is
     not available, after it is;
  2. job B is made as that menu item makes it (copy, reset, checkpoint
     attached as guess.chkin); the deck made by the real ai.gauss16 says
     %OldChk=guess.chkin and Guess=Read, the file survives the removal of
     the old deck and is staged to the run directory, and Gaussian reads it
     and needs fewer SCF cycles than A;
  3. a job with "Create Checkpoint File" off keeps no g16.chk and offers no
     duplicate.

    tests/launch/g16_guess_test.py [--build build-cmake] [--central] [--keep]

Exit status 77 (CTest SKIP) without Gaussian 16 or the built binaries.
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


def findG16():
    for base in (os.environ.get("GAUSS_EXEDIR"), "/opt/gaussian/g16"):
        if base and os.access(os.path.join(base, "g16"), os.X_OK):
            return os.path.join(base, "g16")
    return ""


def makeDeck(work, name, guess, checkpoint=True):
    """The deck as the Calculation Editor makes it, from the real ai.gauss16."""
    folder = os.path.join(work, name)
    os.makedirs(folder)
    with open(os.path.join(folder, "w.param"), "w") as f:
        f.write("title: w\nannotation: water\nCategory: SCF\nTheory: RHF\nRunType: Energy\nCharge: 0\n"
                "Symmetry: C1\nNumElectrons: 10\nChemSys.Multiplicity: 1\n"
                "CreateCheckpoint: %d\nMemorySize: 1\n"
                "SCF.InitialGuess: Gaussian Guess\n" % (1 if checkpoint else 0))
        if guess:
            f.write("ES.Theory.SCF.OrbitalGuessFile: guess.chkin\n")
    with open(os.path.join(folder, "w.frag"), "w") as f:
        f.write("fragment\ntitle: w\nnum_atoms: 3\natom_info: symbol\n"
                "atom_list:\n"
                "O 0.0 0.0 0.1173\nH 0.0 0.7572 -0.4692\nH 0.0 -0.7572 -0.4692\n")
    with open(os.path.join(folder, "w.basis"), "w") as f:
        f.write("useRouteCard STO-3G false\n")
    deck = os.path.join(folder, "deck")
    shutil.copy(os.path.join(PARSERS, "g16.tpl"), deck)
    subprocess.run(["perl", os.path.join(PARSERS, "ai.gauss16"), "-n", "w", "-p",
                    "-f", "-b", "-t", deck], cwd=folder,
                   env=dict(os.environ, ECCE_HOME=REPO), check=True,
                   capture_output=True)
    return deck


def calcDir(s, url):
    path = url.rstrip("/")
    if path.startswith("file://"):
        return path[len("file://"):]
    parts = path.split("/Ecce/users/", 1)[1]
    return os.path.join(s.fixture.stateDir(), "htdocs", "Ecce", "users", parts)


def cycles(output):
    m = re.findall(r"SCF Done:.*? after\s+(\d+)\s+cycles", output)
    return int(m[-1]) if m else None


def energy(output):
    m = re.findall(r"SCF Done:\s+E\(\S+\)\s+=\s+(-?\d+\.\d+)", output)
    return float(m[-1]) if m else None


def runJob(s, name, deck, rundir):
    url, out = s.create(name, "gaussian16_es", deck, "g16.g16in", rundir)
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
    return open(os.path.join(calc, "Outputs", "g16.g16out"), errors="replace").read()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--central", action="store_true")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    g16 = findG16()
    if not g16:
        harness.skip("Gaussian 16 is not installed under /opt/gaussian/g16")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "launchjob"), os.X_OK):
        harness.skip("launchjob is not built")

    s = harness.Session(build, "g16guess", {"Gaussian-16": g16},
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
        stored = os.path.join(calcA, "Outputs", "g16.chk")
        if check(os.path.exists(stored), "g16.chk was stored in Outputs/"):
            check(os.path.getsize(stored) > 10000, "and it is a full file (%d bytes)"
                  % os.path.getsize(stored))
            ran = [os.path.join(d, "g16.chk") for d, _, fs in os.walk(rundir)
                   if "g16.chk" in fs]
            check(ran and open(ran[0], "rb").read() == open(stored, "rb").read(),
                  "byte for byte what Gaussian wrote")
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
        guess = os.path.join(calcB, "Inputs", "guess.chkin")
        check(os.path.exists(guess) and open(guess, "rb").read() == open(stored, "rb").read(),
              "Inputs/guess.chkin is A's g16.chk, byte for byte")
        rc, out = s.driver("uncreate", urlB)
        rc, out = s.driver("hasguess", urlB)
        check(out.startswith("yes"), "the guess survives the removal of the deck")
        deckB = makeDeck(work, "b", True)
        text = open(deckB).read()
        check("%OldChk=guess.chkin" in text and "Guess=Read" in text and
              "%Chk=g16.chk" in text, "the generated deck reads guess.chkin and writes g16.chk")
        rc, out = s.driver("restart", urlB, deckB, "g16.g16in")
        check(rc == 0, "the regenerated deck is stored")
        #  The duplicate shares A's run directory; take A's checkpoint out of it so
        #  that only the staged guess.chkin can supply the orbitals.
        for d, _, fs in os.walk(rundir):
            if "g16.chk" in fs:
                os.unlink(os.path.join(d, "g16.chk"))
        calcB = finish(s, urlB, "B")
        if calcB:
            outB = log(calcB)
            n = cycles(outB)
            say("  B: %s cycles from A's checkpoint (A: %s)" % (n, base))
            check(re.search(r'Initial guess from the checkpoint file:\s+"g16.chk"', outB),
                  "Gaussian read the initial guess from the checkpoint")
            check(n is not None and base is not None and n < base,
                  "B needed fewer SCF cycles than A")
            ea, eb = energy(log(calcA)), energy(outB)
            check(ea is not None and eb is not None and abs(ea - eb) < 1e-6,
                  "the same energy (%s, %s)" % (ea, eb))
            check(os.path.exists(os.path.join(calcB, "Outputs", "g16.chk")),
                  "B kept its own g16.chk")

        # ---- C: checkpoint off, nothing to duplicate -----------------------
        urlC = runJob(s, "water-c", makeDeck(work, "c", False, checkpoint=False), rundir)
        calcC = finish(s, urlC, "C")
        if calcC:
            check(not os.path.exists(os.path.join(calcC, "Outputs", "g16.chk")),
                  "without a checkpoint there is no g16.chk")
            rc, out = s.driver("available", urlC)
            check(out.strip().startswith("no"), "and no Duplicate with Final Orbitals")
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
