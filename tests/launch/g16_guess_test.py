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
     duplicate;
  4. without the checkpoint (#690): water RHF/6-31G* with its g16.chk deleted,
     and triplet O2 UHF/6-31G* run with Preferences > Data "Discard the
     programs' own binary files" on, so the job store drops g16.chk after
     results.h5 passed its checks.  The duplicate gets guess.cardsin, the
     orbitals from results.h5 in Gaussian's order, read with Guess=Cards;
     Gaussian reports the replacement orbitals and converges in a few
     cycles to the same energy.

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


WATER = ("RHF", 10, 1, ["O 0.0 0.0 0.1173", "H 0.0 0.7572 -0.4692", "H 0.0 -0.7572 -0.4692"])
O2 = ("UHF", 16, 3, ["O 0.0 0.0 0.6037", "O 0.0 0.0 -0.6037"])


def makeDeck(work, name, guess, checkpoint=True, mol=WATER, basis="STO-3G"):
    """The deck as the Calculation Editor makes it, from the real ai.gauss16.
    `guess` is the attached guess file name (True: guess.chkin)."""
    if guess is True:
        guess = "guess.chkin"
    theory, nelec, mult, atoms = mol
    folder = os.path.join(work, name)
    os.makedirs(folder)
    with open(os.path.join(folder, "w.param"), "w") as f:
        f.write("title: w\nannotation: w\nCategory: SCF\nTheory: %s\nRunType: Energy\nCharge: 0\n"
                "Symmetry: C1\nNumElectrons: %d\nChemSys.Multiplicity: %d\n"
                "CreateCheckpoint: %d\nMemorySize: 1\n"
                "SCF.InitialGuess: Gaussian Guess\n" % (theory, nelec, mult, 1 if checkpoint else 0))
        if guess:
            f.write("ES.Theory.SCF.OrbitalGuessFile: %s\n" % guess)
    with open(os.path.join(folder, "w.frag"), "w") as f:
        f.write("fragment\ntitle: w\nnum_atoms: %d\natom_info: symbol\n"
                "atom_list:\n%s\n" % (len(atoms), "\n".join(atoms)))
    with open(os.path.join(folder, "w.basis"), "w") as f:
        f.write("useRouteCard %s false\n" % basis)
    deck = os.path.join(folder, "deck")
    shutil.copy(os.path.join(PARSERS, "g16.tpl"), deck)
    subprocess.run(["perl", os.path.join(PARSERS, "ai.gauss16"), "-n", "w", "-p",
                    "-f", "-b", "-t", deck], cwd=folder,
                   env=dict(os.environ, ECCE_HOME=REPO), check=True,
                   capture_output=True)
    return deck


def importFiles(g16, work, name):
    """What Import Calculation does: Gaussian once on the deck, its output
    read by Gaussian-16.expt, which writes the molecule, basis and theory."""
    pre = os.path.join(work, "pre-" + name)
    os.makedirs(pre)
    shutil.copy(os.path.join(work, name, "deck"), os.path.join(pre, "g16.g16in"))
    with open(os.path.join(pre, "g16.g16in")) as inp, \
            open(os.path.join(pre, "g16.g16out"), "w") as out:
        subprocess.run([g16], cwd=pre, stdin=inp, stdout=out,
                       stderr=subprocess.STDOUT, check=True,
                       env=dict(os.environ, GAUSS_SCRDIR=pre))
    subprocess.run(["perl", os.path.join(PARSERS, "Gaussian-16.expt"), "g16.g16out"],
                   cwd=pre, env=dict(os.environ, ECCE_HOME=REPO), check=True,
                   capture_output=True)
    return pre


def stored(calc, name):
    return [os.path.join(d, name) for d, _, fs in os.walk(calc) if name in fs]


def fromResults(s, g16, work, rundir, tag, mol, discard):
    """Duplicate with Final Orbitals when the source kept no checkpoint (#690)."""
    say("-- %s: orbitals from results.h5 (%s)" % (tag, "switch on" if discard else "g16.chk deleted"))
    prefs = os.path.join(s.state, ".ECCE", "EcceGlobal")
    lines = [l for l in (open(prefs).read().splitlines() if os.path.exists(prefs) else [])
             if not l.startswith("DiscardBinaryOutputs")]
    if discard:
        lines.append("DiscardBinaryOutputs:\ttrue")
    with open(prefs, "w") as f:
        f.write("\n".join(lines) + "\n")
    urlA = runJob(s, tag + "-a", makeDeck(work, tag + "a", False, mol=mol, basis="6-31G*"), rundir)
    if not urlA:
        return
    rc, out = s.driver("setup", urlA, importFiles(g16, work, tag + "a"), "g16.g16out")
    if not check(rc == 0, "%s: molecule, basis and theory stored (setup)" % tag):
        say(out)
    calcA = finish(s, urlA, tag + " A")
    if not calcA:
        return
    check(stored(calcA, "results.h5"), "%s: results.h5 was stored" % tag)
    if discard:
        check(not stored(calcA, "g16.chk"), "%s: the job store discarded g16.chk" % tag)
    else:
        for f in stored(calcA, "g16.chk"):
            os.unlink(f)
    rc, out = s.driver("available", urlA)
    check(out.strip() == "yes", "%s: without the checkpoint Duplicate with Final Orbitals "
          "is available (%s)" % (tag, out.strip()))
    base, ea = cycles(log(calcA)), energy(log(calcA))
    rc, out = s.driver("dup3", urlA)
    if not check(rc == 0, "%s: duplicated with its final orbitals" % tag):
        say(out)
        return
    urlB = out.strip().splitlines()[-1]
    calcB = calcDir(s, urlB)
    check(os.path.exists(os.path.join(calcB, "Inputs", "guess.cardsin")) and
          not os.path.exists(os.path.join(calcB, "Inputs", "guess.chkin")),
          "%s: B carries guess.cardsin and no guess.chkin" % tag)
    s.driver("uncreate", urlB)
    rc, out = s.driver("hasguess", urlB)
    check(out.startswith("yes"), "%s: the guess survives the removal of the deck" % tag)
    deckB = makeDeck(work, tag + "b", "guess.cardsin", mol=mol, basis="6-31G*")
    text = open(deckB).read()
    check("Guess=Cards" in text and "\n\n@guess.cardsin/N\n" in text and
          "%OldChk" not in text and "\n\n\n@" not in text,
          "%s: the deck reads guess.cardsin with Guess=Cards" % tag)
    s.driver("restart", urlB, deckB, "g16.g16in")
    for f in stored(rundir, "g16.chk"):
        os.unlink(f)
    calcB = finish(s, urlB, tag + " B")
    if not calcB:
        return
    outB = log(calcB)
    n, eb = cycles(outB), energy(outB)
    say("  %s: A %s cycles, B %s cycles from results.h5; E %s / %s" % (tag, base, n, ea, eb))
    check("Replacement alpha orbitals read in format (4D20.12)" in outB,
          "%s: Gaussian read the alpha orbitals from the cards" % tag)
    if mol[0] == "UHF":
        check("Replacement beta orbitals read in format (4D20.12)" in outB,
              "%s: and the beta orbitals" % tag)
    check(n is not None and base is not None and n <= 3 and n < base,
          "%s: B converged in at most 3 cycles (A: %s, B: %s)" % (tag, base, n))
    check(ea is not None and eb is not None and abs(ea - eb) < 1e-6,
          "%s: the same energy (%s, %s)" % (tag, ea, eb))


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

        # ---- D: no checkpoint, the orbitals from results.h5 ----------------
        fromResults(s, g16, work, rundir, "water", WATER, False)
        fromResults(s, g16, work, rundir, "o2", O2, True)
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
