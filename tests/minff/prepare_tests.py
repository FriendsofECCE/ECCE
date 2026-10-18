#!/usr/bin/env python3
"""GROMACS Prepare with the MINFF source, at script level (issue #930, #241).

minff_topology.py builds a mineral on the client; its conf.gro, topol.top and
includes are staged the way Launch stages the task's inputs (conf.gmxmin,
topol.gmxmin, min.ff__SLASH__<file>); md.gmxprep writes the recipe,
gromacs.launchpp checks it and the job script gensub writes is run for real.
What a prepared mineral has to be is then judged by gmx: grompp accepts it, a
short steepest-descent run lowers a finite energy, the cell and the charge
are what the recipe asked for.

Systems: kaolinite and a substituted pyrophyllite from the minff repository's
own unit cells (fetched with min.ff, never stored here), and, when present,
the montmorillonite layer of the MINFF run in $ECCE_MINFF_REAL (default
~/tmp/Andy/0_MD_gromacs_minff/minff_MD/CuII_MMT_minff.gro), read only.

Exit status 77 (CTest SKIP) without atomipy, gmx or min.ff.
"""
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SCRIPT = os.path.join(ROOT, "scripts", "minff_topology.py")
PARSERS = os.path.join(ROOT, "scripts", "parsers")
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import minff_topology as mt  # noqa: E402

REAL = os.environ.get("ECCE_MINFF_REAL", os.path.expanduser(
    "~/tmp/Andy/0_MD_gromacs_minff/minff_MD/CuII_MMT_minff.gro"))
failures = []

EM_MDP = """integrator = steep
nsteps = 200
emtol = 10
emstep = 0.001
cutoff-scheme = verlet
coulombtype = PME
rcoulomb = 0.9
rvdw = 0.9
vdwtype = cut-off
DispCorr = EnerPres
pbc = xyz
periodic-molecules = yes
nstenergy = 10
nstlog = 10
"""


def check(cond, msg):
    print(("ok   " if cond else "FAIL ") + msg)
    if not cond:
        failures.append(msg)
    return cond


def run(cmd, **kw):
    kw.setdefault("capture_output", True)
    kw.setdefault("text", True)
    return subprocess.run(cmd, **kw)


def stage(build_dir, rundir):
    """The task's inputs as Launch stages them in the run directory."""
    shutil.copy(os.path.join(build_dir, "conf.gro"), os.path.join(rundir, "conf.gmxmin"))
    shutil.copy(os.path.join(build_dir, "topol.top"), os.path.join(rundir, "topol.gmxmin"))
    for root, _dirs, files in os.walk(build_dir):
        for f in files:
            rel = os.path.relpath(os.path.join(root, f), build_dir)
            if rel in ("conf.gro", "topol.top", "minff.json") or not f.endswith(".itp"):
                continue
            shutil.copy(os.path.join(root, f),
                        os.path.join(rundir, rel.replace(os.sep, "__SLASH__")))


def box_of(gro):
    return [float(x) for x in open(gro).read().rstrip("\n").split("\n")[-1].split()]


def molecules(top):
    mols, on = {}, False
    for line in open(top):
        line = line.split(";")[0].strip()
        if line.startswith("["):
            on = line.replace(" ", "") == "[molecules]"
        elif on and line:
            n, c = line.split()
            mols[n] = mols.get(n, 0) + int(c)
    return mols


