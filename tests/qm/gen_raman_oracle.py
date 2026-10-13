#!/usr/bin/env python3
"""Regenerates polar_oracle.json and raman_oracle.json: ORCA 6.1.1 static
polarisabilities (%elprop Polar 1, analytic CP-SCF) and Raman activities of
small molecules at HF and B3LYP/6-31G*.

    gen_raman_oracle.py WORKDIR [CASE ...]

Needs ORCA (not part of the tests).  Both oracles use the geometries of
freq_oracle.json, which are ORCA's own TightOpt results, so ecce-qm runs at
exactly the point ORCA differentiated.  The polarisabilities also carry
NWChem's (response module, CPKS); NWChem's Raman driver does not run on these
inputs, so the Raman oracle is ORCA only.  ORCA computes Raman activities from
analytic Hessians only for HF; B3LYP needs NumFreq.  Run names: polar_<mol>_<method>,
raman_<mol>_<method>.
"""
import glob, json, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ORCA = glob.glob("/opt/orca/orca_6*/orca")[0]

POLAR = [(m, meth) for m in ("h2o", "nh3", "co", "hf") for meth in ("hf", "b3lyp")]
RAMAN = [(m, meth) for m in ("h2o", "nh3", "ch4") for meth in ("hf", "b3lyp")]
KEYWORD = {"hf": "HF", "b3lyp": "B3LYP NoRI NoCosX DefGrid3"}


def geometry(mol, method):
    ent = json.load(open(os.path.join(HERE, "freq_oracle.json")))["%s_%s_r" % (mol, method)]
    return ent["atoms"]


def orca_input(kind, mol, method):
    head = "! %s 6-31G* VeryTightSCF NoAutoStart NoFrozenCore" % KEYWORD[method]
    if kind == "raman":
        head += " Freq" if method == "hf" else " NumFreq"
    s = head + "\n%scf MaxIter 300 end\n%elprop Polar 1 end\n"
    if kind == "raman":
        s += "%freq QuasiRRHO false end\n"
    s += "%pal nprocs 2 end\n"
    s += "* xyz 0 1\n"
    for a in geometry(mol, method):
        s += "%-2s %.8f %.8f %.8f\n" % tuple(a)
    return s + "*\n"


def polar_tensor(txt):
    block = txt.split("The raw cartesian tensor (atomic units):")[-1].splitlines()[1:4]
    return [[float(x) for x in row.split()] for row in block]


def raman_modes(txt):
    block = txt.split("RAMAN SPECTRUM")[-1].split("The first frequency considered")[0]
    modes = [(float(m.group(2)), float(m.group(3)), float(m.group(4))) for m in re.finditer(
        r"^\s*(\d+):\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)", block, re.M)]
    return dict(freq=[m[0] for m in modes], activity=[m[1] for m in modes], depol=[m[2] for m in modes])


NWCHEM = "/usr/bin/nwchem"
NW_XC = "vwn_5 0.19 lyp 0.81 hfexch 0.20 slater 0.80 becke88 nonlocal 0.72"


def nwchem_polar(work, name, mol, method):
    """NWChem's static polarisability (response module, analytic CPKS)."""
    d = os.path.join(work, name + "_nw")
    os.makedirs(d, exist_ok=True)
    s = "start nw\necho\nmemory 2000 mb\ngeometry units angstrom noautosym nocenter noautoz\n"
    for a in geometry(mol, method):
        s += "%-2s %.8f %.8f %.8f\n" % tuple(a)
    s += "end\nbasis spherical\n * library 6-31g*\nend\n"
    if method == "hf":
        s += "scf\n rhf\n thresh 1e-9\n tol2e 1e-12\n maxiter 200\n singlet\nend\n"
    else:
        s += "dft\n xc %s\n grid xfine\n convergence density 1e-8 energy 1e-9\n maxiter 200\nend\n" % NW_XC
    s += "property\n response 1 0.0\nend\ntask %s property\n" % ("scf" if method == "hf" else "dft")
    inp, outp = os.path.join(d, "nw.nw"), os.path.join(d, "nw.out")
    open(inp, "w").write(s)
    if not os.path.exists(outp):
        with open(outp, "w") as f:
            subprocess.run(["mpirun", "-np", "2", NWCHEM, inp], cwd=d, stdout=f, stderr=subprocess.STDOUT)
    txt = open(outp).read()
    m = re.search(r"polarizability / au\s+Frequency\s+=\s+0\.0+ / au\s+Wavelength.*?\n.*?\n.*?\n"
                  r"\s*X\s+(\S+)\s+(\S+)\s+(\S+)\n\s*Y\s+(\S+)\s+(\S+)\s+(\S+)\n\s*Z\s+(\S+)\s+(\S+)\s+(\S+)\n", txt)
    if not m:
        raise RuntimeError("no NWChem polarizability in " + outp)
    v = [float(x) for x in m.groups()]
    return dict(program="NWChem", version=re.findall(r"nwchem branch\s+=\s+(\S+)", txt)[0],
                alpha=[v[0:3], v[3:6], v[6:9]])


def run(work, name, kind, mol, method):
    d = os.path.join(work, name)
    os.makedirs(d, exist_ok=True)
    inp, outp = os.path.join(d, "j.inp"), os.path.join(d, "j.out")
    open(inp, "w").write(orca_input(kind, mol, method))
    if not os.path.exists(outp):
        with open(outp, "w") as f:
            subprocess.run([ORCA, inp], cwd=d, stdout=f, stderr=subprocess.STDOUT)
    txt = open(outp).read()
    if "ORCA TERMINATED NORMALLY" not in txt:
        raise RuntimeError("ORCA failed: " + outp)
    return txt


def main():
    work = os.path.abspath(sys.argv[1])
    only = sys.argv[2:]
    for kind, cases, fname in (("polar", POLAR, "polar_oracle.json"), ("raman", RAMAN, "raman_oracle.json")):
        path = os.path.join(HERE, fname)
        out = json.load(open(path)) if os.path.exists(path) else {}
        for mol, method in cases:
            name = "%s_%s_%s" % (kind, mol, method)
            if only and name not in only:
                continue
            txt = run(work, name, kind, mol, method)
            ent = dict(molecule=mol, method=method, basis="6-31G*", atoms=geometry(mol, method),
                       program="ORCA", version="6.1.1",
                       keywords=orca_input(kind, mol, method).split("\n")[0])
            if kind == "polar":
                ent["alpha"] = polar_tensor(txt)
                ent["nwchem"] = nwchem_polar(work, name, mol, method)
            else:
                ent.update(raman_modes(txt))
            out[name] = ent
            print(name, ent.get("alpha") or ent["activity"], flush=True)
        json.dump(out, open(path, "w"), indent=1, sort_keys=True)


if __name__ == "__main__":
    main()
