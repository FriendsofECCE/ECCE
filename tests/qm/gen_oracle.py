#!/usr/bin/env python3
"""Run the oracle programs and write oracle.json.  Developer tool; the test
(run_tests.py) reads the JSON and does not need ORCA/NWChem.

    gen_oracle.py WORKDIR [--orca-only] [CASE ...]

Records, per case, the program, version, input keywords and the numbers
parsed from the program's output (total energy; orbital energies per spin,
Eh).  Existing entries are kept unless re-run.
"""
import json, os, re, subprocess, sys, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from molecules import MOLECULES

HERE = os.path.dirname(os.path.abspath(__file__))
ORCA = glob.glob("/opt/orca/orca_6*/orca")[0]
ORCA_VERSION = "6.1.1"
NWCHEM = "/usr/bin/nwchem"

# (molecule, basis, method, reference) ; reference: r, u, ro
CASES = []
for mol in ("h2o", "nh3", "co", "ch4", "n2", "benzene"):
    for basis in ("STO-3G", "6-31G*", "def2-SVP"):
        for method in ("hf", "b3lyp", "pbe"):
            CASES.append((mol, basis, method, "r"))
for mol, ref in (("c2", "r"), ("o2", "ro"), ("he2", "r"), ("hf", "r"), ("co", "r")):
    for basis in ("6-31G*", "def2-SVP"):
        CASES.append((mol, basis, "b3lyp", ref))
CASES += [("o2", "def2-SVP", "hf", "ro"), ("o2", "def2-SVP", "hf", "u"), ("o2", "def2-SVP", "b3lyp", "u"),
          ("h2o", "def2-SVP", "svwn", "r"), ("h2o", "def2-SVP", "pbe0", "r")]

def case_name(c):
    return "%s_%s_%s_%s" % (c[0], c[1].replace("*", "s").lower(), c[2], c[3])

ORCA_METHOD = {("hf", "r"): "HF", ("hf", "u"): "UHF", ("hf", "ro"): "ROHF",
               ("b3lyp", "r"): "B3LYP", ("b3lyp", "u"): "UKS B3LYP", ("b3lyp", "ro"): "ROKS B3LYP",
               ("pbe", "r"): "PBE", ("pbe0", "r"): "PBE0", ("svwn", "r"): "LDA"}

def orca_input(c):
    mol, basis, method, ref = c
    ch, mult, atoms = MOLECULES[mol]
    kw = ORCA_METHOD[(method, ref)]
    head = "! %s %s VeryTightSCF NoRI NoCosX NoAutoStart NoFrozenCore DefGrid3" % (kw, basis)
    if method == "hf":
        head = "! %s %s VeryTightSCF NoRI NoAutoStart NoFrozenCore" % (kw, basis)
    s = head + "\n%%scf MaxIter 300 end\n* xyz %d %d\n" % (ch, mult)
    for a in atoms:
        s += "%-2s %.10f %.10f %.10f\n" % a
    return s + "*\n"

def parse_orca(path):
    txt = open(path).read()
    m = re.findall(r"FINAL SINGLE POINT ENERGY\s+(-?\d+\.\d+)", txt)
    if not m:
        raise RuntimeError("no energy in " + path)
    out = {"energy": float(m[-1]), "orbitals": {}}
    # last ORBITAL ENERGIES section
    parts = txt.split("ORBITAL ENERGIES")
    sec = parts[-1].split("\n\n\n")[0] if len(parts) > 1 else ""
    # sections for UHF: "SPIN UP ORBITALS" / "SPIN DOWN ORBITALS"
    blocks = re.split(r"SPIN (UP|DOWN) ORBITALS", sec)
    def rows(t):
        return [(float(a), float(b)) for a, b in re.findall(r"^\s*\d+\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+-?\d+\.\d+\s*$", t, re.M)]
    if len(blocks) > 1:
        out["orbitals"]["alpha"] = rows(blocks[2])
        out["orbitals"]["beta"] = rows(blocks[4])
    else:
        out["orbitals"]["restricted"] = rows(sec)
    return out

def run_orca(c, work):
    name = case_name(c)
    d = os.path.join(work, name)
    os.makedirs(d, exist_ok=True)
    inp = os.path.join(d, name + ".inp")
    outp = os.path.join(d, name + ".out")
    open(inp, "w").write(orca_input(c))
    if not os.path.exists(outp) or "ORCA TERMINATED NORMALLY" not in open(outp).read():
        with open(outp, "w") as f:
            subprocess.run([ORCA, inp], cwd=d, stdout=f, stderr=subprocess.STDOUT, check=False)
    res = parse_orca(outp)
    res.update(program="ORCA", version=ORCA_VERSION, keywords=orca_input(c).splitlines()[0][2:])
    return res

NW_BASIS = {"STO-3G": "sto-3g", "6-31G*": "6-31g*", "def2-SVP": "def2-svp"}

