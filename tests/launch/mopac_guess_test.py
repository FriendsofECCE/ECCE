#!/usr/bin/env python3
"""MOPAC: retain final orbitals, duplicate and launch locally or over WebDAV (#407)."""

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


def findCode():
    return shutil.which("mopac") or ""


def makeDeck(work, name, guess):
    """Use the real input generator, as the Calculation Editor does."""
    folder = os.path.join(work, name)
    os.makedirs(folder)
    with open(os.path.join(folder, "w.param"), "w") as f:
        f.write("title: water\nCategory: SE\nTheory: RPM7\nRunType: Energy\n"
                "Charge: 0\nChemSys.Multiplicity: 1\nES.Theory.SCF.Precise: 1\n")
        if guess:
            f.write("ES.Theory.SCF.OrbitalGuessFile: guess.denin\n")
    with open(os.path.join(folder, "w.frag"), "w") as f:
        f.write("fragment\ntitle: water\nnum_atoms: 3\natom_info: symbol\n"
                "lattice_vectors:\n6 0 0\n0 6 0\n0 0 6\natom_list:\n"
                "O 0.0 0.0 0.1173\nH 0.0 0.7572 -0.4692\nH 0.0 -0.7572 -0.4692\n")
    deck = os.path.join(folder, "deck")
    shutil.copy(os.path.join(PARSERS, "mopac.tpl"), deck)
    subprocess.run(["perl", os.path.join(PARSERS, "ai.mopac"), "-n", "w", "-p",
                    "-f", "-t", deck], cwd=folder,
                   env=dict(os.environ, ECCE_HOME=REPO), check=True,
                   capture_output=True)
    # PL exposes SCF iterations; the summary counts complete SCF calls.
    text = open(deck).read().splitlines(keepends=True)
    text[0] = text[0].rstrip() + " PL\n"
    open(deck, "w").writelines(text)
    return deck


def calcDir(s, url):
    path = url.rstrip("/")
    if path.startswith("file://"):
        return path[len("file://"):]
    parts = path.split("/Ecce/users/", 1)[1]
    return os.path.join(s.fixture.stateDir(), "htdocs", "Ecce", "users", parts)


def cycles(output):
    m = re.findall(r"ITERATION\s+(\d+)\s+PLS=", output)
    return int(m[-1]) if m else None


def energy(output):
    m = re.findall(r"FINAL HEAT OF FORMATION\s*=\s*(-?\d+\.\d+)", output)
    return float(m[-1]) if m else None


def runJob(s, name, deck, rundir):
    xyz = os.path.join(os.path.dirname(deck), "water.xyz")
    with open(xyz, "w") as f:
        f.write("3\nwater\nO 0 0 0.1173\nH 0 0.7572 -0.4692\nH 0 -0.7572 -0.4692\n")
    url, out = s.create(name, "mopac_es", deck, "mopac.mop", rundir, extra=("procs=1", "code=MOPAC", "xyz=" + xyz))
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
    return open(os.path.join(calc, "Outputs", "mopac.mopout"), errors="replace").read()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--central", action="store_true")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    program = findCode()
    if not program:
        harness.skip("MOPAC is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "launchjob"), os.X_OK):
        harness.skip("launchjob is not built")

    s = harness.Session(build, "mopacguess", {"MOPAC": program},
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
        stored = os.path.join(calcA, "Outputs", "mopac.den")
        if check(os.path.exists(stored), "mopac.den was stored in Outputs/"):
            check(os.path.getsize(stored) > 100, "and it is a full file (%d bytes)"
                  % os.path.getsize(stored))
            ran = [os.path.join(d, "mopac.den") for d, _, fs in os.walk(rundir)
                   if "mopac.den" in fs]
            check(ran and open(ran[0], "rb").read() == open(stored, "rb").read(),
                  "byte for byte what MOPAC wrote")
        if not os.path.exists(stored):
            return 1
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
        rc, note = s.driver("guesscompat", urlB, "same")
        check(rc == 0 and not note.strip(), "matching atoms and basis need no warning")
        for change in ("atom", "order"):
            rc, note = s.driver("guesscompat", urlB, change)
            check(rc == 0 and "needs the same atoms in the same order" in note,
                  "%s change warns about atom compatibility" % change)
        rc, note = s.driver("guesscompat", urlB, "basis")
        check(rc == 0 and not note.strip(),
              "basis compatibility follows the code's requirements")
        calcB = calcDir(s, urlB)
        guess = os.path.join(calcB, "Inputs", "guess.denin")
        check(os.path.exists(guess) and open(guess, "rb").read() == open(stored, "rb").read(),
              "Inputs/guess.denin is A's mopac.den, byte for byte")
        rc, out = s.driver("uncreate", urlB)
        rc, out = s.driver("hasguess", urlB)
        check(out.startswith("yes"), "the guess survives the removal of the deck")
        deckB = makeDeck(work, "b", True)
        text = open(deckB).read()
        check("OLDENS" in text and "DENOUT" in text,
              "the generated deck reads the staged guess")
        rc, out = s.driver("restart", urlB, deckB, "mopac.mop")
        check(rc == 0, "the regenerated deck is stored")
        # Only the staged guess may supply the orbitals.
        for d, _, fs in os.walk(rundir):
            if "mopac.den" in fs:
                os.unlink(os.path.join(d, "mopac.den"))
        calcB = finish(s, urlB, "B")
        if calcB:
            outB = log(calcB)
            n = cycles(outB)
            say("  B: %s cycles from A's orbital file (A: %s)" % (n, base))
            check(n is not None and base is not None and n < base,
                  "B needed fewer SCF cycles than A")
            ea, eb = energy(log(calcA)), energy(outB)
            # Heats are printed to 0.00001 kcal/mol; this tolerance is
            # tighter than 2e-7 Hartree while allowing final-digit rounding.
            check(ea is not None and eb is not None and abs(ea - eb) < 1e-4,
                  "the same heat of formation in kcal/mol (%s, %s)" % (ea, eb))
            check(os.path.exists(os.path.join(calcB, "Outputs", "mopac.den")),
                  "B kept its own mopac.den")

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
