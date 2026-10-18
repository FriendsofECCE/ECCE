#!/usr/bin/env python3
"""GROMACS MD study tasks, through the real scripts and the real gmx.

A GROMACS study is run by four scripts of ours around gmx itself:

    md.gmxtask        task model (XML, as MDEdBase saves it)  ->  .mdp
    gromacs.launchpp  the launch-time check of the staged inputs
    gensub            the job script (grompp, mdrun, editconf)
    eccejobmonitor + gromacs.desc + gromacs.energy   the plots

This runs them in the order a study does, one task after another, each
starting from the structure the one before wrote, on a small box of SPC
water:

    Optimize (steep)  ->  Equilibrate NVT  ->  Equilibrate NPT (continues)
                      ->  Dynamics (continues)

and checks what each stage left behind against gmx itself (gmx energy on
the .edr, gmx check on the .gro) rather than against what the scripts were
expected to print.  The task models are written here in the shape
NWChemMDModelXMLizer writes them; tests/apps/gromacs_test.py runs the same
scripts on models saved by the real editor, which is what proves the shape.

    tests/gromacs_md/run_tests.py [-v] [--keep]

Exit status 77 (CTest SKIP) without gmx.  Needs perl and python3 only
otherwise: no build, no services.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(REPO, "scripts", "parsers")
sys.path.insert(0, os.path.join(REPO, "tests", "e2e"))
sys.path.insert(0, os.path.join(REPO, "tests", "parsers"))
import pipeline  # noqa: E402
from eccejobmonitor_sim import read_desc  # noqa: E402

FIXTURES = os.path.join(HERE, "fixtures")

failures = []
verbose = False


def check(ok, what):
    if verbose or not ok:
        print("  %s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def find_gmx():
    for d in os.environ.get("PATH", "").split(os.pathsep):
        p = os.path.join(d, "gmx")
        if os.access(p, os.X_OK):
            return p
    return None


# ---- the task models, as the serializer writes them -------------------------

def model(task, interaction="", optimize="", dynamics="", files=""):
    return """<?xml version="1.0"?>
<NWChemMDModel>
<SystemName>ecceSession</SystemName>
<CalcId>%s</CalcId>
<Code>GROMACS</Code>
<TaskType>%s</TaskType>
  <Interaction>
    <Touched>1</Touched>
    <InteractionOption>0</InteractionOption>
    <GridDimensions>8</GridDimensions>
    <CutoffOption>1</CutoffOption>
%s
  </Interaction>
  <Constraints>
    <UseSHAKE>1</UseSHAKE>
  </Constraints>
%s%s  <Files>
    <RestartFreq> 100</RestartFreq>
%s
  </Files>
</NWChemMDModel>
""" % (task, task, interaction,
       ("  <Optimize>\n%s  </Optimize>\n" % optimize) if optimize else "",
       ("  <Dynamics>\n%s  </Dynamics>\n" % dynamics) if dynamics else "",
       files)


def dyn(steps, resume, npt=False, temp=300):
    return """    <ResumeOpt> %d</ResumeOpt>
    <EquilibrationSteps> 0</EquilibrationSteps>
    <TimeStep> 0.002</TimeStep>
    <DataSteps> %d</DataSteps>
    <RemoveCM> 1</RemoveCM>
    <RemoveCMFreq> 10</RemoveCMFreq>
    <UseNVT> 1</UseNVT>
    <NVTTemperature> %g</NVTTemperature>
    <UseAnnealing> 0</UseAnnealing>
    <UseNPT> %d</UseNPT>
    <NPTPressure> 101325</NPTPressure>
    <NPTRxTime> 1</NPTRxTime>
    <Compressibility> 4.53e-10</Compressibility>
""" % (1 if resume else 0, steps, temp, 1 if npt else 0)


STAGES = [
    ("optimize", "ecceMdOptimize", dict(
        optimize="""    <UseSD>1</UseSD>
    <SDMaxIterations>200</SDMaxIterations>
    <SDInitialStepSize>0.01</SDInitialStepSize>
    <SDTolerance>100</SDTolerance>
    <UseCG>0</UseCG>
    <CGMaxIterations>100</CGMaxIterations>
