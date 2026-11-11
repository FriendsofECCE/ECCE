#!/usr/bin/env python3
"""ecce-mlp-driver (ECCE's optimisers, cell relaxation and dynamics) against
ASE as the reference (#728).

    tests/mlp/driver_test.py --driver <ecce-mlp-driver> [--mace <python>]

Without --mace: the built-in Lennard-Jones (needs no Python packages) through
Energy, Geometry (FIRE and BFGS, atoms and cell, with a pressure), Vibration
and MD; with ASE importable, the same relaxations by ASE must end at the same
energy (1e-4 eV/atom) and cell (1e-3 A).  --mace names a Python with
mace-torch, ASE and the medium-mpa-0 weights: water, an NaCl cell and an
NaCl(100) slab relaxed by both, an NVE run (energy drift bound) and an NVT run
(mean temperature) of a 64-atom NaCl box.  SKIPs (77) without a driver.
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
RUNNER = os.path.join(os.path.dirname(os.path.dirname(HERE)), "scripts", "ecce-mlp")

failures = []


def check(ok, what):
    print("  %s %s" % ("ok  " if ok else "FAIL", what), flush=True)
    if not ok:
        failures.append(what)
    return ok


def nacl(a=5.7, rattle=True, rep=1, vacuum=0.0):
    base = [("Na", 0, 0, 0), ("Na", .5, .5, 0), ("Na", .5, 0, .5), ("Na", 0, .5, .5),
            ("Cl", .5, 0, 0), ("Cl", 0, .5, 0), ("Cl", 0, 0, .5), ("Cl", .5, .5, .5)]
    atoms = []
    for i in range(rep):
        for j in range(rep):
            for k in range(rep):
                for n, (s, x, y, z) in enumerate(base):
                    d = 0.07 * ((n * 7 % 5) - 2) / 2 if rattle else 0
                    atoms.append((s, (x + i) * a + d, (y + j) * a - d / 2, (z + k) * a + d / 3))
    L = a * rep
    cell = "%g 0 0 0 %g 0 0 0 %g" % (L, L, L + vacuum)
    lines = ["%d" % len(atoms),
             'Lattice="%s" Properties=species:S:1:pos:R:3 pbc="T T T"' % cell]
    lines += ["%s %.6f %.6f %.6f" % t for t in atoms]
    return "\n".join(lines) + "\n"


WATER = """3
Properties=species:S:1:pos:R:3
O 0.0 0.0 0.12
H 0.0 0.80 -0.45
H 0.0 -0.74 -0.50
"""


def run(work, opts, structure, engine, python, driver, mock=None):
    deck = os.path.join(work, "deck.mlpin")
    with open(deck, "w") as handle:
        handle.write("mlp " + opts + "\n" + structure)
    env = dict(os.environ)
    env.pop("ECCE_MLP_MOCK", None)
    if mock:
        env["ECCE_MLP_MOCK"] = mock
    env["ECCE_MLP_ENGINE"] = engine
    env["ECCE_MLP_DRIVER"] = driver
    if engine == "ecce" and mock == "lj":
        cmd = [driver, deck]          # no Python at all
    else:
        cmd = [python, RUNNER, deck]
    t0 = time.time()
    proc = subprocess.run(cmd, cwd=work, env=env, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, universal_newlines=True)
    return proc.returncode, proc.stdout, time.time() - t0


def last_frame(out):
    blocks = re.findall(r"ECCE-MLP-FRAME \d+ (\d+)\n(.*?)ECCE-MLP-ENDFRAME", out, re.S)
    n, body = blocks[-1]
    energy = float(re.search(r"energy: (\S+)", body).group(1))
    cell = re.search(r"cell: (.*)", body)
    return int(n), energy, [float(v) for v in cell.group(1).split()] if cell else None, len(blocks)


def compare(work, label, opts, structure, python, driver, mock=None):
    rc1, out1, t1 = run(work, opts, structure, "ase", python, driver, mock)
    rc2, out2, t2 = run(work, opts, structure, "ecce", python, driver, mock)
    if not check(rc1 == 0 and rc2 == 0 and "terminated normally" in out1 + out2,
                 "%s: both engines ran" % label):
        print(out1[-1500:], out2[-1500:])
        return
    check("Engine:       ECCE" in out2 and "Engine:       ASE" in out1,
          "%s: the engines are the ones asked for" % label)
    n, e1, c1, f1 = last_frame(out1)
    _, e2, c2, f2 = last_frame(out2)
    de = abs(e1 - e2) / n
    dc = max(abs(a - b) for a, b in zip(c1, c2)) if c1 else 0.0
    check(de < 1e-4 and dc < 1e-3,
          "%s: ASE %.6f eV in %d steps (%.1f s), ECCE %.6f eV in %d steps (%.1f s); "
          "|dE| %.1e eV/atom, |d cell| %.1e A" % (label, e1, f1 - 1, t1, e2, f2 - 1, t2, de, dc))


def md_series(out):
    rows = re.findall(r"^MD step\s+(\d+)\s+time (\S+) ps\s+Epot (\S+) eV\s+Ekin (\S+) eV\s+"
                      r"T (\S+) K\s+Etot (\S+) eV", out, re.M)
    return [tuple(float(v) for v in r) for r in rows]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--driver", required=True)
    ap.add_argument("--mace")
    args = ap.parse_args()
    if not os.access(args.driver, os.X_OK):
        print("SKIP: no ecce-mlp-driver at", args.driver)
        sys.exit(77)
    python = args.mace or sys.executable
    try:
        subprocess.run([python, "-c", "import ase"], check=True, stderr=subprocess.DEVNULL)
        have_ase = True
    except subprocess.CalledProcessError:
        have_ase = False
    work = tempfile.mkdtemp(prefix="mlpdrv")

    print("Built-in Lennard-Jones (no Python packages)")
    for rt in ("Energy", "Vibration"):
        rc, out, _ = run(work, "runtype=%s" % rt, nacl(), "ecce", python, args.driver, "lj")
        check(rc == 0 and "terminated normally" in out and "Lennard-Jones, built in" in out,
              "%s runs with ECCE_MLP_MOCK=lj and no Python" % rt)
    rc, out, _ = run(work, "runtype=Vibration", WATER, "ecce", python, args.driver, "lj")
    check("3 rigid motions" not in out and "6 rigid motions removed" in out,
          "a molecule loses translations and rotations")
    rc, out, _ = run(work, "runtype=MD ensemble=NVE timestep=2 temperature=50 mdsteps=400 every=20",
                     nacl(rattle=False), "ecce", python, args.driver, "lj")
    rows = md_series(out)
    etot = [r[5] for r in rows]
    check(rc == 0 and len(rows) == 21 and max(etot) - min(etot) < 1e-3 * 8,
          "LJ NVE: 21 frames, Etot spread %.2e eV" % (max(etot) - min(etot) if etot else -1))
    check(out.count("time: ") == 21, "every MD frame carries its time")
    if have_ase:
        for o in ("FIRE", "BFGS"):
            for rel in ("atoms", "cell"):
                compare(work, "LJ %s %s p=0.5 GPa" % (o, rel),
                        "runtype=Geometry optimizer=%s relax=%s fmax=0.001 steps=3000 pressure=0.5"
                        % (o, rel), nacl(), python, args.driver, "lj")
    else:
        print("  (no ASE: the reference comparison is skipped)")

    if args.mace:
        print("MACE medium-mpa-0, float64")
        common = "model=medium-mpa-0 dtype=float64 fmax=0.005 steps=500"
        compare(work, "water FIRE", "runtype=Geometry optimizer=FIRE " + common,
                WATER, python, args.driver)
        compare(work, "NaCl cell BFGS", "runtype=Geometry optimizer=BFGS relax=cell " + common,
                nacl(), python, args.driver)
        compare(work, "NaCl cell FIRE 1 GPa",
                "runtype=Geometry optimizer=FIRE relax=cell pressure=1 " + common,
                nacl(), python, args.driver)
        compare(work, "NaCl(100) slab FIRE", "runtype=Geometry optimizer=FIRE " + common,
                nacl(rep=1, vacuum=10.0), python, args.driver)

        box = nacl(a=5.65, rattle=False, rep=2)
        rc, out, t = run(work, "runtype=MD ensemble=NVE timestep=2 temperature=300 mdsteps=1000 "
                         "every=10 model=medium-mpa-0 dtype=float64", box, "ecce", python, args.driver)
        rows = md_series(out)
        drift = re.search(r"drift (\S+) eV/atom/ps", out)
        if check(rc == 0 and len(rows) == 101 and drift, "NVE 64 atoms, 1000 x 2 fs ran (%.0f s)" % t):
            etot = [r[5] for r in rows]
            check(abs(float(drift.group(1))) < 1e-3 and max(etot) - min(etot) < 0.01,
                  "NVE drift %s eV/atom/ps (bound 1e-3), Etot spread %.4f eV (bound 0.01)"
                  % (drift.group(1), max(etot) - min(etot)))
        rc, out, t = run(work, "runtype=MD ensemble=NVT timestep=2 temperature=300 tau=50 "
                         "mdsteps=3000 every=10 model=medium-mpa-0 dtype=float32",
                         box, "ecce", python, args.driver)
        rows = md_series(out)
        if check(rc == 0 and len(rows) == 301, "NVT 64 atoms, 3000 x 2 fs ran (%.0f s)" % t):
            temps = [r[4] for r in rows if r[0] >= 1000]
            mean = sum(temps) / len(temps)
            check(abs(mean - 300) < 15,
                  "NVT mean T over 4-6 ps %.1f K (300 +- 15)" % mean)

    for name in os.listdir(work):
        os.remove(os.path.join(work, name))
    os.rmdir(work)
    print("%d failure(s)" % len(failures))
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