def prepare(gmx, tmp, name, mineral_dir, recipe, expect_water, charge, extra=()):
    """Recipe -> launchpp -> job script; returns the run directory."""
    print("== %s" % name)
    rundir = os.path.join(tmp, "run-" + name)
    os.makedirs(rundir)
    stage(mineral_dir, rundir)
    r = run([os.path.join(PARSERS, "md.gmxprep")], input=recipe)
    if not check(r.returncode == 0, "md.gmxprep writes the recipe: " + r.stderr.strip()):
        return None
    open(os.path.join(rundir, "gromacs.gmxprep"), "w").write(r.stdout)
    open(os.path.join(rundir, "postParams"), "w").write(
        "inputFile: gromacs.gmxprep\nrunDir: %s\n" % rundir)
    r = run([os.path.join(PARSERS, "gromacs.launchpp"), "-p", "postParams"], cwd=rundir,
            env=dict(os.environ, PATH=os.path.dirname(gmx) + os.pathsep + os.environ["PATH"]))
    if not check(r.returncode == 0 and "accepted" in r.stdout,
                 "gromacs.launchpp: grompp accepts the mineral (%s)" % (r.stdout + r.stderr).strip()[-200:]):
        return None
    params = os.path.join(rundir, "subParams")
    open(params, "w").write(
        " -Q Shell\n -H localtest\n -d localhost\n -c GROMACS\n -n 1\n -N 1\n -r %s\n"
        " -i gromacs.gmxprep\n -o gromacs.gmxlog\n -f %s/submit__prep\n -S ecceSession\n"
        " -C prepare\n" % (rundir, rundir))
    r = run([os.path.join(HOME["home"], "scripts", "gensub"), "-v", "-p", params],
            env=HOME["env"], cwd=rundir)
    if not check(r.returncode == 0, "gensub writes the Prepare job script"):
        print(r.stdout, r.stderr)
        return None
    r = run(["sh", os.path.join(rundir, "submit__prep")], cwd=rundir)
    status = open(os.path.join(rundir, ".ecce.status")).read().strip()
    if not check(status == "0", "the Prepare job exits 0 (got %r)" % status):
        print(open(os.path.join(rundir, "gromacs.gmxlog")).read()[-1500:])
        return None
    return rundir


HOME = {}


def judge(gmx, rundir, label, cell_in, layer, charge_in, expect_ions=None):
    gro = os.path.join(rundir, "ecceSession_prepare.gro")
    top = os.path.join(rundir, "ecceSession.gmxtop")
    check(os.path.exists(gro) and os.path.exists(top),
          label + ": structure and topology are written under the names the next task looks for")
    log = open(os.path.join(rundir, "gromacs.gmxlog"), errors="replace").read()
    check(set(re.findall(r"^ECCE-PREPARE step (\d)/4 (\S+)", log, re.M))
          == {("1", "minff"), ("2", "editconf"), ("3", "solvate"), ("4", "genion")},
          label + ": the four steps are announced (step 1 is minff)")
    # the cell: a, b untouched; c longer by the layer
    b0, b1 = box_of(os.path.join(rundir, "conf.gmxmin")), box_of(gro)
    cell = b0[0], b0[1], b0[2]
    b0 = b0 + [0.0] * (9 - len(b0))
    b1 = b1 + [0.0] * (9 - len(b1))
    check(abs(b1[0] - b0[0]) < 1e-4 and abs(b1[1] - b0[1]) < 1e-4
          and abs(b1[2] - (b0[2] + layer)) < 1e-4
          and all(abs(x - y) < 1e-4 for x, y in zip(b1[3:], b0[3:])),
          label + ": cell %.3f x %.3f x %.3f nm, c lengthened by %.2f nm to %.3f"
          % (cell[0], cell[1], cell[2], layer, b1[2]))
    m = re.search(r"ECCE-PREPARE box charge0=(\S+) charge1=(\S+) box=cell a=(\S+) b=(\S+) "
                  r"c=(\S+) alpha=(\S+) beta=(\S+) gamma=(\S+)", log)
    if check(m is not None, label + ": the summary line has the cell"):
        # lengths of the cell vectors, by hand, against the summary's
        v1 = (b1[0], b1[3], b1[4])
        v2 = (b1[5], b1[1], b1[6])
        v3 = (b1[7], b1[8], b1[2])
        want = [math.sqrt(sum(x * x for x in v)) for v in (v1, v2, v3)]
        got = [float(m.group(i)) for i in (3, 4, 5)]
        check(all(abs(g - w) < 0.002 for g, w in zip(got, want)),
              label + ": summary cell lengths %s agree with the box (%s)"
              % (got, ["%.3f" % w for w in want]))
    mols = molecules(top)
    natoms =int(open(gro).read().splitlines()[1])
    waters = mols.get("SOL", 0)
    check((waters > 0) == (layer > 0),
          label + ": %d atoms, %d waters (layer %.2f nm)" % (natoms, waters, layer))
    # charge: grompp's own account of the finished system
    shutil.copy(os.path.join(rundir, "ions.mdp"), os.path.join(rundir, "chk.mdp"))
    shutil.copy(top, os.path.join(rundir, "chk.top"))
    r = run([gmx, "grompp", "-f", "chk.mdp", "-c", gro, "-p", "chk.top", "-o", "chk.tpr"],
            cwd=rundir)
    m = re.search(r"non-zero total charge: *(\S+)", r.stdout + r.stderr)
    q = float(m.group(1)) if m else 0.0
    if layer > 0:
        check(r.returncode == 0 and abs(q) < 1e-3,
              label + ": neutral after genion (charge %.4f, was %.1f)" % (q, charge_in))
    else:
        check(r.returncode == 0 and abs(q - charge_in) < 1e-3,
              label + ": no water, so the charge stays %.1f (grompp sees %.4f)" % (charge_in, q))
    if expect_ions:
        for k, v in expect_ions.items():
            check(mols.get(k) == v, label + ": %d x %s (%s)" % (v, k, mols))
    pdb = os.path.join(rundir, "chemsys.pdb")
    check(os.path.exists(pdb) and "CRYST1" in open(pdb).read(),
          label + ": chemsys.pdb keeps the cell")
    # a short minimisation of the prepared system, periodic molecules as the
    # Optimize task will have it
    open(os.path.join(rundir, "em.mdp"), "w").write(EM_MDP)
    r = run([gmx, "grompp", "-f", "em.mdp", "-c", gro, "-p", "chk.top", "-o", "em.tpr",
             "-maxwarn", "1" if layer == 0 and abs(charge_in) > 0.01 else "0"],
            cwd=rundir)
    if not check(r.returncode == 0, label + ": grompp for the minimisation"):
        print(r.stderr[-1500:])
        return
    r = run([gmx, "mdrun", "-deffnm", "em", "-nt", "1"], cwd=rundir)
    check(r.returncode == 0, label + ": mdrun")
    r = run([gmx, "energy", "-f", "em.edr", "-o", "pe.xvg"], cwd=rundir,
            input="Potential\n0\n")
    pe = [float(l.split()[1]) for l in open(os.path.join(rundir, "pe.xvg"))
          if l.strip() and l[0] not in "#@"]
    check(len(pe) > 2 and all(map(math.isfinite, pe)) and pe[-1] < pe[0],
          label + ": potential energy %.5g -> %.5g kJ/mol" % (pe[0], pe[-1]))


