#!/usr/bin/env python3
"""ORCA: a finished job's orbitals as the guess of the next one (#75).

Water, HF, through the real Launch, gensub, eccejobmonitor and eccejobstore:

  1. job A (def2-SVP) runs and its orca.orcain.gbw is stored with the calculation,
     byte for byte what ORCA wrote;
  2. job B (def2-SVP) gets A's orbitals attached as "Duplicate with Final
     Orbitals" does (OrbitalGuess::attach), the deck is made by the real
     ai.orca and says MORead, the guess file survives the editor's removal
     of the old deck, is staged to the run directory, and ORCA reads it and
     converges in 3 cycles or fewer;
  3. job C (def2-TZVP) takes A's orbitals: ORCA reports a projection and
     converges; job D, the same without a guess, gives the cycles to compare.
  4. a calculation carrying A's orbitals and a def2-TZVP basis is opened in
     the Calculation Editor (its test hook, private Xvfb): the line "Initial
     guess: MOs from <calculation> (projected from <basis>)", Save keeping
     the guess and writing MORead, Use Default Guess dropping both.

    tests/launch/orca_guess_test.py [--build build-cmake] [--central] [--keep]

--central keeps the data on an Apache data server (binary files over
WebDAV); the default is a local data folder.  Exit status 77 (CTest SKIP)
without ORCA 6 or the built binaries.
"""

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import time

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


def findOrca():
    for path in sorted(glob.glob("/opt/orca/orca_6*/orca")):
        if os.access(path, os.X_OK):
            return path
    return shutil.which("orca_6") or ""


def libraryShells(basis):
    """{element: [(shell letters, [(exponent, coefficients)])]} from the
    library file a basis name stands for."""
    table = {}
    element = None
    path = os.path.join(REPO, "data", "admin", "basissets", basis.lower() + ".BAS")
    lines = open(path).read().splitlines()
    i = 0
    while i < len(lines):
        m = re.match(r"atom=(\w+)", lines[i])
        if m:
            element = m.group(1)
            table.setdefault(element, [])
        m = re.match(r"contraction shell=(\w+) num_primitives=(\d+)", lines[i])
        if m and element:
            rows = []
            for _ in range(int(m.group(2))):
                i += 1
                rows.append(lines[i].split())
            table[element].append((m.group(1), rows))
        i += 1
    return table


def writeGbs(path, elements, basis):
    """The .gbs as TGBSConfig::dump() writes it: names, then numbers."""
    table = libraryShells(basis)
    with open(path, "w") as out:
        out.write('NameBasis\nbasis "ao basis" spherical print\n')
        for el in elements:
            out.write('%s library "%s"\n' % (el, basis))
        out.write("END\nEndNameBasis\n\nNumericalBasis\n"
                  'basis "ao basis" spherical print\n')
        for el in elements:
            for shell, rows in table[el]:
                out.write("%s %s\n" % (el, shell))
                for row in rows:
                    out.write("  " + "  ".join(row) + "\n")
        out.write("END\nEndNumericalBasis\n")


def makeDeck(work, name, basis, guess):
    """The deck as the Calculation Editor makes it, from the real ai.orca."""
    folder = os.path.join(work, name)
    os.makedirs(folder)
    with open(os.path.join(folder, "w.param"), "w") as f:
        f.write("Category: SCF\nTheory: RHF\nRunType: Energy\nCharge: 0\n"
                "Symmetry: C1\nNumElectrons: 10\nChemSys.Multiplicity: 1\n"
                "ES.Theory.SCF.ConvergenceTolerance: Tight\n")
        if guess:
            f.write("ES.Theory.SCF.OrbitalGuessFile: guess.gbwin\n")
    with open(os.path.join(folder, "w.frag"), "w") as f:
        f.write("fragment\ntitle: w\nnum_atoms: 3\natom_info: symbol\n"
                "atom_list:\n"
                "O 0 0 0.1173\nH 0 0.7572 -0.4692\nH 0 -0.7572 -0.4692\n")
    writeGbs(os.path.join(folder, "w.gbs"), ["H", "O"], basis)
    with open(os.path.join(folder, "w.basis"), "w") as f:
        f.write("useRouteCard %s false\n" % basis)
    deck = os.path.join(folder, "deck")
    shutil.copy(os.path.join(PARSERS, "orca.tpl"), deck)
    subprocess.run(["perl", os.path.join(PARSERS, "ai.orca"), "-n", "w", "-p",
                    "-f", "-b", "-t", deck], cwd=folder,
                   env=dict(os.environ, ECCE_HOME=REPO), check=True,
                   capture_output=True)
    return deck


