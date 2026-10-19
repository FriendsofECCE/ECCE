#!/usr/bin/env python3
"""ClayFF for GROMACS: third_party/clayff/clayff.ff and `minff_topology.py
--forcefield clayff`.

Quick mode (default, for CI):
  * every Lennard-Jones parameter and charge in clayff.ff equals Table 1 of
    Cygan, Liang and Kalinichev (J. Phys. Chem. B 2004, 108, 1255) converted
    here, and the sigma of mgh, ao, oh and o* agree with the sigma that
    Pouvreau et al. (J. Phys. Chem. C 2017, 121, 14757, Table 2) print for the
    same parameters (a second, independently typeset source);
  * brucite, gibbsite and kaolinite supercells build, grompp accepts them
    with no switch, -DCLAYFF_MOH and -DCLAYFF_MOH_2004, are neutral, and a
    short minimisation lowers a finite energy;
  * the flexible SPC water and the ions of clayff.ff grompp and minimise.
--long adds NPT runs (anisotropic Berendsen, 300 K, 1 bar) of brucite
  and gibbsite and compares the cell with the "ClayFF-orig" column of
  Pouvreau et al. Table 3, and of boehmite, portlandite, kaolinite and
  pyrophyllite against Table 3 of Cygan et al. 2004 (that paper's own
  simulations).  Tolerances are fixed in advance: lengths 1.5 % (portlandite
  3 %, its cell is soft), angles 3 degrees.

Needs atomipy (bundled), gmx and, for the example cells, the minff repository
download that tests/minff uses (~/.cache/ecce/minff).  Exit 77 (CTest SKIP)
without one of them.  Threads: gmx never gets more than 4.
"""
import argparse
import math
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SCRIPT = os.path.join(ROOT, "scripts", "minff_topology.py")
FF = os.path.join(ROOT, "third_party", "clayff", "clayff.ff")
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import minff_topology as mt  # noqa: E402

KCAL = 4.184
# Table 1, Cygan et al. 2004, as printed: (atomipy type, symbol, q, D0 kcal/mol, R0 A)
TABLE1 = [
    ("H", "ho", 0.4250, None, None), ("Hw", "h*", 0.4100, None, None),
    ("Ow", "o*", -0.8200, 0.1554, 3.5532), ("Oh", "oh", -0.9500, 0.1554, 3.5532),
    ("Ob", "ob", -1.0500, 0.1554, 3.5532), ("Omg", "obos", -1.1808, 0.1554, 3.5532),
    ("Oalt", "obts", -1.1688, 0.1554, 3.5532), ("Odsub", "obss", -1.2996, 0.1554, 3.5532),
    ("Ohmg", "ohs", -1.0808, 0.1554, 3.5532),
    ("Sit", "st", 2.1000, 1.8405e-6, 3.7064), ("Alo", "ao", 1.5750, 1.3298e-6, 4.7943),
    ("Alt", "at", 1.5750, 1.8405e-6, 3.7064), ("Mgo", "mgo", 1.3600, 9.0298e-7, 5.9090),
    ("Mgh", "mgh", 1.0500, 9.0298e-7, 5.9090),
    ("Cao", "cao", 1.3600, 5.0298e-6, 6.2484), ("Cah", "cah", 1.0500, 5.0298e-6, 6.2428),
    ("Feo", "feo", 1.5750, 9.0298e-6, 5.5070), ("Lio", "lio", 0.5250, 9.0298e-6, 4.7257),
    ("Na", "Na", 1.0, 0.1301, 2.6378), ("K", "K", 1.0, 0.1000, 3.7423),
    ("Cs", "Cs", 1.0, 0.1000, 4.3002), ("Ca", "Ca", 2.0, 0.1000, 3.2237),
    ("Ba", "Ba", 2.0, 0.0470, 4.2840), ("Cl", "Cl", -1.0, 0.1001, 4.9388),
]
# Pouvreau et al. 2017 Table 2: sigma in A of the same parameters
POUVREAU_SIGMA = {"Mgh": 5.2643, "Alo": 4.2718, "Oh": 3.1655, "Ow": 3.1655}

