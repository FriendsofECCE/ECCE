#!/usr/bin/env python3
"""Regenerates freq_oracle.json: ORCA 6.1.1 TightOpt + Freq of small molecules
(analytic Hessians), from the pushed-off geometries ecce-qm optimises from.

    gen_freq_oracle.py WORKDIR [CASE ...]

Needs ORCA (not part of the tests; freq_test.py reads the JSON).  The
oracle keeps ORCA's final geometry, so that freq_test.py runs ecce-qm at
exactly that geometry; the frequencies, IR intensities and thermochemistry
are those ORCA prints for it.  Restricted open-shell O2 is not here: ORCA has
no ROHF Hessian, freq_test.py checks it against energies instead.
"""
import glob, json, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from gradient_test import pushed

ORCA = glob.glob("/opt/orca/orca_6*/orca")[0]
BOHR = 0.529177210903

CASES = [(m, meth, "r") for m in ("h2o", "nh3", "ch4", "co", "n2", "hf") for meth in ("hf", "b3lyp")]
CASES += [("o2", "hf", "u"), ("o2", "b3lyp", "u")]
KEYWORD = {("hf", "r"): "HF", ("hf", "u"): "UHF", ("b3lyp", "r"): "B3LYP", ("b3lyp", "u"): "UKS B3LYP"}

def case_name(c):
    return "%s_%s_%s" % c

def orca_input(c):
    mol, method, ref = c
    charge, mult, atoms = pushed(mol)
    head = "! %s 6-31G* TightOpt Freq VeryTightSCF NoRI NoAutoStart NoFrozenCore" % KEYWORD[(method, ref)]
    if method != "hf":
        head += " NoCosX DefGrid3"
    s = head + "\n%scf MaxIter 300 end\n%freq QuasiRRHO false end\n* xyz " + "%d %d\n" % (charge, mult)
    for (sym, x, y, z) in atoms:
        s += "%-2s %.10f %.10f %.10f\n" % (sym, x * BOHR, y * BOHR, z * BOHR)
    return s + "*\n"

def last_block(txt, header):
    return txt.split(header)[-1]

def parse(path):
    txt = open(path).read()
    if "ORCA TERMINATED NORMALLY" not in txt or "THE OPTIMIZATION HAS CONVERGED" not in txt:
        raise RuntimeError("no converged optimisation in " + path)
    atoms = []
    for line in last_block(txt, "CARTESIAN COORDINATES (ANGSTROEM)").split("\n")[2:]:
        m = re.match(r"\s*([A-Z][a-z]?)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)", line)
        if not m:
            break
        atoms.append([m.group(1)] + [float(v) for v in m.groups()[1:]])
    vib = last_block(txt, "VIBRATIONAL FREQUENCIES").split("NORMAL MODES")[0]
    freq = [float(f) for f in re.findall(r"^\s*\d+:\s+(-?\d+\.\d+)\s+cm\*\*-1", vib, re.M)]
    ir = last_block(txt, "IR SPECTRUM").split("The first frequency considered")[0]
    inten = {int(m.group(1)): float(m.group(4)) for m in re.finditer(
        r"^\s*(\d+):\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+\(", ir, re.M)}
    def eh(label):
        return float(re.findall(label + r"\s*\.\.\.\s*(-?\d+\.\d+)\s*Eh", txt)[-1])
    def eh2(label):
        return float(re.findall(label + r"\s+(?:\.\.\.\s+)?(-?\d+\.\d+)\s*Eh", txt)[-1])
    ntr = 5 if (len(atoms) == 2) else 6 if len(atoms) > 2 else 3
    return dict(
        atoms=atoms,
        energy=float(re.findall(r"FINAL SINGLE POINT ENERGY\s+(-?\d+\.\d+)", txt)[-1]),
        freq=freq[ntr:], ir=[inten.get(i, 0.0) for i in range(ntr, len(freq))],
        zpe=eh("Zero point energy"), e_thermal=eh2("Total thermal energy"), enthalpy=eh2("Total Enthalpy"),
        ts=eh("Final entropy term"), gibbs=eh("Final Gibbs free energy"),
        symmetry_number=int(re.findall(r"Symmetry Number:\s*(\d+)", txt)[-1]) if "Symmetry Number" in txt else None)

def main():
    work = sys.argv[1]
    only = sys.argv[2:]
    out = {}
    path = os.path.join(HERE, "freq_oracle.json")
    for c in CASES:
        name = case_name(c)
        if only and name not in only:
            continue
        d = os.path.join(work, name)
        os.makedirs(d, exist_ok=True)
        inp, outp = os.path.join(d, "j.inp"), os.path.join(d, "j.out")
        open(inp, "w").write(orca_input(c))
        if not os.path.exists(outp):
            with open(outp, "w") as f:
                subprocess.run([ORCA, inp], cwd=d, stdout=f, stderr=subprocess.STDOUT)
        ent = parse(outp)
        ent.update(molecule=c[0], method=c[1], reference=c[2], basis="6-31G*", program="ORCA", version="6.1.1")
        out[name] = ent
        print(name, ent["energy"], ent["freq"], flush=True)
    if only and os.path.exists(path):
        old = json.load(open(path))
        old.update(out)
        out = old
    json.dump(out, open(path, "w"), indent=1, sort_keys=True)

if __name__ == "__main__":
    main()