def mineral_only(src, dst):
    L = open(src).read().splitlines()
    keep = [l for l in L[2:-1] if l[5:10].strip() == "MIN"]
    open(dst, "w").write("\n".join([L[0], str(len(keep))] + keep + [L[-1]]) + "\n")


def build(py, cell, out, *args):
    r = run([py, SCRIPT, "build", cell, "-o", out, *args])
    if not check(r.returncode == 0, "build %s %s" % (os.path.basename(cell), " ".join(args))):
        print(r.stderr[-1500:])
        return None
    import json
    return json.loads(r.stdout)


def main():
    py = mt.find_atomipy_python()
    if not py:
        print("SKIP: atomipy is not installed")
        return 77
    gmx = shutil.which("gmx")
    if not gmx:
        print("SKIP: gmx is not installed")
        return 77
    try:
        ffdir = mt.fetch_pinned()
    except mt.MinffError as e:
        print("SKIP: %s" % e)
        return 77
    os.environ["ECCE_MINFF_REEXEC"] = "1"
    tmp = tempfile.mkdtemp(prefix="minffprep-")
    home = os.path.join(tmp, "home")
    user = os.path.join(tmp, "user")
    os.makedirs(os.path.join(user, ".ECCE"))
    os.makedirs(os.path.join(home, "data"))
    for sub in ("scripts", "siteconfig"):
        os.symlink(os.path.join(ROOT, sub), os.path.join(home, sub))
    os.symlink(os.path.join(ROOT, "data", "client"), os.path.join(home, "data", "client"))
    open(os.path.join(user, ".ECCE", "CONFIG.localtest"), "w").write(
        "GROMACS: %s\nperlPath: /usr/bin\n" % gmx)
    HOME.update(home=home, env=dict(os.environ, ECCE_HOME=home, ECCE_REALUSERHOME=user))

    uc = os.path.join(ffdir, "Systems", "UC_conf")

    # the recipe writer's own checks
    for bad in ("SOURCE=minff\nPNAME=NA\n", "SOURCE=minff\nWATER=tip5p\n",
                "SOURCE=minff\nNNAME=CL\n", "SOURCE=gold\n", "PQ=2\n"):
        r = run([os.path.join(PARSERS, "md.gmxprep")], input=bad)
        check(r.returncode != 0, "md.gmxprep refuses %r" % bad.replace("\n", " ").strip())
    r = run([os.path.join(PARSERS, "md.gmxprep")], input="SOURCE=minff\nPNAME=Cu2+\n")
    check(r.returncode == 0 and "PQ=2" in r.stdout and "DIST=0" in r.stdout
          and "WATER=opc3" in r.stdout and "CONC=0\n" in r.stdout,
          "md.gmxprep: minff defaults and the charge of Cu2+ (PQ=2)")

    # 1. kaolinite, neutral: keep the cell, then add a water layer
    k = os.path.join(tmp, "kaol")
    s = build(py, os.path.join(uc, "Kaolinite_GII_0.0487.pdb"), k, "--replicate", "4", "3", "3")
    if s:
        d = prepare(gmx, tmp, "kaol-cell", k, "SOURCE=minff\nWATER=opc3\nDIST=0\nCONC=0\n", 0, 0)
        if d:
            judge(gmx, d, "kaolinite, no water", None, 0.0, s["total_charge"])
        d = prepare(gmx, tmp, "kaol-water", k, "SOURCE=minff\nWATER=opc3\nDIST=2\nCONC=0\n", 1, 0)
        if d:
            judge(gmx, d, "kaolinite, 2 nm water", None, 2.0, s["total_charge"])

    # 2. pyrophyllite with 4 Al -> Mg: charge -4 neutralised by 2 Mg2+ or 4 Na+
    p = os.path.join(tmp, "pyro")
    s = build(py, os.path.join(uc, "Pyrophyllite_Lee_Guggenheim_1981_GII_0.071.pdb"), p,
              "--replicate", "4", "3", "3", "--substitute", "Al", "Mgo", "4", "--seed", "1")
    if s:
        d = prepare(gmx, tmp, "pyro-na", p, "SOURCE=minff\nDIST=2\nCONC=0\nPNAME=Na\n", 1, -4)
        if d:
            judge(gmx, d, "pyrophyllite, Na+", None, 2.0, s["total_charge"], {"Na": 4})
        d = prepare(gmx, tmp, "pyro-ca", p, "SOURCE=minff\nDIST=2\nCONC=0\nPNAME=Ca\n", 1, -4)
        if d:
            judge(gmx, d, "pyrophyllite, Ca2+", None, 2.0, s["total_charge"], {"Ca": 2})
        d = prepare(gmx, tmp, "pyro-nowater", p, "SOURCE=minff\nDIST=0\nCONC=0\n", 0, -4)
        if d:
            judge(gmx, d, "charged pyrophyllite, no water", None, 0.0, s["total_charge"])

    # 3. the montmorillonite of a real MINFF run (optional)
    if os.path.exists(REAL):
        cell = os.path.join(tmp, "mmt_cell.gro")
        mineral_only(REAL, cell)
        m = os.path.join(tmp, "mmt")
        s = build(py, cell, m)
        if s:
            d = prepare(gmx, tmp, "mmt-cu", m,
                        "SOURCE=minff\nDIST=3\nCONC=0\nPNAME=Cu2+\n", 1, s["total_charge"])
            if d:
                judge(gmx, d, "montmorillonite (%d atoms), Cu2+" % s["n_atoms"], None, 3.0,
                      s["total_charge"], {"Cu2+": int(round(-s["total_charge"] / 2))})
    else:
        print("note: %s is not here; the real montmorillonite is skipped" % REAL)

    if not failures:
        shutil.rmtree(tmp, ignore_errors=True)
    else:
        print("work directory kept: " + tmp)
    print("%d failure(s)" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