# Reference cells, per rescaling to the unit cell. Pouvreau et al. 2017, Table 3,
# ClayFF-orig; the supercell of the run is chosen here.
POUVREAU_ORIG = {
    "brucite": dict(a=3.27, c=4.74, V=43.78),
    "gibbsite": dict(a=8.87, b=5.13, c=9.81, beta=100.6, V=439.0),
}
# Cygan et al. 2004, Table 3, "simulation" column
CYGAN_2004 = {
    "boehmite": dict(a=2.980, b=12.442, c=3.732, alpha=90.01, beta=90.00, gamma=90.04),
    "portlandite": dict(a=3.693, b=3.694, c=4.796, alpha=82.04, beta=97.99, gamma=121.59),
    "kaolinite": dict(a=5.190, b=8.956, c=7.361, alpha=90.77, beta=104.17, gamma=90.40),
    "pyrophyllite": dict(a=5.192, b=9.021, c=9.459, alpha=91.38, beta=98.97, gamma=89.81),
}
CELLS = {   # name: (file in Systems/UC_conf, replication)
    "brucite": ("Brucite_GII_0.0027.pdb", (9, 9, 6)),
    "gibbsite": ("Gibbsite_GII_0.0928.pdb", (3, 5, 3)),
    "kaolinite": ("Kaolinite_GII_0.0487.pdb", (5, 3, 4)),
    "boehmite": ("Boehmite_GII_0.0325.pdb", (9, 2, 7)),
    "portlandite": ("Portlandite_GII_0.012.pdb", (8, 8, 6)),
    "pyrophyllite": ("Pyrophyllite_Lee_Guggenheim_1981_GII_0.071.pdb", (5, 3, 3)),
}
THREADS = ["-ntmpi", "1", "-ntomp", "4"]

failures = []


def check(cond, msg):
    print(("ok   " if cond else "FAIL ") + msg, flush=True)
    if not cond:
        failures.append(msg)


def run(cmd, **kw):
    return subprocess.run(["nice", "-n", "19"] + cmd, capture_output=True, text=True, **kw)


def read_atomtypes():
    out, sec = {}, None
    for line in open(os.path.join(FF, "ffnonbonded.itp")):
        s = line.split(";")[0].strip()
        if s.startswith("["):
            sec = s.strip("[] ")
        elif s and sec == "atomtypes":
            p = s.split()
            out[p[0]] = dict(q=float(p[3]), sigma=float(p[5]), eps=float(p[6]))
    return out


def test_parameters():
    got = read_atomtypes()
    for name, sym, q, d0, r0 in TABLE1:
        g = got.get(name)
        if not g:
            check(False, "%s (%s) is in clayff.ff" % (name, sym))
            continue
        sg = 0.0 if d0 is None else r0 / 2 ** (1 / 6) / 10
        ep = 0.0 if d0 is None else d0 * KCAL
        ok = (abs(g["q"] - q) < 1e-9 and abs(g["sigma"] - sg) < 6e-8 and
              abs(g["eps"] - ep) <= 1e-6 * max(ep, 1e-9))
        check(ok, "%s (%s): q %.4f sigma %.6f nm eps %.4e kJ/mol follow Table 1" % (
            name, sym, g["q"], g["sigma"], g["eps"]))
    for name, sig in POUVREAU_SIGMA.items():
        check(abs(got[name]["sigma"] * 10 - sig) < 1e-3,
              "%s: sigma %.4f A equals Pouvreau Table 2 (%.4f)" % (name, got[name]["sigma"] * 10, sig))
    # Op and O are ob under atomipy's names
    for alias in ("Op", "O"):
        check(got[alias]["sigma"] == got["Ob"]["sigma"] and got[alias]["eps"] == got["Ob"]["eps"] and
              got[alias]["q"] == got["Ob"]["q"], alias + " carries the ob parameters")
    # printed values, kept as printed
    check(abs(got["Cao"]["sigma"] * 10 * 2 ** (1 / 6) - 6.2484) < 1e-4 and
          abs(got["Cah"]["sigma"] * 10 * 2 ** (1 / 6) - 6.2428) < 1e-4,
          "cao/cah R0 6.2484/6.2428 kept as printed")
    text = open(os.path.join(FF, "ffbonded.itp")).read()
    check("463700" in text and "383.00" in text and "251.04" in text and "125.52" in text,
          "bond and angle force constants are 2 k of Tables 2 (Cygan) and Pouvreau")