""")),
    ("nvt", "ecceMdEquilibrate", dict(dynamics=dyn(500, False))),
    ("npt", "ecceMdEquilibrate", dict(dynamics=dyn(500, True, npt=True))),
    ("dynamics", "ecceMdDynamics", dict(
        dynamics=dyn(500, True, npt=True),
        files="    <WriteTrajectory> 1</WriteTrajectory>\n"
              "    <CoordinatesSoluteFreq> 50</CoordinatesSoluteFreq>\n"
              "    <CoordinatesSolutePrint> 1</CoordinatesSolutePrint>\n")),
]


def run(cmd, **kw):
    kw.setdefault("stdout", subprocess.PIPE)
    kw.setdefault("stderr", subprocess.STDOUT)
    kw.setdefault("timeout", 600)
    r = subprocess.run(cmd, **kw)
    return r.returncode, r.stdout.decode("utf-8", "replace")


class Home(object):
    """A minimal ECCE_HOME and user home: just what gensub reads."""

    def __init__(self, tmp, gmx):
        self.home = os.path.join(tmp, "home")
        self.user = os.path.join(tmp, "user")
        os.makedirs(os.path.join(self.user, ".ECCE"))
        os.makedirs(os.path.join(self.home, "data"))
        for sub in ("scripts", "siteconfig"):
            os.symlink(os.path.join(REPO, sub), os.path.join(self.home, sub))
        os.symlink(os.path.join(REPO, "data", "client"),
                   os.path.join(self.home, "data", "client"))
        with open(os.path.join(self.user, ".ECCE", "CONFIG.localtest"), "w") as h:
            h.write("GROMACS: %s\nperlPath: /usr/bin\n" % gmx)
        self.env = dict(os.environ, ECCE_HOME=self.home,
                        ECCE_REALUSERHOME=self.user)


def stage(gmx, home, tmp, name, calc, spec, start_gro, nprocs=2,
          topology=os.path.join(FIXTURES, "topol.top"), extras=None):
    """One task of the study, the way Launch and the job script handle it."""
    print("== %s" % name)
    rundir = os.path.join(tmp, "run-" + name)
    os.makedirs(rundir)

    # 1. the model -> the .mdp
    xml = model(calc, **spec)
    rc, out = run([os.path.join(PARSERS, "md.gmxtask")], input=xml.encode(),
                  cwd=rundir)
    if not check(rc == 0, "md.gmxtask wrote the .mdp"):
        print(out)
        return None
    with open(os.path.join(rundir, "gromacs.mdp"), "w") as h:
        h.write(out)
    mdp = dict((m.group(1), m.group(2).strip()) for m in
               re.finditer(r"^(\S+)\s*=\s*(.*)$", out, re.M))

    # 2. staged under the names Launch gives them
    shutil.copy(topology, os.path.join(rundir, "topol.top"))
    shutil.copy(start_gro, os.path.join(rundir, "conf.gro"))
    for stored, src in (extras or {}).items():
        shutil.copy(src, os.path.join(rundir, stored))

    # 3. the launch-time check, as Launch calls it
    with open(os.path.join(rundir, "postParams"), "w") as h:
        h.write("inputFile: gromacs.mdp\nnumProcs: %d\nrunDir: %s\n"
                % (nprocs, rundir))
    rc, out = run([os.path.join(PARSERS, "gromacs.launchpp"), "-p", "postParams"],
                  cwd=rundir)
    check(rc == 0 and "accepted" in out, "gromacs.launchpp: grompp accepts the inputs")
    if rc != 0:
        print(out)
        return None

    # 4. the job script, from the real gensub, run as the Shell queue runs it
    params = os.path.join(rundir, "subParams")
    with open(params, "w") as h:
        h.write(" -Q Shell\n -H localtest\n -d localhost\n -c GROMACS\n"
                " -n %d\n -N 1\n -r %s\n -i gromacs.mdp\n -o gromacs.gmxlog\n"
                " -f %s/submit__%s\n -S ecceSession\n -C %s\n"
                % (nprocs, rundir, rundir, name, calc))
    rc, out = run([os.path.join(home.home, "scripts", "gensub"), "-v", "-p", params],
                  env=home.env, cwd=rundir)
    if not check(rc == 0, "gensub wrote the job script"):
        print(out)
        return None
    rc, out = run(["sh", os.path.join(rundir, "submit__" + name)], cwd=rundir)
    check(rc == 0, "the job script ran")
    status = open(os.path.join(rundir, ".ecce.status")).read().strip()
    check(status == "0", "job exit status is 0 (got %r)" % status)
    final = os.path.join(rundir, "ecceSession_%s.gro" % calc)
    check(os.path.exists(final), "the final structure is written under the name the next task looks for")
    check(os.path.exists(os.path.join(rundir, "chemsys.pdb")),
          "the final structure is also written as chemsys.pdb for the Builder")
    log = os.path.join(rundir, "gromacs.gmxlog")
    check(os.path.exists(log) and "Energies" in open(log, errors="replace").read(),
          "the output file has the energies the monitor reads")

    return dict(rundir=rundir, mdp=mdp, gro=final, log=log)


def properties(res, tmp, name):
    desc_path = os.path.join(PARSERS, "gromacs.desc")
    work = os.path.join(tmp, "mon-" + name)
    os.makedirs(work)
    results = pipeline.run_monitor(res["log"], desc_path, work, calc_name=name)
    blocks = pipeline.unpack(results)
    desc = read_desc(desc_path)
    return pipeline.run_parsers(blocks, desc, (".", "Energy", "MD", "MD", "0"),
                                workdir=work)


def last(props, key):
    return float(pipeline.sect(props[key][-1], "values"))


def gmx_energy(gmx, res, term):
    """The last value of a term in the .edr, read by gmx itself."""
    edr = os.path.join(res["rundir"], "ener.edr")
    p = subprocess.run([gmx, "energy", "-f", edr, "-o", "e.xvg"],
                       input=(term + "\n0\n").encode(), cwd=res["rundir"],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    vals = []
    for line in open(os.path.join(res["rundir"], "e.xvg")):
        if line.strip() and line[0] not in "#@":
            vals.append(float(line.split()[1]))
    return vals


def restrained(gmx, home, tmp):
    src = os.path.join(tmp, "restrained")
    os.makedirs(src)
    with open(os.path.join(FIXTURES, "conf.gro")) as h:
        lines = h.read().splitlines()
    gro = os.path.join(src, "conf.gro")
    with open(gro, "w") as h:
        h.write("Three SPC waters\n9\n%s\n%s\n"
                % ("\n".join(lines[2:11]), lines[-1]))
    top = os.path.join(src, "topol.top")
    with open(top, "w") as h:
        h.write('#include "oplsaa.ff/forcefield.itp"\n'
                '#include "oplsaa.ff/spc.itp"\n\n'
                "[ position_restraints ]\n"
                "; atom  funct  fcx   fcy   fcz\n"
                "    1      1  1000  1000  1000\n\n"
                "[ system ]\nRestrained waters\n\n[ molecules ]\nSOL   3\n")
    name, calc, spec = STAGES[0]
    res = stage(gmx, home, tmp, "restrained", calc, spec, gro, topology=top)
    check(res is not None,
          "a topology with [ position_restraints ] is accepted at launch and runs")


def periodic_molecules(gmx, home, tmp, results):
    """periodic-molecules is written only when the model asks for it (#262),
    and grompp takes it."""
    if "optimize" in results:
        check("periodic-molecules" not in results["optimize"]["mdp"],
              "periodic-molecules is absent when the model does not set it")
    name, calc, spec = STAGES[0]
    spec = dict(spec, interaction="    <PeriodicMolecules>1</PeriodicMolecules>")
    res = stage(gmx, home, tmp, "periodic", calc, spec,
                os.path.join(FIXTURES, "conf.gro"))
    if res is not None:
        check(res["mdp"].get("periodic-molecules") == "yes",
              "PeriodicMolecules 1 in the model gives periodic-molecules = yes "
              "and grompp accepts it")
    off = dict(STAGES[0][2], interaction="    <PeriodicMolecules>0</PeriodicMolecules>")
    rc, out = run([os.path.join(PARSERS, "md.gmxtask")],
                  input=model(STAGES[0][1], **off).encode())
    check(rc == 0 and "periodic-molecules" not in out,
          "PeriodicMolecules 0 writes nothing")


def local_forcefield(gmx, home, tmp, includes_bin):
    """A force field kept beside the topology (#260): found by following the
    #include lines, staged flat, put back in its folder by the job script."""
    src = os.path.join(tmp, "localff")
    os.makedirs(os.path.join(src, "my.ff"))
    os.makedirs(os.path.join(src, "parts"))

    def put(rel, text):
        with open(os.path.join(src, rel), "w") as h:
            h.write(text)

    # ffnonbonded.itp is found next to the file that includes it, parts/
    # next to the topology and inside parts/ next to the including file.
    put("my.ff/forcefield.itp",
        "[ defaults ]\n1 2 yes 0.5 0.5\n\n"
        '#include "ffnonbonded.itp"\n')
    put("my.ff/ffnonbonded.itp",
        "[ atomtypes ]\n"
        "OW 8 15.9994 -0.82 A 3.16557e-01 6.50194e-01\n"
        "HW 1  1.008   0.41 A 0.0 0.0\n")
    put("my.ff/spc.itp",
        "[ moleculetype ]\nSOL 2\n\n[ atoms ]\n"
        "1 OW 1 SOL OW 1 -0.82\n2 HW 1 SOL HW1 1 0.41\n3 HW 1 SOL HW2 1 0.41\n\n"
        '#include "../parts/settles.itp"\n')
    put("parts/settles.itp",
        "[ settles ]\n1 1 0.1 0.16330\n\n"
        '#include "exclusions.itp"\n')
    put("parts/exclusions.itp",
        "[ exclusions ]\n1 2 3\n2 1 3\n3 1 2\n")
    top = os.path.join(src, "topol.top")
    put("topol.top",
        '#include "my.ff/forcefield.itp"\n'
        '#include "my.ff/spc.itp"\n'
        "#ifdef WITH_IONS\n"
        '#include "oplsaa.ff/ions.itp"\n'
        '#include "no-such-file.itp"\n'
        "#endif\n\n"
        "[ system ]\nThree waters\n\n[ molecules ]\nSOL 3\n")
    with open(os.path.join(FIXTURES, "conf.gro")) as h:
        lines = h.read().splitlines()
    gro = os.path.join(src, "conf.gro")
    with open(gro, "w") as h:
        h.write("Three SPC waters\n9\n%s\n%s\n"
                % ("\n".join(lines[2:11]), lines[-1]))

    extras = {}
    if includes_bin:
        rc, out = run([includes_bin, top])
        files = sorted(l.split(None, 1)[1] for l in out.splitlines()
                       if l.startswith("file "))
        check(files == ["my.ff__SLASH__ffnonbonded.itp",
                        "my.ff__SLASH__forcefield.itp",
                        "my.ff__SLASH__spc.itp",
                        "parts__SLASH__exclusions.itp",
                        "parts__SLASH__settles.itp"],
              "the topology's includes are followed recursively, relative to "
              "each including file and to the topology (%s)" % files)
        check("missing no-such-file.itp" in out.splitlines(),
              "an include found nowhere is reported as missing")
        check(not any("oplsaa.ff" in l for l in out.splitlines()
                      if l.startswith(("file", "missing"))),
              "GROMACS's own library include is left alone")
        for f in files:
            extras[f] = os.path.join(src, f.replace("__SLASH__", "/"))
    else:
        for rel in ("my.ff/forcefield.itp", "my.ff/ffnonbonded.itp",
                    "my.ff/spc.itp", "parts/settles.itp",
                    "parts/exclusions.itp"):
            extras[rel.replace("/", "__SLASH__")] = os.path.join(src, rel)

    name, calc, spec = STAGES[0]
    res = stage(gmx, home, tmp, "localff", calc, spec, gro, topology=top,
                extras=extras)
    check(res is not None,
          "a topology with a force-field folder beside it launches and runs")


# ---- Prepare: protein structure -> solvated, neutral system -----------------

def prepare(gmx, home, tmp, name, recipe, pdb, ext=".pdb", expect_ok=True):
    """md.gmxprep -> launchpp -> the job script gensub writes, for one recipe.
    Returns the run directory, or None when a step did not do what a
    successful run needs."""
    print("== prepare %s" % name)
    rundir = os.path.join(tmp, "run-prep-" + name)
    os.makedirs(rundir)
    rc, out = run([os.path.join(PARSERS, "md.gmxprep")], input=recipe.encode())
    if not check(rc == 0, "md.gmxprep wrote the recipe"):
        print(out)
        return None
    with open(os.path.join(rundir, "gromacs.gmxprep"), "w") as h:
        h.write(out)
    # the structure is staged as <system>.pdb or as a .gmxpdb
    shutil.copy(pdb, os.path.join(rundir, "ecceSession" + ext))
    with open(os.path.join(rundir, "postParams"), "w") as h:
        h.write("inputFile: gromacs.gmxprep\nrunDir: %s\n" % rundir)
    rc, out = run([os.path.join(PARSERS, "gromacs.launchpp"), "-p", "postParams"],
                  cwd=rundir)
    if expect_ok:
        check(rc == 0 and "accepted" in out, "gromacs.launchpp: pdb2gmx accepts the structure")
        if rc != 0:
            print(out)
            return None
    else:
        return rundir if rc != 0 and out else None
    params = os.path.join(rundir, "subParams")
    with open(params, "w") as h:
        h.write(" -Q Shell\n -H localtest\n -d localhost\n -c GROMACS\n"
                " -n 1\n -N 1\n -r %s\n -i gromacs.gmxprep\n -o gromacs.gmxlog\n"
                " -f %s/submit__prep\n -S ecceSession\n -C prepare\n"
                % (rundir, rundir))
    rc, out = run([os.path.join(home.home, "scripts", "gensub"), "-v", "-p", params],
                  env=home.env, cwd=rundir)
    if not check(rc == 0, "gensub wrote the Prepare job script"):
        print(out)
        return None
    rc, out = run(["sh", os.path.join(rundir, "submit__prep")], cwd=rundir)
    status = open(os.path.join(rundir, ".ecce.status")).read().strip()
    check(status == "0", "the Prepare job exits 0 (got %r)" % status)
    return rundir if status == "0" else None


def molecules(top):
    """[ molecules ] of a topology as {name: count}."""
    mols = {}
    on = False
    for line in open(top):
        line = line.split(";")[0].strip()
        if line.startswith("["):
            on = line.replace(" ", "") == "[molecules]"
        elif on and line:
            n, c = line.split()
            mols[n] = mols.get(n, 0) + int(c)
    return mols


def water_sites(rundir):
    """Atoms of residue SOL in the prepared structure."""
    gro = open(os.path.join(rundir, "ecceSession_prepare.gro")).read().splitlines()
    return sum(1 for l in gro[2:-1] if l[5:10].strip() == "SOL")


def check_prepared(gmx, rundir, maxwarn=0):
    """What a prepared system has to be, judged by gmx and by the files."""
    gro = os.path.join(rundir, "ecceSession_prepare.gro")
    top = os.path.join(rundir, "ecceSession.gmxtop")
    check(os.path.exists(gro) and os.path.exists(top),
          "the structure and topology are written under the names the next task looks for")
    pdb = os.path.join(rundir, "chemsys.pdb")
    check(os.path.exists(pdb) and "CRYST1" in open(pdb).read(),
          "chemsys.pdb for the Builder keeps the box (CRYST1)")
    out = open(os.path.join(rundir, "gromacs.gmxlog"), errors="replace").read()
    check(all(("ECCE-PREPARE step %d/4" % k) in out for k in (1, 2, 3, 4)),
          "the output file announces all four steps")
    natoms = int(open(gro).read().splitlines()[1])
    rc, chk = run([gmx, "check", "-f", gro])
    m = re.search(r"# Atoms\s+(\d+)", chk)
    check(rc == 0 and m and int(m.group(1)) == natoms,
          "gmx check reads the .gro (%d atoms)" % natoms)
    mols = molecules(top)
    check(any(k.startswith("Protein") for k in mols) and mols.get("SOL", 0) > 100,
          "the topology lists the protein and solvent (%s)" % mols)
    # grompp's own report of the net charge
    # (gmx goes by the extension: the .gmxtop is staged as topol.top)
    shutil.copy(os.path.join(rundir, "ions.mdp"), os.path.join(rundir, "chk.mdp"))
    shutil.copy(top, os.path.join(rundir, "chk.top"))
    rc, g = run([gmx, "grompp", "-f", "chk.mdp", "-c", gro, "-p", "chk.top",
                 "-o", "chk.tpr", "-maxwarn", str(maxwarn)], cwd=rundir)
    check(rc == 0 and "non-zero total charge" not in g,
          "grompp finds the prepared system neutral")
    return mols, natoms


def prepare_tests(gmx, home, tmp, optimize_stage):
    pdb = os.path.join(FIXTURES, "1l2y.pdb")
    rcp = "SOURCE=protein\nFF=amber99sb-ildn\nWATER=tip3p\nBOX=dodecahedron\nDIST=1.0\n"

    # the recipe writer's own checks
    rc, out = run([os.path.join(PARSERS, "md.gmxprep")], input=b"")
    check(rc == 0 and "FF=amber99sb-ildn" in out and "CONC=0.15" in out,
          "md.gmxprep with no input writes the defaults")
    for bad in ("FF=a;touch x\n", "BOX=hexagon\n", "DIST=-1\n", "CONC=lots\n",
                "WATER=tip3p extra\n", "NOPE=1\n", "HIS=names\n"):
        rc, out = run([os.path.join(PARSERS, "md.gmxprep")], input=bad.encode())
        check(rc != 0, "md.gmxprep refuses %r" % bad.strip())

    xml = ("<?xml version=\"1.0\"?>\n<GromacsPrepareModel>\n"
           "<ForceField>charmm27</ForceField>\n<WaterModel>tip3p</WaterModel>\n"
           "<Histidine>auto</Histidine>\n<BoxDistance>1.2</BoxDistance>\n"
           "</GromacsPrepareModel>\n")
    rc, out = run([os.path.join(PARSERS, "md.gmxprep")], input=xml.encode())
    check(rc == 0 and "FF=charmm27" in out and "DIST=1.2" in out and "CONC=0.15" in out,
          "md.gmxprep reads the task model XML")

    # neutralise only: Trp-cage is +1, so one CL and no sodium
    d = prepare(gmx, home, tmp, "neutral", rcp + "CONC=0\n", pdb)
    if d:
        mols, natoms = check_prepared(gmx, d)
        check(mols.get("CL") == 1 and "NA" not in mols,
              "CONC=0 neutralises with exactly one CL (%s)" % mols)
        print("  1L2Y: %d atoms, %d waters, ions %s"
              % (natoms, mols.get("SOL", 0),
                 dict((k, v) for k, v in mols.items() if k in ("NA", "CL"))))
        # the outputs go on through the Optimize stage
        name, calc, spec = optimize_stage
        res = stage(gmx, home, tmp, "prep-optimize", calc, spec,
                    os.path.join(d, "ecceSession_prepare.gro"),
                    topology=os.path.join(d, "ecceSession.gmxtop"),
                    extras={"posre.itp": os.path.join(d, "posre.itp")})
        if res:
            pot = gmx_energy(gmx, res, "Potential")
            check(len(pot) >= 2 and all(abs(p) < 1e9 for p in pot) and pot[-1] < pot[0],
                  "Optimize on the prepared system lowers a finite potential "
                  "energy (%.4g -> %.4g)" % (pot[0], pot[-1]) if pot else
                  "Optimize on the prepared system wrote energies")

    # a salt concentration: counter-ions and neutralising ions together
    d = prepare(gmx, home, tmp, "salt", rcp + "CONC=0.15\n", pdb, ext=".gmxpdb")
    if d:
        mols, natoms = check_prepared(gmx, d)
        check(mols.get("NA", 0) >= 1 and mols.get("CL", 0) == mols.get("NA", 0) + 1,
              "CONC=0.15 adds salt and one extra CL for the +1 charge (%s)" % mols)

    # a GROMOS force field and the SPC water box
    d = prepare(gmx, home, tmp, "gromos",
                "FF=gromos54a7\nWATER=spc\nBOX=cubic\nDIST=1.0\nCONC=0\n", pdb)
    if d:
        mols, natoms = check_prepared(gmx, d, maxwarn=1)
        check(water_sites(d) == 3 * mols.get("SOL", 0),
              "gromos54a7 with spc builds on the three-site water box")
        # the MD tasks after it need grompp -maxwarn 1 for the twin-range warning
        name, calc, spec = optimize_stage
        res = stage(gmx, home, tmp, "gromos-optimize", calc, spec,
                    os.path.join(d, "ecceSession_prepare.gro"),
                    topology=os.path.join(d, "ecceSession.gmxtop"),
                    extras={"posre.itp": os.path.join(d, "posre.itp")})
        check(res is not None,
              "a task after a GROMOS Prepare launches and runs (grompp -maxwarn 1)")

    # the four-site water box is chosen for a four-site model
    d = prepare(gmx, home, tmp, "tip4p", rcp + "WATER=tip4p\nCONC=0\n", pdb)
    if d:
        check_prepared(gmx, d)
        check(water_sites(d) == 4 * molecules(
            os.path.join(d, "ecceSession.gmxtop")).get("SOL", 0),
              "WATER=tip4p solvates with the four-site box")

    # a failure: launch stops with pdb2gmx's own words ...
    broken = os.path.join(FIXTURES, "1l2y_broken.pdb")
    rd = os.path.join(tmp, "run-prep-broken")
    os.makedirs(rd)
    rc, out = run([os.path.join(PARSERS, "md.gmxprep")], input=rcp.encode())
    open(os.path.join(rd, "gromacs.gmxprep"), "w").write(out)
    shutil.copy(broken, os.path.join(rd, "ecceSession.pdb"))
    open(os.path.join(rd, "postParams"), "w").write("inputFile: gromacs.gmxprep\n")
    rc, out = run([os.path.join(PARSERS, "gromacs.launchpp"), "-p", "postParams"],
                  cwd=rd)
    check(rc == 0 and out.startswith("ECCE-LAUNCH-WARNING") and "is missing in residue TRP 6" in out
          and "missing atoms in molecule" in out,
          "a side chain cut short is reported to the Launcher with pdb2gmx's message")
    # ... and the job script, if it is run anyway, puts them in the output
    with open(os.path.join(rd, "subParams"), "w") as h:
        h.write(" -Q Shell\n -H localtest\n -d localhost\n -c GROMACS\n"
                " -n 1\n -N 1\n -r %s\n -i gromacs.gmxprep\n -o gromacs.gmxlog\n"
                " -f %s/submit__prep\n -S ecceSession\n -C prepare\n" % (rd, rd))
    run([os.path.join(home.home, "scripts", "gensub"), "-v", "-p",
         os.path.join(rd, "subParams")], env=home.env, cwd=rd)
    run(["sh", os.path.join(rd, "submit__prep")], cwd=rd)
    status = open(os.path.join(rd, ".ecce.status")).read().strip()
    glog = open(os.path.join(rd, "gromacs.gmxlog"), errors="replace").read()
    check(status != "0" and "Fatal error" in glog
          and "ECCE-PREPARE step 1/4 pdb2gmx" in glog
          and "step 2/4" not in glog,
          "the job script stops at the failed step and leaves gmx's output "
          "in the output file (status %s)" % status)


def main():
    global verbose
    ap = argparse.ArgumentParser()
    ap.add_argument("-v", action="store_true")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--includes-bin", help="the gromacs_includes binary")
    args = ap.parse_args()
    verbose = args.v

    gmx = find_gmx()
    if gmx is None:
        if os.environ.get("ECCE_GROMACS_REQUIRE"):
            print("FAIL: gmx is not installed and ECCE_GROMACS_REQUIRE is set")
            return 1
        print("SKIP: gmx is not installed")
        return 77

    base = os.path.expanduser("~/.cache")
    os.makedirs(base, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix="gromacs-md-", dir=base)
    try:
        home = Home(tmp, gmx)
        gro = os.path.join(FIXTURES, "conf.gro")
        results = {}
        for name, calc, spec in STAGES:
            res = stage(gmx, home, tmp, name, calc, spec, gro)
            if res is None:
                break
            results[name] = res
            gro = res["gro"]
            res["props"] = properties(res, tmp, name)

        # ---- what each stage has to have done -------------------------
        if "optimize" in results:
            r = results["optimize"]
            check(r["mdp"].get("integrator") == "steep"
                  and r["mdp"].get("emtol") == "100"
                  and r["mdp"].get("nsteps") == "200",
                  "Optimize is steepest descent with the tolerance and step count from the model")
            te = r["props"].get("TE")
            check(bool(te), "TE is extracted")
            pot = gmx_energy(gmx, r, "Potential")
            check(len(pot) >= 2 and pot[-1] < pot[0],
                  "the minimisation lowered the potential energy (gmx energy)")
            if te and pot:
                check(abs(last(r["props"], "TE") - pot[-1]) < 0.5,
                      "the TE ECCE parsed is the last Potential gmx energy reports "
                      "(%g vs %g)" % (last(r["props"], "TE"), pot[-1]))
        if "nvt" in results:
            r = results["nvt"]
            m = r["mdp"]
            check(m.get("tcoupl") == "v-rescale" and m.get("ref-t") == "300"
                  and m.get("gen-vel") == "yes" and "pcoupl" not in m,
                  "an NVT task makes new velocities, couples to 300 K and has no barostat")
            temps = gmx_energy(gmx, r, "Temperature")
            check(len(temps) > 2 and 150 < temps[-1] < 450,
                  "the system is near the target temperature (gmx energy: %.0f K)"
                  % (temps[-1] if temps else -1))
            tv = r["props"].get("TEMPVEC", [])
            check(len(tv) >= 2, "TEMPVEC accumulated (%d points)" % len(tv))
            if tv and temps:
                check(abs(last(r["props"], "TEMPVEC") - temps[-1]) < 1.0,
                      "TEMPVEC's last point is gmx energy's Temperature")
        if "npt" in results:
            r = results["npt"]
            m = r["mdp"]
            check(m.get("pcoupl") == "C-rescale" and m.get("ref-p") == "1.01325"
                  and m.get("compressibility") == "4.53e-05"
                  and m.get("gen-vel") == "no" and m.get("continuation") == "yes",
                  "an NPT task continues, and the pressure is in bar and the "
                  "compressibility in 1/bar")
            check(bool(r["props"].get("PRESSURE")) and
                  len(r["props"].get("PRESSVEC", [])) >= 2,
                  "PRESSURE and PRESSVEC are extracted")
        if "dynamics" in results:
            r = results["dynamics"]
            check(r["mdp"].get("nstxout-compressed") == "50"
                  and os.path.exists(os.path.join(r["rundir"], "traj.xtc")),
                  "the trajectory request reaches mdrun (traj.xtc written)")
            te = gmx_energy(gmx, r, "Total-Energy")
            if te and r["props"].get("TE"):
                check(abs(last(r["props"], "TE") - te[-1]) < 0.5,
                      "TE is Total-Energy as gmx energy reports it (%g vs %g)"
                      % (last(r["props"], "TE"), te[-1]))

        # ---- a topology with position restraints (#253) ----------------
        #  grompp refuses [ position_restraints ] without a reference
        #  structure (-r); three SPC waters, the first one held in place.
        restrained(gmx, home, tmp)
        periodic_molecules(gmx, home, tmp, results)

        # ---- includes kept beside the topology (#260) ------------------
        local_forcefield(gmx, home, tmp, args.includes_bin)

        # ---- Prepare: PDB -> solvated, neutral system -------------------
        prepare_tests(gmx, home, tmp, STAGES[0])

        # ---- an input mistake is reported, plainly, at launch ----------
        rd = os.path.join(tmp, "run-broken")
        os.makedirs(rd)
        for f in ("conf.gro", "topol.top"):
            shutil.copy(os.path.join(FIXTURES, f), rd)
        with open(os.path.join(rd, "topol.top"), "a") as h:
            h.write("SOL   5\n")
        shutil.copy(os.path.join(results["optimize"]["rundir"], "gromacs.mdp")
                    if "optimize" in results else os.devnull, rd) \
            if "optimize" in results else None
        with open(os.path.join(rd, "postParams"), "w") as h:
            h.write("inputFile: gromacs.mdp\n")
        if os.path.exists(os.path.join(rd, "gromacs.mdp")):
            rc, out = run([os.path.join(PARSERS, "gromacs.launchpp"), "-p",
                           "postParams"], cwd=rd)
            check(rc == 0 and out.startswith("ECCE-LAUNCH-WARNING") and "does not match" in out,
                  "a topology with the wrong number of molecules is reported to the "
                  "Launcher in grompp's own words")
        rd2 = os.path.join(tmp, "run-missing")
        os.makedirs(rd2)
        open(os.path.join(rd2, "gromacs.mdp"), "w").write("integrator = md\n")
        open(os.path.join(rd2, "postParams"), "w").write("inputFile: gromacs.mdp\n")
        rc, out = run([os.path.join(PARSERS, "gromacs.launchpp"), "-p", "postParams"],
                      cwd=rd2)
        check(rc != 0 and "topology" in out and "starting structure" in out,
              "missing topology and structure are named in the message")

    finally:
        if args.keep:
            print("kept: %s" % tmp)
        else:
            shutil.rmtree(tmp, ignore_errors=True)

    print("%s" % ("FAILED: %d" % len(failures) if failures else "PASSED"))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
