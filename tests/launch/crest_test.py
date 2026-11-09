#!/usr/bin/env python3
"""CREST conformer search through the real Launch (#682).

Local data, no data server.  The deck is made by the real ai.crest, stored
with a new calculation and run on `localhost` through Launch, gensub (sub
crest, with the xtb path of the same machine), eccejobmaster, eccejobstore
and eccejobmonitor.  n-butane with GFN-FF must reach "completed" with the
ensemble stored: GEOMTRACE with one frame per conformer (at least two) and
CONFREL, the energies relative to the lowest in kcal/mol, starting at 0 and
ascending.  The option line is also checked for every option.

    tests/launch/crest_test.py [--build build-cmake] [--keep]

Exit status 77 (CTest SKIP) without crest, xtb or the built binaries.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import harness  # noqa: E402
from harness import say  # noqa: E402

failures = []

METHYLAMINE = [("C", -0.725, 0.0, 0.0), ("N", 0.725, 0.0, 0.0),
               ("H", 1.1, 0.82, 0.4), ("H", 1.1, -0.82, 0.4),
               ("H", -1.1, 0.0, -1.02), ("H", -1.1, 0.89, 0.5),
               ("H", -1.1, -0.89, 0.5)]
FORMAMIDE = [("C", 0.0, 0.15, 0.0), ("O", 1.15, 0.45, 0.0),
             ("N", -1.0, 1.0, 0.0), ("H", -0.8, 2.0, 0.0),
             ("H", -2.0, 0.7, 0.0), ("H", -0.4, -0.9, 0.0)]

BUTANE = [("C", 1.9, 0.2, 0.0), ("C", 0.6, -0.5, 0.2), ("C", -0.6, 0.4, 0.0),
          ("C", -1.9, -0.3, 0.2), ("H", 2.0, 1.1, 0.6), ("H", 2.8, -0.4, 0.2),
          ("H", 1.9, 0.5, -1.0), ("H", 0.6, -1.4, -0.4), ("H", 0.5, -0.8, 1.2),
          ("H", -0.5, 1.3, 0.6), ("H", -0.6, 0.7, -1.0), ("H", -2.7, 0.4, 0.0),
          ("H", -2.0, -1.2, -0.4), ("H", -2.0, -0.6, 1.2)]


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def deck(atoms, theory="GFN-FF", extra="", runtype="Conformer Search"):
    """The input deck ai.crest writes for a molecule."""
    tmp = tempfile.mkdtemp(prefix="crestdeck-")
    try:
        open(os.path.join(tmp, "x.param"), "w").write(
            "title:\tcresttest\nTheory:\t%s\nCategory:\tSE\n"
            "RunType:\t%s\nCharge:\t0\n"
            "ES.ChemSys.Multiplicity:\t1\n%s" % (theory, runtype, extra))
        frag = ["x", "title:\tcresttest", "type:\tmolecule",
                "num_atoms:\t%d" % len(atoms), "atom_info:\tsymbol cart",
                "atom_list:"]
        frag += [" %s %s %s %s" % a for a in atoms]
        open(os.path.join(tmp, "x.frag"), "w").write("\n".join(frag) + "\n")
        open(os.path.join(tmp, "crest.crestin"), "w").write(
            "##CrestHeader##\n##chemsys##\n")
        env = dict(os.environ, ECCE_HOME=REPO)
        res = subprocess.run(
            ["perl", os.path.join(REPO, "scripts", "parsers", "ai.crest"),
             "-p", "-f", "-n", "x", "-t", "crest.crestin"],
            cwd=tmp, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        text = open(os.path.join(tmp, "crest.crestin")).read()
        return text, res.returncode
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def calcDir(url):
    path = url.rstrip("/")
    return path[len("file://"):] if path.startswith("file://") else path


def numbers(calc, key):
    with open(os.path.join(calc, "Props", key), errors="replace") as handle:
        body = handle.read().split(">", 2)[2]
    return [float(v) for v in re.findall(r"[-+]?\d+\.\d+(?:[eE][-+]?\d+)?", body)]


def runJob(s, name, text, work, rundir, atoms=None):
    path = os.path.join(work, name + ".crestin")
    open(path, "w").write(text)
    #  The code and the structure, as the Calculation Editor stores them:
    #  the Builder's orbital views look the basis ordering up by code.
    extra = ["procs=2", "code=CREST"]
    if atoms:
        xyz = os.path.join(work, name + ".xyz")
        with open(xyz, "w") as h:
            h.write("%d\n%s\n" % (len(atoms), name))
            for a in atoms:
                h.write("%s %.6f %.6f %.6f\n" % a)
        extra.append("xyz=" + xyz)
    url, out = s.create(name, "crest_es", path, "xtb.crestin", rundir,
                        extra=tuple(extra))
    if not check(url is not None, "%s created" % name):
        say(out)
        return None
    rc, out = s.launch(url)
    say("\n".join("  | " + l for l in out.strip().splitlines()[-6:]))
    if not check(rc == 0, "%s: Launch ran to the end" % name):
        return None
    state = s.waitState(url, seconds=300)
    if not check(state == "completed",
                 "%s reached completed (last: %s)" % (name, state)):
        say(s.driver("reason", url)[1])
        return None
    time.sleep(2)
    return calcDir(url)


def storedOutput(calc):
    path = os.path.join(calc, "Outputs", "crest.crestout")
    check(os.path.exists(path), "the output was stored as Outputs/crest.crestout")
    return open(path, errors="replace").read() if os.path.exists(path) else ""


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    crest, xtb = shutil.which("crest"), shutil.which("xtb")
    if not crest or not xtb:
        harness.skip("crest and xtb are not both installed")
    harness.prerequisites(build, ())

    say("--- option lines")
    text, rc = deck(BUTANE)
    check(rc == 0 and text.splitlines()[0] ==
          "crest --gfnff --chrg 0 --uhf 0 --ewin 6", "default: %r" % text.splitlines()[0])
    text, rc = deck(BUTANE, "GFN2//GFN-FF",
                    "ES.Theory.Solvent.Model:\tALPB\nES.Theory.Solvent.Name:\twater\n"
                    "ES.Runtype.Conformers.EnergyWindow:\t4.5\n"
                    "ES.Runtype.Conformers.NCI:\t1\n"
                    "ES.Runtype.Conformers.NoRefTopo:\t1\n")
    check(rc == 0 and text.splitlines()[0] ==
          "crest --gfn2//gfnff --legacy --chrg 0 --uhf 0 --alpb water --ewin 4.5 "
          "--nci --noreftopo", "all options: %r" % text.splitlines()[0])
    # --notopo only when ticked (#831); the default line above has neither.
    text, rc = deck(BUTANE, "GFN-FF", "ES.Runtype.Conformers.NoTopo:\t1\n")
    check(rc == 0 and text.splitlines()[0] ==
          "crest --gfnff --chrg 0 --uhf 0 --ewin 6 --notopo",
          "no topology check: %r" % text.splitlines()[0])
    text, rc = deck(BUTANE, "GFN-FF", "ES.Runtype.Conformers.NoTopo:\t0\n")
    check(rc == 0 and "--notopo" not in text.splitlines()[0],
          "unticked: no --notopo: %r" % text.splitlines()[0])
    text, rc = deck(BUTANE, "GFN2-xTB", "ES.Theory.Solvent.Model:\tGBSA\n"
                    "ES.Theory.Solvent.Name:\tbenzaldehyde\n")
    check(rc != 0, "GBSA with a solvent it lacks is refused")

    text, rc = deck(METHYLAMINE, "GFN2-xTB", runtype="Protonation Sites")
    check(rc == 0 and text.splitlines()[0] ==
          "crest --gfn2 --chrg 0 --uhf 0 --protonate", "protonate: %r" % text.splitlines()[0])
    text, rc = deck(METHYLAMINE, "GFN2-xTB", runtype="Protonation Sites",
                    extra="ES.Runtype.Sites.EnergyWindow:\t50\n")
    check(rc == 0 and text.splitlines()[0] ==
          "crest --gfn2 --chrg 0 --uhf 0 --protonate --ewin 50",
          "protonate with a window: %r" % text.splitlines()[0])
    text, rc = deck(FORMAMIDE, "GFN2-xTB", runtype="Tautomers")
    check(rc == 0 and text.splitlines()[0] ==
          "crest --gfn2 --chrg 0 --uhf 0 --tautomerize", "tautomerize: %r" % text.splitlines()[0])
    text, rc = deck(METHYLAMINE, "GFN-FF", runtype="Protonation Sites")
    check(rc != 0, "protonation with GFN-FF is refused (CREST needs LMOs)")
    qcg = ("ES.Runtype.QCG.Solvent:\tmethanol\nES.Runtype.QCG.Number:\t4\n")
    text, rc = deck(METHYLAMINE, "GFN2-xTB", runtype="Solvation Shell",
                    extra=qcg)
    lines = text.splitlines()
    check(rc == 0 and lines[0] == "crest --qcg solvent.xyz --nsolv 4 "
          "--gfn2 --chrg 0 --uhf 0 --alpb methanol" and lines[1] == "$solvent"
          and lines[8] == "$end" and lines[9] == "7", "QCG deck: %r" % lines[:10])
    # The Theory dialog's solvent is not used: the solvent is the molecule.
    text, rc = deck(METHYLAMINE, "GFN2-xTB", runtype="Solvation Shell",
                    extra=qcg + "ES.Runtype.QCG.Ensemble:\t1\n"
                    "ES.Theory.Solvent.Model:\tALPB\n"
                    "ES.Theory.Solvent.Name:\tbenzene\n")
    check(rc == 0 and text.splitlines()[0] == "crest --qcg solvent.xyz "
          "--nsolv 4 --ensemble --gfn2 --chrg 0 --uhf 0 --alpb methanol",
          "QCG ensemble deck: %r" % text.splitlines()[0])
    text, rc = deck(METHYLAMINE, "GFN2-xTB", runtype="Solvation Shell",
                    extra=qcg + "ES.Theory.Solvent.Model:\tNone\n")
    check(rc == 0 and "--alpb" not in text.splitlines()[0],
          "QCG with model None has no --alpb")

    s = harness.Session(build, "crest", {"xTB": xtb, "CREST": crest},
                        local=True, keep=args.keep)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    os.makedirs(rundir, exist_ok=True)
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1

        say("--- n-butane, GFN-FF conformer search")
        text, rc = deck(BUTANE)
        calc = runJob(s, "c-%d" % int(time.time()), text, work, rundir, BUTANE)
        if calc:
            out = storedOutput(calc)
            check("CREST terminated normally" in out, "CREST ended normally")
            props = s.props("file://" + calc)
            say("  properties: " + " ".join(props))
            for key in ("GEOMTRACE", "CONFREL"):
                check(key in props, "%s present in Props/" % key)
            rel = numbers(calc, "CONFREL")
            geo = numbers(calc, "GEOMTRACE")
            check(len(rel) >= 2, "%d conformers stored" % len(rel))
            check(len(geo) == len(rel) * 14 * 3,
                  "GEOMTRACE has one frame of 14 atoms per conformer")
            check(rel and rel[0] == 0.0 and rel == sorted(rel),
                  "relative energies start at 0 and ascend: %r" % rel)
            check("ECCE-CREST-BEST" in out, "crest_best.xyz brought back")

        #  CREST's other run types write their ensemble to another file.
        for name, atoms, runtype, key in (
                ("p", METHYLAMINE, "Protonation Sites", "protonate"),
                ("t", FORMAMIDE, "Tautomers", "tautomerize")):
            say("--- %s, GFN2-xTB" % runtype)
            text, rc = deck(atoms, "GFN2-xTB", runtype=runtype)
            calc = runJob(s, "%s-%d" % (name, int(time.time())), text, work,
                          rundir, atoms)
            if not calc:
                continue
            out = storedOutput(calc)
            check("CREST terminated normally" in out, "CREST ended normally")
            props = s.props("file://" + calc)
            check(os.path.exists(os.path.join(calc, "Props", "GEOMTRACE")),
                  "GEOMTRACE stored")
            rel = numbers(calc, "CONFREL")
            geo = numbers(calc, "GEOMTRACE")
            #  A protomer has one more atom (the proton).
            nat = len(atoms) + (1 if key == "protonate" else 0)
            check(len(rel) >= 1 and len(geo) == len(rel) * nat * 3,
                  "%d structures of %d atoms stored" % (len(rel), nat))
            check(rel and rel[0] == 0.0 and rel == sorted(rel),
                  "relative energies start at 0 and ascend: %r" % rel)

        #  CREST 3.0.2's QCG docks with `xtb dock`, which aborts in Debian's
        #  xtb 6.7.1 (stack smashing); run it only where that works.
        say("--- Solvation Shell (QCG), GFN2-xTB")
        probe = tempfile.mkdtemp(prefix="qcgprobe-")
        try:
            for n, a in (("a.xyz", METHYLAMINE), ("b.xyz", [
                    ("O", 0.0, 0.0, 0.1173), ("H", 0.0, 0.7572, -0.4692),
                    ("H", 0.0, -0.7572, -0.4692)])):
                with open(os.path.join(probe, n), "w") as h:
                    h.write("%d\n%s\n" % (len(a), n))
                    for t in a:
                        h.write("%s %.4f %.4f %.4f\n" % t)
            res = subprocess.run([crest, "a.xyz", "--qcg", "b.xyz", "--nsolv",
                                  "1", "--gfn2", "--chrg", "0", "--uhf", "0",
                                  "-T", "2"], cwd=probe,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, timeout=240)
            grown = os.path.join(probe, "grow", "qcg_grow.xyz")
            qcgWorks = os.path.exists(grown) and os.path.getsize(grown) > 0
        except Exception:
            qcgWorks = False
        finally:
            shutil.rmtree(probe, ignore_errors=True)
        if not qcgWorks:
            say("  SKIP QCG run: CREST's docking step fails with this xtb")
        else:
            text, rc = deck(METHYLAMINE, "GFN2-xTB", runtype="Solvation Shell",
                            extra="ES.Runtype.QCG.Number:\t2\n")
            calc = runJob(s, "q-%d" % int(time.time()), text, work, rundir,
                          METHYLAMINE)
            if calc:
                check("GEOMTRACE" in s.props("file://" + calc) or os.path.exists(
                    os.path.join(calc, "Props", "GEOMTRACE")),
                    "the QCG ensemble was stored")
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