def importFiles(orca, work, name):
    """What Import Calculation does: ORCA once on the deck, its output read
    by ORCA.expt, which writes the molecule, basis and theory files."""
    pre = os.path.join(work, "pre-" + name)
    os.makedirs(pre)
    shutil.copy(os.path.join(work, name, "deck"), os.path.join(pre, "orca.orcain"))
    with open(os.path.join(pre, "orca.orcaout"), "w") as handle:
        subprocess.run([orca, "orca.orcain"], cwd=pre, stdout=handle,
                       stderr=subprocess.STDOUT, check=True)
    subprocess.run(["perl", os.path.join(PARSERS, "ORCA.expt"), "orca.orcaout"],
                   cwd=pre, env=dict(os.environ, ECCE_HOME=REPO), check=True,
                   capture_output=True)
    return pre


def editorPhase(s, build, orca, work, rundir, urlA, andy=False):
    """The Calculation Editor on a calculation that carries A's orbitals,
    through its test hook on a private X server: the line under Settings,
    Save keeping the guess, and Use Default Guess dropping it."""
    for tool in ("Xvfb",):
        if not shutil.which(tool):
            say("  SKIP the editor phase: %s is not installed" % tool)
            return
    if not os.access(os.path.join(build, "calced"), os.X_OK):
        say("  SKIP the editor phase: calced is not built")
        return
    import xdisplay
    harness.link(os.path.join(build, "calced"), os.path.join(s.home, "bin", "calced"))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)

    tag = "f" if andy else "e"
    urlE = runJob(s, "water-tzvp-" + ("flow" if andy else "editor"),
                  makeDeck(work, tag, "def2-TZVP", False), rundir)
    rc, out = s.driver("setup", urlE, importFiles(orca, work, tag), "orca.orcaout")
    check(rc == 0, "E's molecule, basis and theory stored (setup)")
    if andy:
        #  The real Duplicate with Final Orbitals: a copy of the finished
        #  source, reset, the orbitals attached.
        rc, out = s.driver("dup3", urlA)
        check(rc == 0, "A duplicated with its final orbitals")
        urlE = out.strip().splitlines()[-1]
    else:
        rc, out = s.driver("guess", urlE, urlA)
        check(rc == 0, "A's orbitals attached to E")
    calcE = calcDir(s, urlE)

    cmdfile = os.path.join(s.state, "calced.cmd")
    open(cmdfile, "w").close()
    logpath = os.path.join(s.state, "calced.log")
    with xdisplay.Display() as display, open(logpath, "w") as log:
        env = s.env({"DISPLAY": display.name, "ECCE_TEST_CALCED": cmdfile,
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        proc = subprocess.Popen([os.path.join(wrappers, "ecce-calced"), "-context", urlE],
                                env=env, cwd=os.path.join(s.home, "bin"),
                                stdout=log, stderr=subprocess.STDOUT,
                                start_new_session=True)
        asked = {}

        def ask(cmd, seconds=120):
            with open(cmdfile, "a") as handle:
                handle.write(cmd + "\n")
            asked[cmd] = asked.get(cmd, 0) + 1
            want = "ECCE_TEST_CALCED: %s: " % cmd
            end = time.time() + seconds
            while time.time() < end and proc.poll() is None:
                seen = 0
                for line in open(logpath, errors="replace"):
                    if line.startswith(want):
                        seen += 1
                        if seen == asked[cmd]:
                            return line[len(want):].strip()
                time.sleep(0.5)
            return None

        def deck():
            path = os.path.join(calcE, "Inputs", "orca.orcain")
            return open(path, errors="replace").read() if os.path.exists(path) else ""

        try:
            time.sleep(10)
            if andy:
                andyFlow(s, ask, deck, calcE, urlE)
                ask("quit", 30)
                return
            answer = ask("guess")
            say("    guess -> %s" % answer)
            check(answer is not None and "shown=1" in answer and
                  "Initial guess: MOs from water-svp-editor" not in answer and
                  "Initial guess: MOs from water-svp (projected from def2-svp)" in answer,
                  "the editor shows the guess and that it is projected from the old basis")
            answer = ask("save")
            say("    save -> %s" % answer)
            check("MORead" in deck() and '%moinp "guess.gbwin"' in deck(),
                  "Save wrote MORead into the deck")
            check(os.path.exists(os.path.join(calcE, "Inputs", "guess.gbwin")),
                  "and Save kept guess.gbwin among the inputs")
            ask("guessdefault")
            answer = ask("guess")
            say("    guess -> %s" % answer)
            check(answer is not None and "shown=0" in answer and "has=0" in answer,
                  "Use Default Guess hides the line and drops the guess")
            ask("save")
            check("MORead" not in deck() and "moinp" not in deck() and deck() != "",
                  "the next Save writes the deck without MORead")
            check(not os.path.exists(os.path.join(calcE, "Inputs", "guess.gbwin")),
                  "and the guess file is gone")
            ask("quit", 30)
        finally:
            if proc.poll() is None:
                try:
                    os.killpg(proc.pid, 15)
                    proc.wait(timeout=20)
                except (OSError, subprocess.TimeoutExpired):
                    pass


def andyFlow(s, ask, deck, calcE, urlE):
    """Duplicate with Final Orbitals, then in the editor: change the run
    type to GeoVib, Save, launch; the deck and ORCA's output must show the
    guess."""
    answer = ask("guess")
    check(answer is not None and "has=1" in answer and "shown=1" in answer,
          "the editor shows the guess (%s)" % answer)
    answer = ask("runtype GeoVib")
    check(answer == "ok", "the run type was changed to GeoVib (%s)" % answer)
    answer = ask("guess")
    check(answer is not None and "has=1" in answer,
          "the guess is still attached after the run type change")
    ask("save")
    check("MORead" in deck() and '%moinp "guess.gbwin"' in deck(),
          "Save after the run type change wrote MORead and %moinp")
    check("Opt" in deck() and "Freq" in deck(), "and the GeoVib route")
    check(os.path.exists(os.path.join(calcE, "Inputs", "guess.gbwin")),
          "and kept guess.gbwin")
    ran = finish(s, urlE, "the duplicate")
    if ran:
        out = open(os.path.join(ran, "Outputs", "orca.orcaout"),
                   errors="replace").read()
        check("Guess MOs are being read from file: guess.gbwin" in out,
              "ORCA read guess.gbwin in the launched job")


def calcDir(s, url):
    path = url.rstrip("/")
    if path.startswith("file://"):
        return path[len("file://"):]
    parts = path.split("/Ecce/users/", 1)[1]
    return os.path.join(s.fixture.stateDir(), "htdocs", "Ecce", "users", parts)


def cycles(output):
    m = re.search(r"SCF CONVERGED AFTER\s+(\d+)\s+CYCLES", output)
    return int(m.group(1)) if m else None


def runJob(s, name, deck, rundir):
    url, out = s.create(name, "orca_es", deck, "orca.orcain", rundir)
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


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--central", action="store_true")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--no-editor", action="store_true",
                    help="skip the Calculation Editor phase (needs Xvfb)")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    orca = findOrca()
    if not orca:
        harness.skip("ORCA 6 is not installed under /opt/orca")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "launchjob"), os.X_OK):
        harness.skip("launchjob is not built")

    s = harness.Session(build, "orcaguess", {"ORCA": orca},
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
        urlA = runJob(s, "water-svp", makeDeck(work, "a", "def2-SVP", False), rundir)
        if not urlA:
            return 1
        rc, out = s.driver("setup", urlA, importFiles(orca, work, "a"), "orca.orcaout")
        if not check(rc == 0, "A's molecule, basis and theory stored (setup)"):
            say(out)
        rc, out = s.driver("available", urlA)
        check(out.strip().startswith("no"), "before the run there are no orbitals to use")
        calcA = finish(s, urlA, "A")
        if not calcA:
            return 1
        outA = open(os.path.join(calcA, "Outputs", "orca.orcaout"), errors="replace").read()
        stored = os.path.join(calcA, "Outputs", "orca.orcain.gbw")
        if check(os.path.exists(stored), "orca.orcain.gbw was stored in Outputs/"):
            ran = [p for p in glob.glob(os.path.join(rundir, "**", "orca.orcain.gbw"), recursive=True)
                   if os.path.getsize(p) == os.path.getsize(stored)]
            check(os.path.getsize(stored) > 50000 and ran and
                  open(stored, "rb").read() == open(ran[0], "rb").read(),
                  "the stored file is the file ORCA wrote (%d bytes)" % os.path.getsize(stored))
        rc, out = s.driver("available", urlA)
        check(out.strip() == "yes", "now Duplicate with Final Orbitals is available (%s)" % out.strip())
        base = cycles(outA)
        say("  A: %s cycles, default guess" % base)

        # ---- B: same basis, orbitals of A ----------------------------------
        urlB = runJob(s, "water-svp-guess", makeDeck(work, "b", "def2-SVP", True), rundir)
        deckB = open(os.path.join(work, "b", "deck")).read()
        check("MORead" in deckB and '%moinp "guess.gbwin"' in deckB,
              "the generated deck says MORead and names the guess file")
        rc, out = s.driver("guess", urlB, urlA)
        check(rc == 0, "A's orbitals attached to B")
        rc, out = s.driver("hasguess", urlB)
        check("yes source=water-svp basis=def2-svp" in out.lower(),
              "B records its source and its basis (%s)" % out.strip())
        calcB = calcDir(s, urlB)
        guess = os.path.join(calcB, "Inputs", "guess.gbwin")
        check(os.path.exists(guess) and open(guess, "rb").read() == open(stored, "rb").read(),
              "Inputs/guess.gbwin is A's orca.orcain.gbw, byte for byte")
        rc, out = s.driver("uncreate", urlB)
        rc, out = s.driver("hasguess", urlB)
        check(out.startswith("yes"), "the guess survives the removal of the deck")
        check(not os.path.exists(os.path.join(calcB, "Inputs", "orca.orcain")),
              "and the old deck is gone")
        rc, out = s.driver("restart", urlB, os.path.join(work, "b", "deck"), "orca.orcain")
        check(rc == 0, "the regenerated deck is stored")
        calcB = finish(s, urlB, "B")
        if calcB:
            outB = open(os.path.join(calcB, "Outputs", "orca.orcaout"), errors="replace").read()
            check("Guess MOs are being read from file: guess.gbwin" in outB,
                  "ORCA read guess.gbwin in the run directory")
            n = cycles(outB)
            say("  B: %s cycles from A's orbitals (A: %s)" % (n, base))
            check(n is not None and n <= 3, "B converged in 3 cycles or fewer")
            check(os.path.exists(os.path.join(calcB, "Outputs", "orca.orcain.gbw")),
                  "B kept its own orca.orcain.gbw, not the guess")
            check(os.path.getsize(os.path.join(calcB, "Outputs", "orca.orcain.gbw")) > 50000,
                  "and it is a full file")

        # ---- C and D: a larger basis, with and without the guess ----------
        urlC = runJob(s, "water-tzvp-guess", makeDeck(work, "c", "def2-TZVP", True), rundir)
        rc, out = s.driver("guess", urlC, urlA)
        check(rc == 0, "A's orbitals attached to C")
        rc, out = s.driver("restart", urlC, os.path.join(work, "c", "deck"), "orca.orcain")
        calcC = finish(s, urlC, "C")
        urlD = runJob(s, "water-tzvp", makeDeck(work, "d", "def2-TZVP", False), rundir)
        calcD = finish(s, urlD, "D")
        if calcC and calcD:
            outC = open(os.path.join(calcC, "Outputs", "orca.orcaout"), errors="replace").read()
            outD = open(os.path.join(calcD, "Outputs", "orca.orcaout"), errors="replace").read()
            check("Input basis set needs projection" in outC,
                  "ORCA projected the def2-SVP orbitals onto def2-TZVP")
            nc, nd = cycles(outC), cycles(outD)
            say("  C: %s cycles with projected orbitals; D: %s cycles with the default guess" % (nc, nd))
            check(nc is not None, "C converged")
            ec = re.findall(r"FINAL SINGLE POINT ENERGY\s+(-?\d+\.\d+)", outC)
            ed = re.findall(r"FINAL SINGLE POINT ENERGY\s+(-?\d+\.\d+)", outD)
            check(ec and ed and abs(float(ec[-1]) - float(ed[-1])) < 1e-6,
                  "the same energy either way (%s, %s)" % (ec[-1:], ed[-1:]))
        #  The editor needs a login against a data server; local data covers it.
        if not args.no_editor and not args.central:
            editorPhase(s, build, orca, work, rundir, urlA)
            editorPhase(s, build, orca, work, rundir, urlA, andy=True)
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