EM_MDP = """integrator = steep
nsteps = 300
emtol = 10
emstep = 0.0005
cutoff-scheme = verlet
coulombtype = PME
rcoulomb = 0.7
rvdw = 0.7
DispCorr = EnerPres
pbc = xyz
periodic-molecules = yes
nstenergy = 10
nstlog = 10
define = %s
"""


def grompp(out, mdp, label, extra=(), maxwarn=0):
    r = run(["gmx", "grompp", "-f", mdp, "-c", extra[0] if extra else "conf.gro", "-p", "topol.top",
             "-o", "run.tpr", "-maxwarn", str(maxwarn), "-po", "mdout.mdp"], cwd=out)
    check(r.returncode == 0, label + ": grompp")
    if r.returncode:
        print(r.stderr[-1500:])
    return r.returncode == 0


def minimise(out, label, define="", gro=None):
    with open(os.path.join(out, "em.mdp"), "w") as f:
        f.write(EM_MDP % define)
    if not grompp(out, "em.mdp", label, [gro] if gro else (), 0):
        return None
    r = run(["gmx", "mdrun", "-deffnm", "run", "-ntmpi", "1", "-ntomp", "2"], cwd=out)
    check(r.returncode == 0, label + ": mdrun")
    if r.returncode:
        print(r.stderr[-800:])
        return None
    r = run(["gmx", "energy", "-f", "run.edr", "-o", "pe.xvg"], cwd=out, input="Potential\n0\n")
    pe = [float(l.split()[1]) for l in open(os.path.join(out, "pe.xvg")) if l.strip() and l[0] not in "#@"]
    check(len(pe) > 2 and all(map(math.isfinite, pe)) and pe[-1] < pe[0],
          label + ": energy finite and decreasing %.4g -> %.4g kJ/mol" % (pe[0], pe[-1]))
    return pe


def build(cell, out, rep):
    r = run([sys.executable, SCRIPT, "build", cell, "-o", out, "--forcefield", "clayff",
             "--replicate", *map(str, rep), "--gui"], env=dict(os.environ, ECCE_MINFF_REEXEC="1",
                                                                PYTHONPATH=mt._atomipy_env()["PYTHONPATH"]))
    check(r.returncode == 0, "build %s %s" % (os.path.basename(cell), "x".join(map(str, rep))))
    if r.returncode:
        print(r.stderr[-1500:])
        return None
    import json
    return json.load(open(os.path.join(out, "minff.json")))


def test_minerals(uc, work):
    for name in ("brucite", "gibbsite", "kaolinite"):
        f, rep = CELLS[name]
        out = os.path.join(work, name)
        s = build(os.path.join(uc, f), out, (6, 6, 4) if name == "brucite" else
                  (2, 3, 2) if name == "gibbsite" else (3, 2, 2))
        if not s:
            continue
        check(abs(s["total_charge"]) < 1e-4 and not s["untyped_atoms"] and not s["warnings"]
              and not s["types_missing_from_ff"], name + ": neutral, every type in clayff.ff, no warning")
        check(not s["charges_differing_from_atomipy"], name + ": atomipy's charges are Table 1's")
        top = open(os.path.join(out, "topol.top")).read()
        check('clayff.ff/forcefield.itp' in top and "MIN    1" in top, name + ": topol.top includes clayff.ff")
        for d in ("", "-DCLAYFF_MOH", "-DCLAYFF_MOH_2004"):
            minimise(out, "%s %s" % (name, d or "(no switch)"), d)
        # angle counts: none without a switch, one per M-O-H with it
        itp = open(os.path.join(out, "min.itp")).read()
        check(itp.count("-O-H") % 2 == 0 and "[ angles ]" in itp, name + ": M-O-H angles only inside the switches")


