#!/usr/bin/env python3
"""ML potential (ecce-mlp) through the real Launch (#728).

Local data, no data server.  A periodic NaCl cell, the deck made by the real
ai.mlp, run on `localhost` through Launch, gensub (sub mlp: the runner is
written into the run directory), eccejobmaster, eccejobstore and
eccejobmonitor.  The network is replaced by ASE's Lennard-Jones through
ECCE_MLP_MOCK, so this needs ASE and nothing else.  A Geometry run with the
cell must reach "completed" with GEOMTRACE, TEVEC, LATTICEVEC, TE, EGRADVEC,
STRESS and the model name stored; one stopped at its step limit must end
"unsuccessful" (not converged) and still keep its last structure.

    tests/launch/mlp_test.py [--build build-cmake] [--keep]

Exit status 77 (CTest SKIP) without ASE or the built binaries.
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

NACL = [("Na", 0, 0, 0), ("Na", 2.85, 2.85, 0), ("Na", 2.85, 0, 2.85),
        ("Na", 0, 2.85, 2.85), ("Cl", 2.85, 0, 0), ("Cl", 0, 2.85, 0),
        ("Cl", 0, 0, 2.85), ("Cl", 2.85, 2.85, 2.85)]


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def deck(atoms, runtype="Geometry", extra=""):
    """The input deck ai.mlp writes for a periodic structure."""
    tmp = tempfile.mkdtemp(prefix="mlpdeck-")
    try:
        open(os.path.join(tmp, "x.param"), "w").write(
            "title:\tmlptest\nTheory:\tMACE\nCategory:\tSE\n"
            "RunType:\t%s\n%s" % (runtype, extra))
        frag = ["x", "title:\tmlptest", "type:\tmolecule",
                "lattice_vectors:", "5.7 0 0", "0 5.7 0", "0 0 5.7",
                "num_atoms:\t%d" % len(atoms), "atom_info:\tsymbol cart",
                "atom_list:"]
        frag += [" %s %s %s %s" % a for a in atoms]
        open(os.path.join(tmp, "x.frag"), "w").write("\n".join(frag) + "\n")
        open(os.path.join(tmp, "mlp.mlpin"), "w").write(
            "##MlpHeader##\n##chemsys##\n")
        env = dict(os.environ, ECCE_HOME=REPO)
        res = subprocess.run(
            ["perl", os.path.join(REPO, "scripts", "parsers", "ai.mlp"),
             "-p", "-f", "-n", "x", "-t", "mlp.mlpin"],
            cwd=tmp, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        text = open(os.path.join(tmp, "mlp.mlpin")).read()
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


def runJob(s, name, text, work, rundir, atoms=None, want="completed"):
    path = os.path.join(work, name + ".mlpin")
    open(path, "w").write(text)
    #  The code and the structure, as the Calculation Editor stores them:
    #  the Builder's orbital views look the basis ordering up by code.
    extra = ["procs=2", "code=MLP"]
    if atoms:
        xyz = os.path.join(work, name + ".xyz")
        with open(xyz, "w") as h:
            h.write("%d\n%s\n" % (len(atoms), name))
            for a in atoms:
                h.write("%s %.6f %.6f %.6f\n" % a)
        extra.append("xyz=" + xyz)
    url, out = s.create(name, "mlp_es", path, "mlp.mlpin", rundir,
                        extra=tuple(extra))
    if not check(url is not None, "%s created" % name):
        say(out)
        return None
    rc, out = s.launch(url)
    say("\n".join("  | " + l for l in out.strip().splitlines()[-6:]))
    if not check(rc == 0, "%s: Launch ran to the end" % name):
        return None
    state = s.waitState(url, seconds=300)
    if not check(state == want,
                 "%s reached %s (last: %s)" % (name, want, state)):
        say(s.driver("reason", url)[1])
        return None
    time.sleep(2)
    return calcDir(url)


def storedOutput(calc):
    path = os.path.join(calc, "Outputs", "mlp.mlpout")
    check(os.path.exists(path), "the output was stored as Outputs/mlp.mlpout")
    return open(path, errors="replace").read() if os.path.exists(path) else ""


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    try:
        import ase  # noqa: F401
    except ImportError:
        harness.skip("ASE is not importable")
    harness.prerequisites(build, ())
    os.environ["ECCE_MLP_MOCK"] = "lj"

    say("--- option lines")
    text, rc = deck(NACL, "Energy")
    check(rc == 0 and text.splitlines()[0] ==
          "mlp runtype=Energy model=medium-mpa-0 dtype=float32 dispersion=off",
          "Energy: %r" % text.splitlines()[0])

    s = harness.Session(build, "mlp", {"MLP": sys.executable},
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

        say("--- NaCl cell, FIRE with the cell relaxed")
        text, rc = deck(NACL, "Geometry",
                        "ES.Runtype.GeomOpt.RelaxCell:\t1\n"
                        "ES.Runtype.GeomOpt.Fmax:\t0.05\n")
        calc = runJob(s, "m-%d" % int(time.time()), text, work, rundir, NACL)
        if calc:
            out = storedOutput(calc)
            check("ecce-mlp terminated normally" in out, "ecce-mlp ended normally")
            check("Optimisation converged" in out, "the optimisation converged")
            props = s.props("file://" + calc)
            say("  properties: " + " ".join(props))
            for key in ("GEOMTRACE", "TEVEC", "LATTICEVEC", "TE", "EGRADVEC",
                        "STRESS", "PRESSURE"):
                check(key in props, "%s present in Props/" % key)
            steps = len(re.findall(r"^Step\s+\d+\s*$", out, re.M))
            geo = numbers(calc, "GEOMTRACE")
            check(steps >= 3 and len(geo) == steps * 8 * 3,
                  "GEOMTRACE has one frame of 8 atoms per step (%d)" % steps)
            check(len(numbers(calc, "TEVEC")) == steps, "one energy per step")
            check(len(numbers(calc, "LATTICEVEC")) == steps * 9,
                  "one cell per step")
            #  A string property is stored in the calculation's metadata.
            with open(os.path.join(calc, ".ecce-meta"), errors="replace") as h:
                ver = [l for l in h if "ecce:version" in l]
            check(ver and "Lennard-Jones" in ver[0] and "licence" in ver[0],
                  "the model, version and licence are stored: %r" % ver[:1])

        say("--- Molecular Dynamics, NVT, through ecce-mlp-driver")
        text, rc = deck(NACL, "Molecular Dynamics",
                        "ES.Runtype.MD.Steps:\t40\n"
                        "ES.Runtype.MD.Every:\t10\n"
                        "ES.Runtype.MD.TimeStep:\t2\n")
        first = text.splitlines()[0]
        check(rc == 0 and "runtype=MD " in first and
              "ensemble=NVT timestep=2 temperature=300 " in first,
              "MD deck: %r" % first)
        calc = runJob(s, "d-%d" % int(time.time()), text, work, rundir, NACL)
        if calc:
            out = storedOutput(calc)
            check("Engine:       ECCE" in out and "MD finished" in out,
                  "the driver ran the dynamics")
            props = s.props("file://" + calc)
            for key in ("GEOMTRACE", "TEVEC", "TIMEVEC", "LATTICEVEC"):
                check(key in props, "%s present in Props/" % key)
            times = numbers(calc, "TIMEVEC")
            check(len(times) == 5 and abs(times[-1] - 0.08) < 1e-9 and
                  len(numbers(calc, "TEVEC")) == 5 and
                  len(numbers(calc, "GEOMTRACE")) == 5 * 8 * 3,
                  "5 frames, one time each, last at 0.08 ps: %r" % times)

        say("--- stopped at the step limit")
        text, rc = deck(NACL, "Geometry",
                        "ES.Runtype.GeomOpt.RelaxCell:\t1\n"
                        "ES.Runtype.GeomOpt.Fmax:\t0.00001\n"
                        "ES.Runtype.GeomOpt.MaxSteps:\t3\n")
        calc = runJob(s, "n-%d" % int(time.time()), text, work, rundir, NACL,
                      want="unsuccessful")
        if calc:
            out = storedOutput(calc)
            check("NOT CONVERGED" in out and "ECCE-MLP-FINAL" in out,
                  "the output says it stopped and keeps the last structure")
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
