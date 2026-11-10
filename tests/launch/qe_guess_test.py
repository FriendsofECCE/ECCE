#!/usr/bin/env python3
"""QE: retain the density, duplicate and launch locally or over WebDAV (#407)."""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tarfile

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
    return shutil.which("pw.x") or ""


def makeDeck(work, name, guess):
    """Use the real input generator, as the Calculation Editor does."""
    folder = os.path.join(work, name)
    os.makedirs(folder)
    with open(os.path.join(folder, "w.param"), "w") as f:
        f.write("title: %s\nCategory: PW\nTheory: PW\nRunType: Energy\n"
                "Charge: 0\nChemSys.Multiplicity: 1\n"
                "ES.Theory.PW.KPointScheme: Gamma point only\n"
                "ES.Theory.PW.EcutWfc: 15\nES.Theory.PW.UseEcutRho: 1\n"
                "ES.Theory.PW.EcutRho: 120\n"
                "ES.Theory.PW.ConvergenceEnergy: 1.0e-9\n" % ("water_copy" if guess else "water"))
        if guess:
            f.write("ES.Theory.SCF.OrbitalGuessFile: guess.qesavein\n")
    with open(os.path.join(folder, "w.frag"), "w") as f:
        f.write("fragment\ntitle: water\nnum_atoms: 3\natom_info: symbol cart\n"
                "lattice_vectors:\n6 0 0\n0 6 0\n0 0 6\natom_list:\n"
                "O 0.0 0.0 0.1173\nH 0.0 0.7572 -0.4692\nH 0.0 -0.7572 -0.4692\n")
    deck = os.path.join(folder, "deck")
    shutil.copy(os.path.join(PARSERS, "qe.tpl"), deck)
    subprocess.run(["perl", os.path.join(PARSERS, "ai.qe"), "-n", "w", "-p",
                    "-f", "-t", deck], cwd=folder,
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
    m = re.findall(r"convergence has been achieved in\s+(\d+) iterations", output)
    return sum(map(int, m)) if m else None


def energy(output):
    m = re.findall(r"!\s+total energy\s+=\s+(\S+)\s+Ry", output)
    return float(m[-1]) if m else None


def runJob(s, name, deck, rundir):
    gui = os.path.join(os.path.dirname(deck), "settings")
    with open(gui, "w") as f:
        f.write("ES.Theory.PW.EcutWfc|15||1|1|float_input\nES.Theory.PW.UseEcutRho|1||1|1|toggle_input\nES.Theory.PW.EcutRho|120||1|1|float_input\n")
    scratch = os.path.join(s.state, "qe-scratch")
    os.makedirs(scratch, exist_ok=True)
    url, out = s.create(name, "qe_es", deck, "qe.pwin", rundir,
                        extra=("procs=1", "code=QuantumESPRESSO", "scratch=" + scratch,
                               "mvm=" + os.path.join(os.path.dirname(deck), "w.frag"),
                               "gui=" + gui))
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
    return open(os.path.join(calc, "Outputs", "qe.pwout"), errors="replace").read()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--central", action="store_true")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    program = findCode()
    if not program:
        harness.skip("QuantumESPRESSO is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "launchjob"), os.X_OK):
        harness.skip("launchjob is not built")

    s = harness.Session(build, "qeguess", {"QuantumESPRESSO": program},
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
        stored = os.path.join(calcA, "Outputs", "qe.qesave")
        if check(os.path.exists(stored), "qe.qesave was stored in Outputs/"):
            check(os.path.getsize(stored) > 100, "and it is a full file (%d bytes)"
                  % os.path.getsize(stored))
            with tarfile.open(stored) as archive:
                check(set(archive.getnames()) == {"charge-density.dat", "data-file-schema.xml"},
                      "only density and metadata are retained, without wavefunctions")
                for member in archive.getnames():
                    ran = [os.path.join(d, member) for d, _, fs in os.walk(s.state)
                           if member in fs and d.endswith("water.save")]
                    check(ran and archive.extractfile(member).read() == open(ran[0], "rb").read(),
                          "%s is byte for byte what pw.x wrote" % member)
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
        check(rc == 0 and not note.strip(), "matching atoms, cell and cutoffs need no warning")
        for change, message in (("atom", "same atoms"), ("position", "same atoms"),
                                ("cell", "same cell"), ("cutoff", "same cutoffs"),
                                ("densitycutoff", "same cutoffs")):
            rc, note = s.driver("guesscompat", urlB, change)
            check(rc == 0 and message in note,
                  "%s change warns about compatibility" % change)
        calcB = calcDir(s, urlB)
        guess = os.path.join(calcB, "Inputs", "guess.qesavein")
        check(os.path.exists(guess) and open(guess, "rb").read() == open(stored, "rb").read(),
              "Inputs/guess.qesavein is A's qe.qesave, byte for byte")
        rc, out = s.driver("uncreate", urlB)
        rc, out = s.driver("hasguess", urlB)
        check(out.startswith("yes"), "the guess survives the removal of the deck")
        deckB = makeDeck(work, "b", True)
        text = open(deckB).read()
        check("startingpot = 'file'" in text and
              "startingwfc = 'file'" not in text,
              "the generated deck reads the staged guess")
        check("prefix = 'water_copy'" in text, "B uses a different prefix from A")
        rc, out = s.driver("restart", urlB, deckB, "qe.pwin")
        check(rc == 0, "the regenerated deck is stored")
        # Only the staged archive may supply the density, under B's new prefix.
        for d, folders, _ in os.walk(s.state):
            for folder in list(folders):
                if folder.endswith(".save"):
                    shutil.rmtree(os.path.join(d, folder))
                    folders.remove(folder)
        calcB = finish(s, urlB, "B")
        if calcB:
            outB = log(calcB)
            n = cycles(outB)
            say("  B: %s cycles from A's charge density (A: %s)" % (n, base))
            check(n is not None and base is not None and n < base,
                  "B needed fewer SCF cycles than A")
            ea, eb = energy(log(calcA)), energy(outB)
            check(ea is not None and eb is not None and abs(ea - eb) < 1e-6,
                  "the same energy (%s, %s)" % (ea, eb))
            check(os.path.exists(os.path.join(calcB, "Outputs", "qe.qesave")),
                  "B kept its own qe.qesave")

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