def test_water_ions(work):
    out = os.path.join(work, "water")
    os.makedirs(out)
    shutil.copytree(FF, os.path.join(out, "clayff.ff"))
    r = run(["gmx", "solvate", "-cs", "spc216.gro", "-box", "1.86", "1.86", "1.86", "-o", "w.gro"], cwd=out)
    n = int(open(os.path.join(out, "w.gro")).read().split("\n")[1]) // 3
    check(r.returncode == 0, "water: gmx solvate")
    top = ('#include "clayff.ff/forcefield.itp"\n#include "clayff.ff/ions.itp"\n'
           '#include "clayff.ff/spc.itp"\n[ system ]\nw\n[ molecules ]\nSOL %d\n' % n)
    open(os.path.join(out, "topol.top"), "w").write(top)
    for d, lab in (("", "flexible SPC"), ("-DRIGID_WATER", "rigid SPC")):
        minimise(out, "water " + lab, d, "w.gro")
    # one of each ion in a box, only to prove ions.itp and the types parse
    ions = [("Na", "Na+"), ("K", "K+"), ("Cs", "Cs+"), ("Ca", "Ca2+"), ("Ba", "Ba2+"), ("Cl", "Cl-")]
    with open(os.path.join(out, "i.gro"), "w") as f:
        f.write("ions\n%d\n" % len(ions))
        for i, (m, a) in enumerate(ions):
            f.write("%5d%-5s%5s%5d%8.3f%8.3f%8.3f\n" % (i + 1, m, a, i + 1, 0.5 * (i % 3) + 0.2, 0.5 * (i // 3) + 0.2, 0.5))
        f.write("   3.0   3.0   3.0\n")
    open(os.path.join(out, "topol.top"), "w").write(
        '#include "clayff.ff/forcefield.itp"\n#include "clayff.ff/ions.itp"\n[ system ]\nions\n[ molecules ]\n' +
        "".join("%s 1\n" % m for m, _ in ions))
    with open(os.path.join(out, "em.mdp"), "w") as f:
        f.write(EM_MDP % "")
    r = run(["gmx", "grompp", "-f", "em.mdp", "-c", "i.gro", "-p", "topol.top", "-o", "i.tpr",
             "-maxwarn", "1", "-po", "o.mdp"], cwd=out)
    check(r.returncode == 0, "ions Na K Cs Ca Ba Cl: grompp (net charge 3, hence one warning)")
    if r.returncode:
        print(r.stderr[-1000:])


NPT_MDP = """integrator = md
dt = 0.0005
nsteps = %d
nstcomm = 100
cutoff-scheme = verlet
coulombtype = PME
rcoulomb = 1.0
rvdw = 1.0
DispCorr = EnerPres
pbc = xyz
periodic-molecules = yes
tcoupl = nose-hoover
tc-grps = System
tau-t = 0.5
ref-t = 300
%s
nstenergy = 200
nstlog = 20000
nstxout-compressed = 2000
compressed-x-grps = System
define = %s
"""
# Berendsen (anisotropic, damped) gives the correct mean cell of a solid.  The
# inertial Parrinello-Rahman barostat of the paper's runs threw the soft c axis
# of the layered minerals apart here: the cell starts at ~1e4 bar, the pressure
# fluctuates by 1e4 bar in a cell this small, and beyond the cut-off nothing
# pulls the layers back.  C-rescale has no anisotropic mode.
BER = """pcoupl = Berendsen
pcoupltype = anisotropic
tau-p = 5.0
compressibility = 4.5e-5 4.5e-5 4.5e-5 4.5e-5 4.5e-5 4.5e-5
ref-p = 1 1 1 0 0 0
refcoord-scaling = com
"""
NVT = "gen-vel = yes\ngen-temp = 300\ngen-seed = 20260918\n"


def cell_params(edr_dir, hexagonal=False):
    """Per-frame (t ps, a, b, c in A, alpha, beta, gamma in deg) from the box of the
    xtc (the edr keeps only the diagonal of a triclinic box)."""
    r = run(["gmx", "traj", "-f", "npt.xtc", "-s", "npt.tpr", "-ob", "box.xvg"], cwd=edr_dir,
            input="0\n")
    if r.returncode:
        print(r.stderr[-800:])
        return []
    out = []
    for l in open(os.path.join(edr_dir, "box.xvg")):
        if not l.strip() or l[0] in "#@":
            continue
        v = [float(x) for x in l.split()]
        t, rest = v[0], v[1:]
        if len(rest) == 9:      # xx yx zx xy yy zy xz yz zz, row-wise
            xx, yy, zz = rest[0], rest[4], rest[8]
            yx, zx, zy = rest[3], rest[6], rest[7]
        else:                   # xx yy zz yx zx zy
            xx, yy, zz, yx, zx, zy = rest[:6]
        A, B, C = (xx * 10, 0, 0), (yx * 10, yy * 10, 0), (zx * 10, zy * 10, zz * 10)
        n = lambda u: math.sqrt(sum(c * c for c in u))
        ang = lambda u, w: math.degrees(math.acos(sum(p * q for p, q in zip(u, w)) / (n(u) * n(w))))
        al, be, ga = ang(B, C), ang(A, C), ang(A, B)
        if hexagonal:     # a hexagonal cell flips between 60 and 120 degrees: same lattice
            ga = max(ga, 180 - ga)
        out.append((t, n(A), n(B), n(C), al, be, ga, xx * yy * zz * 1000))
    return out


def npt(name, uc, work, ps, define):
    f, rep = CELLS[name]
    out = os.path.join(work, "npt-" + name)
    s = build(os.path.join(uc, f), out, rep)
    if not s:
        return None
    n = s["n_atoms"]
    print("  %s: %d atoms, replication %s" % (name, n, "x".join(map(str, rep))), flush=True)
    if minimise(out, name + " EM", "") is None:
        return None
    shutil.copy(os.path.join(out, "run.gro"), os.path.join(out, "em.gro"))
    with open(os.path.join(out, "nvt.mdp"), "w") as f:
        f.write(NPT_MDP % (40000, NVT, define))
    if not grompp(out, "nvt.mdp", name + " NVT", ["em.gro"]):
        return None
    r = run(["gmx", "mdrun", "-deffnm", "run", *THREADS, "-c", "nvt.gro"], cwd=out)
    check(r.returncode == 0, name + ": 20 ps NVT")
    if r.returncode:
        print(r.stderr[-800:])
        return None
    for stage, ctl, steps, src in (("npt", BER, int(ps * 2000), "nvt.gro"),):
        with open(os.path.join(out, stage + ".mdp"), "w") as f:
            f.write(NPT_MDP % (steps, ctl, define))
        r = run(["gmx", "grompp", "-f", stage + ".mdp", "-c", src, "-p", "topol.top", "-o", stage + ".tpr",
                 "-maxwarn", "1", "-po", "mdout-" + stage + ".mdp"], cwd=out)   # the Berendsen warning
        check(r.returncode == 0, "%s: grompp %s" % (name, stage))
        if r.returncode:
            print(r.stderr[-1500:])
            return None
        r = run(["gmx", "mdrun", "-deffnm", stage, *THREADS], cwd=out)
        check(r.returncode == 0, "%s: %g ps NPT" % (name, ps))
        if r.returncode:
            print(r.stderr[-800:])
            return None
    fr = cell_params(out, name in ("brucite", "portlandite"))
    fr = [x for x in fr if x[0] >= ps / 2.0]       # second half
    mean = [sum(x[i] for x in fr) / len(fr) for i in range(1, 8)]
    sd = [math.sqrt(sum((x[i] - mean[i - 1]) ** 2 for x in fr) / len(fr)) for i in range(1, 8)]
    return rep, mean, sd, n


def long_mode(uc, work, ps, keep_note):
    results = {}
    order = ["brucite", "gibbsite", "kaolinite", "boehmite", "portlandite", "pyrophyllite"]
    for name in [n for n in order if n in CELLS]:
        r = npt(name, uc, work, ps, "")
        if not r:
            continue
        rep, mean, sd, n = r
        results[name] = (rep, mean, sd, n)
    print()
    print("NPT %g ps (second half averaged), cell rescaled to the unit cell [A, deg]" % (ps))
    for name, (rep, m, sd, n) in results.items():
        a, b, c = m[0] / rep[0], m[1] / rep[1], m[2] / rep[2]
        al, be, ga = m[3], m[4], m[5]
        # the supercell vectors may run in other directions than the unit cell's: lengths per unit cell
        ref = POUVREAU_ORIG.get(name) or CYGAN_2004.get(name)
        src = "Pouvreau 2017 ClayFF-orig" if name in POUVREAU_ORIG else "Cygan 2004 simulation"
        tol_len = 0.03 if name == "portlandite" else 0.015
        print("%-13s %5d atoms  a %.3f b %.3f c %.3f alpha %.2f beta %.2f gamma %.2f   (%s)" % (
            name, n, a, b, c, al, be, ga, src))
        vals = dict(a=a, b=b, c=c, alpha=al, beta=be, gamma=ga)
        if name == "brucite":     # hexagonal cell: a and b are one length, c along z
            vals["a"] = (a + b) / 2
        V = m[6] / (rep[0] * rep[1] * rep[2])
        print("              V %.2f A^3" % V)
        for k, v in ref.items():
            if k == "V":
                got = V
                ok = abs(got / v - 1) < 0.03
                print("   %-5s %8.3f  vs %8.3f  (%+.1f %%)" % (k, got, v, 100 * (got / v - 1)))
            elif k in ("a", "b", "c"):
                got = vals[k]
                ok = abs(got / v - 1) < tol_len
                print("   %-5s %8.3f  vs %8.3f  (%+.1f %%)" % (k, got, v, 100 * (got / v - 1)))
            else:
                got = vals[k]
                ok = abs(got - v) < 3.0
                print("   %-5s %8.2f  vs %8.2f  (%+.1f deg)" % (k, got, v, got - v))
            check(ok, "%s %s within tolerance of %s" % (name, k, src))
    return results


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--long", action="store_true", help="also run the NPT comparisons")
    ap.add_argument("--ps", type=float, default=250.0, help="NPT length in ps (default 250)")
    ap.add_argument("--keep", action="store_true", help="keep the work directory")
    ap.add_argument("--only", help="comma-separated systems for --long")
    a = ap.parse_args()
    if not mt.find_atomipy_python():
        print("SKIP: atomipy is not installed")
        return 77
    if not shutil.which("gmx"):
        print("SKIP: gmx is not installed")
        return 77
    try:
        ffdir = mt.fetch_pinned()
    except mt.MinffError as e:
        print("SKIP: %s" % e)
        return 77
    uc = os.path.join(ffdir, "Systems", "UC_conf")
    work = tempfile.mkdtemp(prefix=".work-", dir=HERE)
    os.environ["ECCE_MINFF_REEXEC"] = "1"
    try:
        test_parameters()
        test_minerals(uc, work)
        test_water_ions(work)
        if a.long:
            if a.only:
                keep = a.only.split(",")
                for k in list(CELLS):
                    if k not in keep:
                        del CELLS[k]
            long_mode(uc, work, a.ps, a.keep)
    finally:
        if a.keep:
            print("work directory kept: " + work)
        else:
            shutil.rmtree(work, ignore_errors=True)
    print("\n%d failure(s)" % len(failures) if failures else "\nall ok")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
