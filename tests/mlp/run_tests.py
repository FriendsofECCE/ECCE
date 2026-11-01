#!/usr/bin/env python3
"""scripts/ecce-mlp and its ECCE glue, with a model-free mock calculator (#728).

ECCE_MLP_MOCK=lj swaps the network for ASE's Lennard-Jones, so this needs ASE
but no PyTorch and no weights.  Checks: Energy and Geometry runs of a periodic
NaCl cell (atoms and cell), the step and the time limit keeping the last
structure, the extxyz trajectory, the bad-input messages, ai.mlp's deck for the
dictionary the Calculation Editor writes, and the parsers (mlp.version,
mlp.geomtrace, mlp.final) on the real output.

    tests/mlp/run_tests.py

Exit status 77 (CTest SKIP) when ASE cannot be imported.
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
RUNNER = os.path.join(REPO, "scripts", "ecce-mlp")
PARSERS = os.path.join(REPO, "scripts", "parsers")

failures = []


def check(ok, what):
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


NACL = """8
Lattice="5.7 0 0 0 5.7 0 0 0 5.7" Properties=species:S:1:pos:R:3 pbc="T T T"
Na 0 0 0
Na 2.85 2.85 0
Na 2.85 0 2.85
Na 0 2.85 2.85
Cl 2.85 0 0
Cl 0 2.85 0
Cl 0 0 2.85
Cl 2.85 2.85 2.85
"""
DIMER = """2
Properties=species:S:1:pos:R:3
Na 0 0 0
Cl 0 0 3.3
"""


def run(work, opts, structure, mock="lj"):
    deck = os.path.join(work, "deck.mlpin")
    with open(deck, "w") as handle:
        handle.write("mlp " + opts + "\n" + structure)
    env = dict(os.environ)
    env.pop("ECCE_MLP_MOCK", None)
    if mock:
        env["ECCE_MLP_MOCK"] = mock
    res = subprocess.run([sys.executable, RUNNER, deck], cwd=work, env=env,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         universal_newlines=True)
    return res.returncode, res.stdout


def parse(script, key, text, begin, end, skip):
    """Feed the lines of one block to a parser script, as eccejobmonitor does."""
    lines = text.splitlines()
    start = next(i for i, l in enumerate(lines) if re.match(begin, l))
    stop = next(i for i in range(start, len(lines)) if re.match(end, lines[i]))
    block = "\n".join(lines[start + skip:stop]) + "\n"
    res = subprocess.run(["perl", os.path.join(PARSERS, script), key, "Energy", "SE", "MACE"],
                         input=block, stdout=subprocess.PIPE, universal_newlines=True)
    return res.stdout


def main():
    try:
        import ase  # noqa: F401
    except ImportError:
        print("SKIP: ASE is not importable")
        return 77

    work = tempfile.mkdtemp(prefix="mlp-test-")
    try:
        print("--- Energy of a periodic cell")
        rc, out = run(work, "runtype=Energy model=medium-mpa-0", NACL)
        check(rc == 0 and "ecce-mlp terminated normally" in out, "Energy run ends normally")
        check("Model:        mock (ASE Lennard-Jones)" in out, "the model is named in the output")
        check("energies are for screening, not for reporting" in out, "the screening note is printed")
        check(out.count("ECCE-MLP-FRAME") == 1, "one frame")
        check("stress (kbar):" in out, "stress is printed for a cell")

        print("--- Geometry: atoms and cell")
        rc, out = run(work, "runtype=Geometry relax=cell optimizer=FIRE fmax=0.05 steps=200", NACL)
        check(rc == 0 and "Optimisation converged" in out, "FIRE with the cell filter converges")
        steps = len(re.findall(r"^Step\s+\d+\s*$", out, re.M))
        check(steps >= 3 and out.count("ECCE-MLP-FRAME") == steps,
              "%d steps, one frame each" % steps)
        cells = re.findall(r"^cell: (\S+)", out, re.M)
        check(len(cells) == steps and cells[0] != cells[-1], "the cell changed during the run")
        traj = open(os.path.join(work, "ecce-mlp.extxyz")).read()
        check(traj.count("Lattice=") == steps, "the extxyz trajectory has every step")
        rc, out2 = run(work, "runtype=Geometry relax=atoms optimizer=BFGS fmax=0.05 steps=200", NACL)
        check(rc == 0 and "Optimisation converged" in out2 and "FrechetCellFilter" not in out2,
              "BFGS with fixed cell converges")

        print("--- Limits keep the last structure")
        rc, out = run(work, "runtype=Geometry relax=cell fmax=0.00001 steps=3", NACL)
        check(rc == 0 and "ecce-mlp: NOT CONVERGED: stopped at the step limit of 3" in out,
              "step limit stops the run and says so")
        check("ECCE-MLP-FINAL" in out and "ecce-mlp terminated normally" in out,
              "the last structure is still written")
        rc, out = run(work, "runtype=Geometry relax=cell fmax=0.00001 steps=500 timelimit=0.0001", NACL)
        check(rc == 0 and "stopped at the time limit" in out, "time limit stops the run")

        print("--- Bad input")
        rc, out = run(work, "runtype=Geometry relax=cell", DIMER)
        check(rc != 0 and "no cell" in out, "cell relaxation of a molecule is refused")
        rc, out = run(work, "runtype=Energy model=nonsense", DIMER, mock="")
        check(rc != 0 and "unknown model" in out, "an unknown model is refused")
        rc, out = run(work, "runtype=Energy model=medium-mpa-0", DIMER, mock="")
        try:
            import mace  # noqa: F401
            check(True, "mace present: not checking the missing-package message")
        except ImportError:
            check(rc != 0 and "mace package is not importable" in out,
                  "a missing mace gives an install hint, not a traceback")
        rc, out = run(work, "runtype=Energy", DIMER, mock="")
        check(rc != 0 and "Traceback" not in out, "no traceback without the mock")

        print("--- Parsers on the Geometry output")
        rc, out = run(work, "runtype=Geometry relax=cell fmax=0.05 steps=50", NACL)
        ver = parse("mlp.version", "VERSION", out, r"^Model:", r"^Precision:", 0)
        check("mock (ASE Lennard-Jones)" in ver and "none (test calculator)" in ver,
              "VERSION carries model, version and licence")
        geo = parse("mlp.geomtrace", "GEOMTRACE", out, r"^ECCE-MLP-FRAME", r"^ECCE-MLP-ENDFRAME", 1)
        check("key: GEOMTRACE" in geo and "key: TEVEC" in geo and "key: LATTICEVEC" in geo,
              "a frame gives GEOMTRACE, TEVEC and LATTICEVEC")
        check(geo.count("-Na") == 4 and geo.count("-Cl") == 4, "8 atoms in the frame")
        fin = parse("mlp.final", "TE", out, r"^ECCE-MLP-FINAL", r"^ECCE-MLP-ENDFINAL", 1)
        for key in ("TE", "EGRADVEC", "STRESS", "PRESSURE"):
            check("key: %s\n" % key in fin, "final block gives %s" % key)
        te = float(re.search(r"key: TE\nsize:\n1\nvalues:\n(\S+)", fin).group(1))
        ev = float(re.search(r"^Total energy: (\S+) eV", out, re.M).group(1))
        check(abs(te - ev / 27.211386245988) < 1e-8, "TE is the eV energy in Hartree")
        rc, mol = run(work, "runtype=Energy", DIMER)
        fin = parse("mlp.final", "TE", mol, r"^ECCE-MLP-FINAL", r"^ECCE-MLP-ENDFINAL", 1)
        check("key: TE" in fin and "key: STRESS" not in fin, "a molecule has no stress")

        print("--- ai.mlp deck")
        def deck(param, lattice):
            d = tempfile.mkdtemp(prefix="mlpdeck-", dir=work)
            open(os.path.join(d, "x.param"), "w").write(param)
            frag = ["x", "title:\tmlptest", "type:\tmolecule"]
            if lattice:
                frag += ["lattice_vectors:", "5.7 0 0", "0 5.7 0", "0 0 5.7"]
            frag += ["num_atoms:\t2", "atom_info:\tsymbol cart", "atom_list:",
                     " Na 0 0 0", " Cl 0 0 2.8"]
            open(os.path.join(d, "x.frag"), "w").write("\n".join(frag) + "\n")
            open(os.path.join(d, "mlp.mlpin"), "w").write("##MlpHeader##\n##chemsys##\n")
            res = subprocess.run(["perl", os.path.join(PARSERS, "ai.mlp"), "-p", "-f", "-n", "x",
                                  "-t", "mlp.mlpin"], cwd=d, env=dict(os.environ, ECCE_HOME=REPO),
                                 stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            f = os.path.join(d, "mlp.mlpin")
            return res.returncode, (open(f).read() if os.path.exists(f) else "")
        base = "title:\tx\nTheory:\tMACE\nCategory:\tSE\n"
        rc, text = deck(base + "RunType:\tEnergy\n", False)
        check(rc == 0 and text.splitlines()[0] ==
              "mlp runtype=Energy model=medium-mpa-0 dtype=float32 dispersion=off", "Energy line: %r" % text.splitlines()[0])
        check(text.splitlines()[1] == "2" and text.splitlines()[2] == "Properties=species:S:1:pos:R:3",
              "molecule: count and plain comment line")
        rc, text = deck(base + "RunType:\tGeometry\nES.Theory.MLP.Model:\tmedium-omat-0 (ASL)\n"
                        "ES.Theory.MLP.Precision:\tfloat64\nES.Theory.MLP.Dispersion:\t1\n"
                        "ES.Runtype.GeomOpt.Optimizer:\tBFGS\nES.Runtype.GeomOpt.RelaxCell:\t1\n"
                        "ES.Runtype.GeomOpt.Fmax:\t0.02\nES.Runtype.GeomOpt.MaxSteps:\t50\n"
                        "ES.Runtype.GeomOpt.TimeLimit:\t30\n", True)
        check(rc == 0 and text.splitlines()[0] ==
              "mlp runtype=Geometry model=medium-omat-0 dtype=float64 dispersion=on relax=cell "
              "optimizer=BFGS fmax=0.02 steps=50 timelimit=30", "Geometry line: %r" % text.splitlines()[0])
        check('Lattice="5.7000000000 0.0000000000 0.0000000000' in text.splitlines()[2]
              and 'pbc="T T T"' in text.splitlines()[2], "cell goes into the extxyz comment line")
        rc, text = deck(base + "RunType:\tGeometry\nES.Runtype.GeomOpt.RelaxCell:\t1\n", False)
        check(rc != 0, "relaxing the cell of a structure without one is refused")
        rc, text = deck(base + "RunType:\tVibration\n", False)
        check(rc != 0, "an unsupported run type is refused")
    finally:
        shutil.rmtree(work, ignore_errors=True)

    print("")
    if failures:
        print("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