def nwchem_input(c, name):
    mol, basis, method, ref = c
    ch, mult, atoms = MOLECULES[mol]
    s = 'start %s\necho\nmemory 2000 mb\ngeometry units angstrom noautosym nocenter noautoz\n' % name
    for a in atoms:
        s += "%-2s %.10f %.10f %.10f\n" % a
    s += "end\nbasis spherical\n * library %s\nend\ncharge %d\n" % (NW_BASIS[basis], ch)
    if method == "hf":
        s += "scf\n rhf\n thresh 1e-9\n tol2e 1e-12\n maxiter 200\n singlet\nend\ntask scf energy\n"
    else:
        xc = {"b3lyp": "vwn_5 0.19 lyp 0.81 hfexch 0.20 slater 0.80 becke88 nonlocal 0.72",
              "pbe": "xpbe96 cpbe96", "pbe0": "pbe0", "svwn": "slater vwn_5"}[method]
        s += "dft\n xc %s\n grid xfine\n convergence density 1e-8 energy 1e-9\n maxiter 200\nend\ntask dft energy\n" % xc
    return s

def run_nwchem(c, work):
    name = case_name(c) + "_nw"
    d = os.path.join(work, name)
    os.makedirs(d, exist_ok=True)
    inp = os.path.join(d, name + ".nw")
    outp = os.path.join(d, name + ".out")
    open(inp, "w").write(nwchem_input(c, name))
    if not os.path.exists(outp):
        with open(outp, "w") as f:
            subprocess.run([NWCHEM, inp], cwd=d, stdout=f, stderr=subprocess.STDOUT, check=False)
    txt = open(outp).read()
    m = re.findall(r"Total (?:DFT|SCF) energy =\s+(-?\d+\.\d+)", txt)
    if not m:
        raise RuntimeError("no NWChem energy in " + outp)
    return {"program": "NWChem", "version": re.findall(r"nwchem branch\s+=\s+(\S+)", txt)[0] if "nwchem branch" in txt else "?",
            "keywords": "dft grid xfine, spherical basis" if method_is_dft(c) else "scf rhf thresh 1e-9, spherical basis",
            "energy": float(m[-1])}

def run_g16(c, work):
    """Gaussian 16 PBE/def2-SVP water: a third opinion on the PBE definition
    (recorded for information; the test does not require agreement)."""
    mol, basis, method, ref = c
    ch, mult, atoms = MOLECULES[mol]
    name = case_name(c) + "_g16"
    d = os.path.join(work, name)
    os.makedirs(d, exist_ok=True)
    com = os.path.join(d, name + ".com")
    with open(com, "w") as f:
        f.write("%nprocshared=4\n#p PBEPBE/def2SVP int=ultrafine scf=(tight,maxcycle=200) 5D 7F nosymm\n\n" + name + "\n\n%d %d\n" % (ch, mult))
        for a in atoms:
            f.write("%-2s %.10f %.10f %.10f\n" % a)
        f.write("\n")
    env = dict(os.environ, g16root="/opt/gaussian", GAUSS_SCRDIR=d)
    log = os.path.join(d, name + ".log")
    if not os.path.exists(log):
        subprocess.run(["/opt/gaussian/g16/g16", com], cwd=d, env=env, capture_output=True)
    m = re.findall(r"SCF Done:.*=\s+(-?\d+\.\d+)\s+A\.U\.", open(log).read())
    return {"program": "Gaussian", "version": "16 C.01", "keywords": "PBEPBE/def2SVP int=ultrafine scf=tight 5D 7F",
            "energy": float(m[-1])}

def method_is_dft(c):
    return c[2] != "hf"

def main():
    args = sys.argv[1:]
    work = args.pop(0)
    do_nw = "--orca-only" not in args
    sel = [a for a in args if not a.startswith("--")]
    path = os.path.join(HERE, "oracle.json")
    data = json.load(open(path)) if os.path.exists(path) else {}
    for c in CASES:
        name = case_name(c)
        if sel and name not in sel:
            continue
        ch, mult, atoms = MOLECULES[c[0]]
        ent = data.get(name, {})
        ent.update(molecule=c[0], basis=c[1], method=c[2], reference=c[3], charge=ch, multiplicity=mult,
                   atoms=[list(a) for a in atoms])
        ent["orca"] = run_orca(c, work)
        if do_nw and c[3] == "r" and ((c[0] in ("h2o", "co", "nh3") and c[1] != "STO-3G") or (c[0] == "benzene" and c[1] == "def2-SVP")):
            ent["nwchem"] = run_nwchem(c, work)
        if do_nw and name == "h2o_def2-svp_pbe_r":
            ent["gaussian"] = run_g16(c, work)
        data[name] = ent
        print(name, ent["orca"]["energy"], flush=True)
        json.dump(data, open(path, "w"), indent=1, sort_keys=True)

main()
