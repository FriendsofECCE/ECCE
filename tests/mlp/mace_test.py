#!/usr/bin/env python3
"""One real MACE run: NaCl, rocksalt cell, cell and atoms relaxed (#728).

Needs mace-torch, torch and ASE in the interpreter named by ECCE_MLP_PYTHON
(default: the one running this script) and the medium-mpa-0 weights, which
mace downloads on first use (network).  SKIPs (77) when mace is not importable.

    ECCE_MLP_PYTHON=~/opt/mace-venv/bin/python tests/mlp/mace_test.py

Expected from MACE-MPA-0 on NaCl (PBE-trained): a = 5.6 to 5.8 A and about
-3.4 eV per atom.  The bounds are loose; they catch a wrong unit or a broken
cell filter, not model accuracy.
"""

import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
RUNNER = os.path.join(os.path.dirname(os.path.dirname(HERE)), "scripts", "ecce-mlp")

DECK = """mlp runtype=Geometry model=medium-mpa-0 dtype=float32 dispersion=off relax=cell optimizer=FIRE fmax=0.05 steps=200 timelimit=0
8
Lattice="5.7 0 0 0 5.7 0 0 0 5.7" Properties=species:S:1:pos:R:3 pbc="T T T"
Na 0.00 0.00 0.00
Na 2.85 2.85 0.00
Na 2.85 0.00 2.85
Na 0.00 2.85 2.85
Cl 2.85 0.00 0.00
Cl 0.00 2.85 0.00
Cl 0.00 0.00 2.85
Cl 2.85 2.85 2.85
"""


# Runs in the MACE environment: ase.vibrations as the reference.  The
# molecule is relaxed first so the rigid motions (projected out by ecce-mlp,
# not by ase) do not mix with the vibrations.
REFERENCE = r"""
import io, json, sys
from ase.io import read, write
from ase.optimize import BFGS
from ase.vibrations import Vibrations
from mace.calculators import mace_mp
atoms = read(io.StringIO(sys.stdin.read()), format="extxyz")
atoms.calc = mace_mp(model="medium-mpa-0", default_dtype=sys.argv[1])
if not atoms.pbc.any():
    BFGS(atoms, logfile=None).run(fmax=1e-4)
vib = Vibrations(atoms, delta=0.01)
vib.clean(); vib.run(); f = vib.get_frequencies(); vib.clean()
buf = io.StringIO()
write(buf, atoms, format="extxyz", columns=["symbols", "positions"])
print(json.dumps({"freq": [x.real if abs(x.imag) < 1e-9 else -abs(x.imag) for x in f],
                  "xyz": buf.getvalue()}))
"""


def vibration_checks(python, env, work, a, check):
    import json
    h = a / 2
    nacl = ("8\nLattice=\"%g 0 0 0 %g 0 0 0 %g\" Properties=species:S:1:pos:R:3 pbc=\"T T T\"\n"
            % (a, a, a)) + "".join("%s %g %g %g\n" % (s, x * h, y * h, z * h) for s, x, y, z in
            [("Na", 0, 0, 0), ("Na", 1, 1, 0), ("Na", 1, 0, 1), ("Na", 0, 1, 1),
             ("Cl", 1, 0, 0), ("Cl", 0, 1, 0), ("Cl", 0, 0, 1), ("Cl", 1, 1, 1)])
    water = "3\nProperties=species:S:1:pos:R:3\nO 0 0 0\nH 0.76 0.59 0\nH -0.76 0.59 0\n"
    for what, structure, nrigid in (("water", water, 6), ("NaCl (Gamma)", nacl, 3)):
        for dtype in ("float64", "float32"):
            ref = subprocess.run([python, "-c", REFERENCE, dtype], input=structure, cwd=work,
                                 stdout=subprocess.PIPE, universal_newlines=True, env=env)
            data = json.loads(ref.stdout.strip().splitlines()[-1])
            deck = os.path.join(work, "vib.mlpin")
            with open(deck, "w") as handle:
                handle.write("mlp runtype=Vibration model=medium-mpa-0 dtype=%s delta=0.01\n%s"
                             % (dtype, data["xyz"]))
            res = subprocess.run([python, RUNNER, deck], cwd=work, env=env,
                                 stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                 universal_newlines=True)
            ours = [float(f) for f in re.findall(r"^mode \d+ (\S+)", res.stdout, re.M)]
            want = sorted(sorted(data["freq"], key=abs)[nrigid:])
            got = sorted(ours[nrigid:])
            diff = max(abs(x - y) for x, y in zip(got, want)) if got else 99
            print("  %s %s: %s" % (what, dtype, " ".join("%.1f" % f for f in got)))
            check(len(ours) == len(data["freq"]) and diff < 1.0,
                  "%s, %s: frequencies match ase.vibrations to 1 cm-1 (max diff %.3f)"
                  % (what, dtype, diff))


def main():
    python = os.path.expanduser(os.environ.get("ECCE_MLP_PYTHON", sys.executable))
    probe = subprocess.run([python, "-c", "import mace, torch, ase"],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if probe.returncode != 0:
        print("SKIP: mace, torch and ase are not importable by %s" % python)
        return 77
    failures = []

    def check(ok, what):
        print("  %s %s" % ("ok  " if ok else "FAIL", what))
        if not ok:
            failures.append(what)

    work = tempfile.mkdtemp(prefix="mlp-mace-")
    deck = os.path.join(work, "deck.mlpin")
    with open(deck, "w") as handle:
        handle.write(DECK)
    env = dict(os.environ)
    env.pop("ECCE_MLP_MOCK", None)
    res = subprocess.run([python, RUNNER, deck], cwd=work, env=env,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         universal_newlines=True)
    out = res.stdout
    print("\n".join(out.splitlines()[:12]))
    check(res.returncode == 0 and "ecce-mlp terminated normally" in out, "the run ends normally")
    check("Model:        MACE medium-mpa-0" in out and "mace-torch" in out
          and "Licence:      MIT" in out, "model, version and licence are in the output")
    check("Optimisation converged" in out, "the optimisation converged")
    cells = re.findall(r"^cell: (.+)$", out, re.M)
    if check(bool(cells), "cells are printed"):
        pass
    if cells:
        a = float(cells[-1].split()[0])
        check(5.6 < a < 5.8, "lattice parameter %.3f A is between 5.6 and 5.8" % a)
    m = re.search(r"^Total energy: (\S+) eV", out, re.M)
    if check(m is not None, "the total energy is printed"):
        per_atom = float(m.group(1)) / 8
        check(-4.0 < per_atom < -2.8, "energy %.3f eV/atom is in the range for NaCl" % per_atom)
    check(os.path.exists(os.path.join(work, "ecce-mlp.extxyz")), "the extxyz trajectory exists")
    vibration_checks(python, env, work, a if cells else 5.7, check)
    print("")
    if failures:
        print("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        print(out[-1500:])
        return 1
    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
