#!/usr/bin/env python3
"""ECCE-QM: a finished job's final orbitals as the guess of the next one (#407).

Water RHF/6-31G and O2 triplet ROHF/6-31G through the real Launch, gensub,
eccejobmonitor and eccejobstore:

  1. job A runs and its ecceqm.qmorb is stored with the calculation, byte for
     byte what ecce-qm wrote; before the run "Duplicate with Final Orbitals" is
     not available, after it is;
  2. job B is made as that menu item makes it (copy, reset, orbitals attached
     as guess.qmorbin); the deck made by the real ai.ecceqm says
     "read_orbitals guess.qmorbin", the file survives the removal of the old
     deck and is staged to the run directory, and ecce-qm needs fewer SCF
     iterations than A for the same energy.

    tests/launch/ecceqm_guess_test.py [--build build-cmake] [--central] [--keep]

Exit status 77 (CTest SKIP) without the built binaries.
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
sys.path.insert(0, os.path.join(REPO, "tests", "qm"))

import harness  # noqa: E402
from harness import say  # noqa: E402
from molecules import MOLECULES  # noqa: E402
import ecceqm_test as qt  # noqa: E402  (basis writer of the ECCE-QM test)

PARSERS = os.path.join(REPO, "scripts", "parsers")
BASIS = "6-31G"
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def makeDeck(work, name, mol, mult, guess):
    """The deck as the Calculation Editor makes it, from the real ai.ecceqm."""
    folder = os.path.join(work, name)
    os.makedirs(folder)
    charge, _m, atoms = MOLECULES[mol]
    with open(os.path.join(folder, "w.param"), "w") as f:
        f.write("Category: SCF\nTheory: HF\nRunType: Energy\nCharge: %d\n"
                "ChemSys.Multiplicity: %d\n" % (charge, mult))
        if guess:
            f.write("ES.Theory.SCF.OrbitalGuessFile: guess.qmorbin\n")
    with open(os.path.join(folder, "w.frag"), "w") as f:
        f.write("fragment\ntitle: w\nnum_atoms: %d\natom_info: symbol\natom_list:\n"
                % len(atoms))
        for a in atoms:
            f.write("%s %.10f %.10f %.10f\n" % a)
    qt.writeGbs(os.path.join(folder, "w.gbs"), sorted({a[0] for a in atoms}), BASIS)
    env = dict(os.environ, ECCE_HOME=REPO)
    with open(os.path.join(folder, "w.gbs")) as gbs:
        out = subprocess.run(["perl", os.path.join(PARSERS, "std2ECCEQM")], stdin=gbs,
                             env=env, capture_output=True, check=True).stdout
    open(os.path.join(folder, "w.basis"), "wb").write(out)
    deck = os.path.join(folder, "deck")
    shutil.copy(os.path.join(PARSERS, "ecceqm.tpl"), deck)
    subprocess.run(["perl", os.path.join(PARSERS, "ai.ecceqm"), "-n", "w", "-p", "-f",
                    "-b", "-t", deck], cwd=folder, env=env, check=True,
                   capture_output=True)
    return deck


def calcDir(s, url):
    path = url.rstrip("/")
    if path.startswith("file://"):
        return path[len("file://"):]
    parts = path.split("/Ecce/users/", 1)[1]
    return os.path.join(s.fixture.stateDir(), "htdocs", "Ecce", "users", parts)


def field(output, key):
    m = re.search(r"^%s\s+(\S+)" % key, output, re.M)
    return m.group(1) if m else None


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
    return open(os.path.join(calc, "Outputs", "ecceqm.qmout"), errors="replace").read()


def runCase(s, work, base, tag, mol, mult):
    say("--- %s" % tag)
    rundir = os.path.join(base, tag)
    os.makedirs(rundir, exist_ok=True)
    urlA, out = s.create(tag + "-a", "ecceqm_es", makeDeck(work, tag + "a", mol, mult, False),
                         "ecceqm.qmin", rundir)
    if not check(urlA is not None, "%s-a created" % tag):
        say(out)
        return
    rc, out = s.driver("available", urlA)
    check(out.strip().startswith("no"), "before the run there are no orbitals to use")
    calcA = finish(s, urlA, "A")
    if not calcA:
        return
    stored = os.path.join(calcA, "Outputs", "ecceqm.qmorb")
    if check(os.path.exists(stored), "ecceqm.qmorb was stored in Outputs/"):
        check(os.path.getsize(stored) > 500, "and it is a full file (%d bytes)"
              % os.path.getsize(stored))
        ran = [os.path.join(d, "ecceqm.qmorb") for d, _, fs in os.walk(rundir)
               if "ecceqm.qmorb" in fs]
        check(ran and open(ran[0], "rb").read() == open(stored, "rb").read(),
              "byte for byte what ecce-qm wrote")
    rc, out = s.driver("available", urlA)
    check(out.strip() == "yes", "now Duplicate with Final Orbitals is available (%s)" % out.strip())
    outA = log(calcA)
    base = int(field(outA, "iterations") or 0)
    say("  A: %s iterations, atomic-density guess" % base)

    rc, out = s.driver("dup3", urlA)
    if not check(rc == 0, "A duplicated with its final orbitals"):
        say(out)
        return
    urlB = out.strip().splitlines()[-1]
    rc, out = s.driver("hasguess", urlB)
    check(out.startswith("yes source=%s-a" % tag), "B records its source (%s)" % out.strip())
    guess = os.path.join(calcDir(s, urlB), "Inputs", "guess.qmorbin")
    check(os.path.exists(guess) and open(guess, "rb").read() == open(stored, "rb").read(),
          "Inputs/guess.qmorbin is A's ecceqm.qmorb, byte for byte")
    s.driver("uncreate", urlB)
    rc, out = s.driver("hasguess", urlB)
    check(out.startswith("yes"), "the guess survives the removal of the deck")
    deckB = makeDeck(work, tag + "b", mol, mult, True)
    check("read_orbitals guess.qmorbin\n" in open(deckB).read(),
          "the generated deck reads guess.qmorbin")
    rc, out = s.driver("restart", urlB, deckB, "ecceqm.qmin")
    check(rc == 0, "the regenerated deck is stored")
    #  Only the staged guess may supply the orbitals.
    for d, _, fs in os.walk(rundir):
        if "ecceqm.qmorb" in fs:
            os.unlink(os.path.join(d, "ecceqm.qmorb"))
    calcB = finish(s, urlB, "B")
    if calcB:
        outB = log(calcB)
        n = int(field(outB, "iterations") or 0)
        say("  B: %s iterations from A's orbitals (A: %s)" % (n, base))
        check("not used" not in outB, "ecce-qm accepted the guess")
        check(0 < n < base, "B needed fewer SCF iterations than A")
        ea, eb = float(field(outA, "energy_total")), float(field(outB, "energy_total"))
        check(abs(ea - eb) < 1e-7, "the same energy (%.9f, %.9f)" % (ea, eb))
        check(os.path.exists(os.path.join(calcB, "Outputs", "ecceqm.qmorb")),
              "B kept its own ecceqm.qmorb")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--central", action="store_true")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    qm = os.path.join(build, "src", "qm", "ecce-qm")
    harness.prerequisites(build, ())
    if not os.access(qm, os.X_OK):
        harness.skip("ecce-qm is not built (cmake -DECCE_BUILD_QM=ON)")

    s = harness.Session(build, "qmguess", {"ecce-qm": qm, "ECCE-QM": qm},
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
        runCase(s, work, rundir, "water", "h2o", 1)
        runCase(s, work, rundir, "o2", "o2", 3)
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
