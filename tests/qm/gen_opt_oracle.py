#!/usr/bin/env python3
"""Regenerates opt_oracle.json: ORCA 6.1.1 geometry optimisations of small
molecules, from the same pushed-off starting geometries ecce-qm starts from.

    gen_opt_oracle.py WORKDIR [CASE ...]

Needs ORCA (not part of the tests; opt_test.py reads the JSON).  TightOpt, so
that the ORCA geometry is converged better than ecce-qm's own thresholds.
ORCA 6.1.1 has no ROHF/ROKS gradient, so the restricted open-shell diatomic
is minimised along the bond with ORCA single points (quartic fits).
"""
import glob, json, math, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from gradient_test import pushed

ORCA = glob.glob("/opt/orca/orca_6*/orca")[0]
BOHR = 0.529177210903

# (molecule, method, reference)
CASES = [(m, meth, "r") for m in ("h2o", "nh3", "ch4", "co", "n2", "hf") for meth in ("hf", "b3lyp")]
CASES += [("o2", "hf", "ro"), ("o2", "b3lyp", "ro"), ("o2", "b3lyp", "u")]

KEYWORD = {("hf", "r"): "HF", ("hf", "ro"): "ROHF", ("b3lyp", "r"): "B3LYP",
           ("b3lyp", "ro"): "ROKS B3LYP", ("b3lyp", "u"): "UKS B3LYP"}

def case_name(c):
    return "%s_%s_%s" % c

def orca_input(c, conv, atoms=None):
    mol, method, ref = c
    charge, mult, start = pushed(mol)
    atoms = atoms or start
    head = "! %s 6-31G* %s VeryTightSCF NoRI NoAutoStart NoFrozenCore" % (KEYWORD[(method, ref)], conv)
    if method != "hf":
        head += " NoCosX DefGrid3"
    s = head + "\n%scf MaxIter 300 end\n* xyz " + "%d %d\n" % (charge, mult)
    for (sym, x, y, z) in atoms:
        s += "%-2s %.10f %.10f %.10f\n" % (sym, x * BOHR, y * BOHR, z * BOHR)
    return s + "*\n"

def parse(path):
    txt = open(path).read()
    if "ORCA TERMINATED NORMALLY" not in txt or "THE OPTIMIZATION HAS CONVERGED" not in txt:
        raise RuntimeError("no converged optimisation in " + path)
    e = float(re.findall(r"FINAL SINGLE POINT ENERGY\s+(-?\d+\.\d+)", txt)[-1])
    blocks = txt.split("CARTESIAN COORDINATES (ANGSTROEM)")
    atoms = []
    for line in blocks[-1].split("\n")[2:]:
        m = re.match(r"\s*([A-Z][a-z]?)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)", line)
        if not m:
            break
        atoms.append([m.group(1)] + [float(v) for v in m.groups()[1:]])
    cycles = len(re.findall(r"GEOMETRY OPTIMIZATION CYCLE", txt))
    return dict(energy=e, atoms=atoms, cycles=cycles)

def scan_diatomic(c, work):
    """Bond length minimum from single points, for the cases ORCA cannot optimise."""
    import numpy as np
    charge, mult, start = pushed(c[0])
    sym = [a[0] for a in start]
    r = math.dist([v * BOHR for v in start[0][1:]], [v * BOHR for v in start[1][1:]])
    count = 0
    def single(rr):
        nonlocal count
        d = os.path.join(work, "%s_scan_%.5f" % (case_name(c), rr))
        os.makedirs(d, exist_ok=True)
        inp, outp = os.path.join(d, "j.inp"), os.path.join(d, "j.out")
        atoms = [(sym[0], 0.0, 0.0, 0.0), (sym[1], 0.0, 0.0, rr / BOHR)]
        open(inp, "w").write(orca_input(c, "", atoms))
        if not os.path.exists(outp):
            with open(outp, "w") as f:
                subprocess.run([ORCA, inp], cwd=d, stdout=f, stderr=subprocess.STDOUT)
        count += 1
        return float(re.findall(r"FINAL SINGLE POINT ENERGY\s+(-?\d+\.\d+)", open(outp).read())[-1])
    for _ in range(6):
        xs = r + 0.02 * np.arange(-3, 4)
        ys = np.array([single(x) for x in xs])
        p = np.polyfit(xs - r, ys, 4)
        grid = np.linspace(-0.06, 0.06, 24001)
        step = grid[np.argmin(np.polyval(p, grid))]
        r += step
        if abs(step) < 2e-5:
            break
    e = float(np.polyval(p, step))
    res = dict(energy=e, atoms=[[sym[0], 0.0, 0.0, 0.0], [sym[1], 0.0, 0.0, r]], cycles=count)
    return {"opt": res, "tightopt": res}

def main():
    work = sys.argv[1]
    only = sys.argv[2:]
    out = {}
    for c in CASES:
        name = case_name(c)
        if only and name not in only:
            continue
        ent = scan_diatomic(c, work) if c[2] == "ro" else {}
        for conv in (() if c[2] == "ro" else ("Opt", "TightOpt")):
            d = os.path.join(work, name + "_" + conv)
            os.makedirs(d, exist_ok=True)
            inp, outp = os.path.join(d, "j.inp"), os.path.join(d, "j.out")
            open(inp, "w").write(orca_input(c, conv))
            if not os.path.exists(outp):
                with open(outp, "w") as f:
                    subprocess.run([ORCA, inp], cwd=d, stdout=f, stderr=subprocess.STDOUT)
            ent[conv.lower()] = parse(outp)
        ent.update(molecule=c[0], method=c[1], reference=c[2], basis="6-31G*",
                   program="ORCA", version="6.1.1")
        out[name] = ent
        print(name, ent["tightopt"]["energy"], ent["opt"]["cycles"], ent["tightopt"]["cycles"], flush=True)
    path = os.path.join(HERE, "opt_oracle.json")
    if only and os.path.exists(path):
        old = json.load(open(path))
        old.update(out)
        out = old
    json.dump(out, open(path, "w"), indent=1, sort_keys=True)

if __name__ == "__main__":
    main()
