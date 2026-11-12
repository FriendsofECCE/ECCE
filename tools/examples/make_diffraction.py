#!/usr/bin/env python3
"""Inputs and outputs of the Diffraction topic of General Chemistry (#972).

    tools/examples/make_diffraction.py inputs          write the inputs
    tools/examples/make_diffraction.py run HOST        run them on HOST (ssh)

KCl and quartz are pw.x single points with the settings of
packing-in-solids/qe-nacl (PBE, 50 Ry, marzari-vanderbilt smearing 0.02);
quartz takes its cell from tests/fixtures/cif/quartz.cif expanded by
ecce_xrd.read_cif.  K, O and Si have no GBRV file in this tree, so they use the
pslibrary files of /usr/share/espresso/pseudo with ecutrho 400 (PAW).  NaCl is
the Packing in solids output, copied.  B-DNA and the alpha-helix are xtb GFN-FF
single points of the Teaching/Fibres samples.  The parsed copies in stored/ come
from tools/examples/preparse.py.
"""
import os
import shutil
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import ecce_xrd as xrd  # noqa: E402

TOPIC = os.path.join(ROOT, "data", "client", "examples", "genchem", "diffraction")
PK = os.path.join(ROOT, "data", "client", "examples", "genchem", "packing-in-solids")
FIB = os.path.join(ROOT, "data", "client", "StructureLibrary", "Teaching", "Fibres")
PSEUDO = "/usr/share/espresso/pseudo"
MASS = {"K": 39.0983, "Cl": 35.453, "Si": 28.0855, "O": 15.999}
UPF = {"K": "K.pbe-spn-kjpaw_psl.1.0.0.UPF", "Cl": "cl_pbe_v1.4.uspp.F.UPF",
       "Si": "Si.pbe-n-rrkjus_psl.1.0.0.UPF", "O": "O.pbe-n-kjpaw_psl.0.1.UPF"}


def pwin(name, lat, sym, frac, kpts, ecutrho):
    els = sorted(set(sym), key=sym.index)
    out = ["&CONTROL", "  calculation = 'scf'", "  prefix = '%s'" % name,
           "  outdir = './qe.scratch'", "  pseudo_dir = '%s/'" % PSEUDO, "  tstress = .true.", "/",
           "&SYSTEM", "  ibrav = 0", "  nat = %d" % len(sym), "  ntyp = %d" % len(els),
           "  ecutwfc = 50", "  ecutrho = %d" % ecutrho, "  occupations = 'smearing'",
           "  smearing = 'marzari-vanderbilt'", "  degauss = 0.02", "/",
           "&ELECTRONS", "  conv_thr = 1e-08", "  mixing_beta = 0.7", "/", "ATOMIC_SPECIES"]
    out += ["  %s  %g  %s" % (e, MASS[e], UPF[e]) for e in els]
    out.append("CELL_PARAMETERS angstrom")
    out += ["  %.10f %.10f %.10f" % tuple(v) for v in lat]
    out.append("ATOMIC_POSITIONS angstrom")
    for s, p in zip(sym, frac @ lat):
        out.append("  %s %.10f %.10f %.10f" % ((s,) + tuple(p)))
    out += ["K_POINTS automatic", "  %d %d %d 0 0 0" % kpts]
    return "\n".join(out) + "\n"


def mvm_xyz(path, out, title):
    atoms, on, n = [], False, 0
    for ln in open(path):
        if ln.startswith("num_atoms:"):
            n = int(ln.split()[1])
        elif on and len(atoms) == n:
            break
        elif ln.startswith("atom_list:"):
            on = True
        elif on and ln.strip():
            w = ln.split()
            atoms.append((w[0], w[1:4]))
    with open(out, "w") as f:
        f.write("%d\n%s\n" % (len(atoms), title))
        for s, p in atoms:
            f.write("%-2s %s %s %s\n" % ((s,) + tuple(p)))


def inputs():
    for d in ("qe-nacl_powder", "qe-nacl_bragg", "qe-kcl", "qe-quartz", "xtb-b_dna", "xtb-alpha_helix"):
        os.makedirs(os.path.join(TOPIC, d), exist_ok=True)
    src = os.path.join(PK, "qe-nacl")   # the files are copied under the new calculation names
    for d, n in (("qe-nacl_powder", "nacl_powder"), ("qe-nacl_bragg", "nacl_bragg")):
        for ext in ("pwin", "pwout"):
            shutil.copy(os.path.join(src, "nacl." + ext), os.path.join(TOPIC, d, n + "." + ext))
    lat, sym, frac, occ = xrd.read_cif(os.path.join(ROOT, "tests", "fixtures", "cif", "kcl.cif"))
    open(os.path.join(TOPIC, "qe-kcl", "kcl.pwin"), "w").write(pwin("kcl", lat, sym, frac, (6, 6, 6), 400))
    lat, sym, frac, occ = xrd.read_cif(os.path.join(ROOT, "tests", "fixtures", "cif", "quartz.cif"))
    open(os.path.join(TOPIC, "qe-quartz", "quartz.pwin"), "w").write(pwin("quartz", lat, sym, frac, (4, 4, 4), 400))
    mvm_xyz(os.path.join(FIB, "B-DNA_20bp.mvm"), os.path.join(TOPIC, "xtb-b_dna", "bdna.xyz"),
            "B-DNA, 20 bp poly(dA).poly(dT)")
    mvm_xyz(os.path.join(FIB, "Alpha-helix_polyAla.mvm"), os.path.join(TOPIC, "xtb-alpha_helix", "helix.xyz"),
            "alpha-helix, polyalanine")


def run(host):
    work = "ecce-offload/tmp/diffraction"
    sh = lambda c: subprocess.run(["ssh", "-o", "BatchMode=yes", host, c], check=True)
    sh("rm -rf ~/%s && mkdir -p ~/%s/pseudo" % (work, work))
    subprocess.run(["scp", "-q"] + [os.path.join(PSEUDO, u) for u in UPF.values()] +
                   [host + ":" + work + "/pseudo/"], check=True)
    jobs = [("qe-kcl", "kcl.pwin", "kcl.pwout"), ("qe-quartz", "quartz.pwin", "quartz.pwout")]
    for d, i, o in jobs:
        subprocess.run(["scp", "-q", os.path.join(TOPIC, d, i), host + ":" + work + "/"], check=True)
        sh("cd ~/%s && sed -i \"s|pseudo_dir = .*|pseudo_dir = '$HOME/%s/pseudo/'|\" %s && "
           "mpirun -np 4 pw.x -in %s > %s" % (work, work, i, i, o))
        subprocess.run(["scp", "-q", host + ":" + work + "/" + o, os.path.join(TOPIC, d, o)], check=True)
    for d, x, o in (("xtb-b_dna", "bdna", "bdna_gfnff.out"), ("xtb-alpha_helix", "helix", "helix_gfnff.out")):
        subprocess.run(["scp", "-q", os.path.join(TOPIC, d, x + ".xyz"), host + ":" + work + "/"], check=True)
        sh("cd ~/%s && xtb %s.xyz --gfnff --chrg 0 > %s 2>&1" % (work, x, o))
        subprocess.run(["scp", "-q", host + ":" + work + "/" + o, os.path.join(TOPIC, d, o)], check=True)


if __name__ == "__main__":
    if sys.argv[1:2] == ["inputs"]:
        inputs()
    elif sys.argv[1:2] == ["run"] and len(sys.argv) == 3:
        run(sys.argv[2])
    else:
        sys.exit(__doc__)
