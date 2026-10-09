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
          topology=os.path.join(FIXTURES, "topol.top")):
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


def main():
    global verbose
    ap = argparse.ArgumentParser()
    ap.add_argument("-v", action="store_true")
    ap.add_argument("--keep", action="store_true")
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
            check(rc != 0 and "does not match" in out,
                  "a topology with the wrong number of molecules stops the launch "
                  "and grompp's own words are in the message")
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
