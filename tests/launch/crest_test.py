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


def deck(atoms, theory="GFN-FF", extra=""):
    """The input deck ai.crest writes for a molecule."""
    tmp = tempfile.mkdtemp(prefix="crestdeck-")
    try:
        open(os.path.join(tmp, "x.param"), "w").write(
            "title:\tcresttest\nTheory:\t%s\nCategory:\tSE\n"
            "RunType:\tConformer Search\nCharge:\t0\n"
            "ES.ChemSys.Multiplicity:\t1\n%s" % (theory, extra))
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
    text, rc = deck(BUTANE, "GFN2-xTB", "ES.Theory.Solvent.Model:\tGBSA\n"
                    "ES.Theory.Solvent.Name:\tbenzaldehyde\n")
    check(rc != 0, "GBSA with a solvent it lacks is refused")

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
