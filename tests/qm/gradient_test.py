#!/usr/bin/env python3
"""qm_gradient: the analytic gradient of ecce-qm against central differences
of its own energy (step 1e-4 bohr), on geometries pushed off equilibrium so
that no component is zero.

    gradient_test.py ECCE_QM_EXE [--quick] [--grid NRAD NANG] [--filter REGEX]

HF must agree to 1e-6 hartree/bohr.  The DFT gradient leaves out the
derivative of the grid weights, so it agrees to about 1e-5 on the default
grid and better on a finer one; the tolerance here is 5e-5 (--grid changes the
grid of both the gradient and the energies).

Cases named "<mol>_<method>_solv" run with the CPCM solvent water (epsilon
80.151); the gradient must also sum to zero over the atoms.
"""
import math, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from molecules import MOLECULES

STEP = 1e-4
TOL_HF, TOL_DFT = 1e-6, 5e-5

def pushed(name):
    """Geometry in bohr, every atom moved by a fixed pseudo-random amount."""
    charge, mult, atoms = MOLECULES[name]
    out = []
    for i, (s, x, y, z) in enumerate(atoms):
        d = [0.05 * math.sin(1.7 * i + k + 0.3) for k in range(3)]
        out.append((s, (x + d[0]) / 0.529177210903, (y + d[1]) / 0.529177210903, (z + d[2]) / 0.529177210903))
    return charge, mult, out

def write_input(path, charge, mult, atoms, method, basis, runtype, grid, solv=False):
    with open(path, "w") as f:
        f.write("charge %d\nmultiplicity %d\nmethod %s\nbasis %s\nunits bohr\n" % (charge, mult, method, basis))
        f.write("runtype %s\nconv_energy 1e-12\nconv_grad 1e-9\nmaxiter 300\n" % runtype)
        if solv:
            f.write("epsilon 80.151\n")
        if grid:
            f.write("grid %d %d\n" % grid)
        f.write("geometry\n")
        for a in atoms:
            f.write("%s %.12f %.12f %.12f\n" % a)
        f.write("end\n")

def run(exe, work, tag, *args):
    inp = os.path.join(work, tag + ".qm")
    write_input(inp, *args)
    p = subprocess.run([exe, inp], capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError("%s failed: %s" % (tag, p.stderr[-300:]))
    return p.stdout

def energy(text):
    return float(re.search(r"^energy_total\s+(\S+)", text, re.M).group(1))

def gradient(text):
    block = re.search(r"begin gradient.*?\n(.*?)end gradient", text, re.S).group(1)
    return [float(v) for line in block.splitlines() for v in line.split()[2:5]]

def check(exe, work, name, method, grid, basis="6-31G*", solv=False):
    charge, mult, atoms = pushed(name)
    text = run(exe, work, "g", charge, mult, atoms, method, basis, "gradient", grid, solv)
    ana = gradient(text)
    fd = []
    for a in range(len(atoms)):
        for k in range(3):
            e = []
            for sign in (1, -1):
                moved = [list(t) for t in atoms]
                moved[a][1 + k] += sign * STEP
                e.append(energy(run(exe, work, "e", charge, mult, [tuple(t) for t in moved], method, basis, "energy", grid, solv)))
            fd.append((e[0] - e[1]) / (2 * STEP))
    err = max(abs(x - y) for x, y in zip(ana, fd))
    net = max(abs(sum(ana[k::3])) for k in range(3))
    return err, net

CASES = [(m, meth) for m in ("h2o", "nh3", "co") for meth in ("hf", "svwn", "pbe", "b3lyp", "pbe0")] + \
        [("o2", meth) for meth in ("rohf", "uhf", "rosvwn", "usvwn", "ropbe", "upbe", "rob3lyp", "ub3lyp", "upbe0")]
SOLV = [("h2o", "hf"), ("nh4", "hf"), ("ch3oh", "hf"), ("h2o", "b3lyp"), ("nh3+", "uhf")]
CASES += [(m, meth, True) for m, meth in SOLV]
QUICK = [("h2o", "hf"), ("h2o", "b3lyp"), ("o2", "rohf"), ("o2", "ub3lyp"), ("h2o", "hf", True), ("nh3+", "uhf", True)]

def main():
    args = sys.argv[1:]
    exe = args.pop(0)
    grid = None
    pat = None
    cases = CASES
    while args:
        a = args.pop(0)
        if a == "--quick": cases = QUICK
        elif a == "--grid": grid = (int(args.pop(0)), int(args.pop(0)))
        elif a == "--filter": pat = re.compile(args.pop(0))
    bad = 0
    with tempfile.TemporaryDirectory() as work:
        for case in cases:
            name, method = case[:2]
            solv = len(case) > 2
            if pat and not pat.search("%s_%s%s" % (name, method, "_solv" if solv else "")):
                continue
            err, net = check(exe, work, name, method, grid, solv=solv)
            tol = TOL_HF if method.lstrip("ru").lstrip("o") == "hf" else TOL_DFT
            ok = err <= tol and net <= (1e-8 if tol == TOL_HF else 1e-6)  # grid breaks translation in DFT
            bad += not ok
            print("%-6s %-8s %-5s max|analytic - FD| = %.2e  net force %.1e  %s" % (name, method, "solv" if solv else "", err, net, "ok" if ok else "FAIL (tol %.0e)" % tol), flush=True)
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
