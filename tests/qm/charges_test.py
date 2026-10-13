#!/usr/bin/env python3
"""ecce-qm Loewdin, CHELPG and Merz-Kollman charges against ORCA (Loewdin,
CHELPG) and Gaussian 16 (Pop=MK), live runs on H2O, NH3, CO and HF at
HF/6-31G* and B3LYP/6-31G*.

    charges_test.py ECCE-QM-EXE [WORKDIR]

Loewdin must agree to 1e-3 e.  The fits sample different points in each
program (grid origin, surface generator), so CHELPG and MK are held to 0.02 e.
Exits 77 (skip) if neither ORCA nor g16 is installed.  One core each, one run
at a time.
"""
import glob, os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from molecules import MOLECULES

ORCA = (glob.glob("/opt/orca/orca_6*/orca") or [None])[0]
G16ROOT = "/opt/gaussian"
G16 = os.path.join(G16ROOT, "g16", "g16")
TOL_LOEWDIN, TOL_FIT = 1e-3, 0.02
CASES = [(m, meth) for m in ("h2o", "nh3", "co", "hf") for meth in ("hf", "b3lyp")]


def run_ecceqm(exe, mol, meth, d):
    ch, mult, atoms = MOLECULES[mol]
    inp = os.path.join(d, "q.qmin")
    with open(inp, "w") as f:
        f.write("title %s\ncharge %d\nmultiplicity %d\nmethod %s\nbasis 6-31G*\n"
                "charges loewdin chelpg mk\ngeometry\n" % (mol, ch, mult, meth))
        for a in atoms:
            f.write("%s %.10f %.10f %.10f\n" % a)
        f.write("end\n")
    env = dict(os.environ, OMP_NUM_THREADS="1")
    subprocess.run([exe, "-o", os.path.join(d, "q.qmout"), inp], env=env, check=True,
                   capture_output=True)
    out = {}
    cur = None
    for line in open(os.path.join(d, "q.qmout")):
        t = line.split()
        if line.startswith("begin charges"):
            cur = t[2]
            out[cur] = []
        elif line.startswith("end charges"):
            cur = None
        elif cur:
            out[cur].append(float(t[2]))
    return out


def run_orca(mol, meth, d):
    ch, mult, atoms = MOLECULES[mol]
    kw = "HF" if meth == "hf" else "B3LYP NoRI DefGrid3"
    inp = os.path.join(d, "o.inp")
    with open(inp, "w") as f:
        f.write("! %s 6-31G* VeryTightSCF NoAutoStart NoFrozenCore CHELPG\n"
                "* xyz %d %d\n" % (kw, ch, mult))
        for a in atoms:
            f.write("%-2s %.10f %.10f %.10f\n" % a)
        f.write("*\n")
    txt = subprocess.run([ORCA, inp], cwd=d, capture_output=True, text=True).stdout
    open(os.path.join(d, "o.out"), "w").write(txt)

    def table(head):
        part = txt.split(head)[-1].split("\n\n")[0:2]
        return [float(x) for x in re.findall(r"^\s*\d+\s+[A-Za-z]+\s*:\s*(-?\d+\.\d+)", "\n\n".join(part), re.M)]
    return {"loewdin": table("LOEWDIN ATOMIC CHARGES"), "chelpg": table("CHELPG Charges")}


def run_g16(mol, meth, d):
    ch, mult, atoms = MOLECULES[mol]
    kw = "HF" if meth == "hf" else "B3LYP"
    inp = os.path.join(d, "g.gjf")
    with open(inp, "w") as f:
        f.write("%%nprocshared=1\n%%mem=1GB\n#P %s/6-31G* 5D 7F NoSymm Pop=MK SCF=Tight\n\nt\n\n%d %d\n"
                % (kw, ch, mult))
        for a in atoms:
            f.write("%-2s %.10f %.10f %.10f\n" % a)
        f.write("\n")
    env = dict(os.environ, g16root=G16ROOT, GAUSS_SCRDIR=d)
    with open(inp) as fin, open(os.path.join(d, "g.log"), "w") as fout:
        subprocess.run([G16], stdin=fin, stdout=fout, stderr=subprocess.DEVNULL, env=env, cwd=d)
    txt = open(os.path.join(d, "g.log")).read()
    m = re.search(r"ESP charges:\n\s+1\n(.*?)Sum of ESP", txt, re.S)
    return {"mk": [float(x) for x in re.findall(r"^\s*\d+\s+[A-Za-z]+\s+(-?\d+\.\d+)", m.group(1), re.M)]} if m else {}


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    exe = os.path.abspath(sys.argv[1])
    have_g16 = os.path.exists(G16)
    if not ORCA and not have_g16:
        print("SKIP: neither ORCA nor Gaussian 16 found")
        return 77
    keep = len(sys.argv) > 2
    work = os.path.abspath(sys.argv[2]) if keep else tempfile.mkdtemp(prefix="qmcharges-")
    fails = 0
    try:
        for mol, meth in CASES:
            d = os.path.join(work, "%s_%s" % (mol, meth))
            os.makedirs(d, exist_ok=True)
            q = run_ecceqm(exe, mol, meth, d)
            ref = {}
            if ORCA:
                ref.update(run_orca(mol, meth, d))
            if have_g16:
                ref.update(run_g16(mol, meth, d))
            for scheme, tol, who in (("loewdin", TOL_LOEWDIN, "ORCA"), ("chelpg", TOL_FIT, "ORCA"),
                                     ("mk", TOL_FIT, "g16")):
                if scheme not in ref or len(ref[scheme]) != len(q[scheme]):
                    print("MISSING %s/%s %s from %s" % (mol, meth, scheme, who))
                    fails += 1
                    continue
                dev = max(abs(a - b) for a, b in zip(q[scheme], ref[scheme]))
                ok = dev <= tol
                fails += not ok
                print("%-4s %-4s %-5s %-7s max|dq| %.5f (tol %.3f) vs %s"
                      % ("ok" if ok else "FAIL", mol, meth, scheme, dev, tol, who))
    finally:
        if not keep:
            shutil.rmtree(work, ignore_errors=True)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
