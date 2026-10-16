#!/usr/bin/env python3
"""Compares the .mdp the task editor can make with a real workflow's .mdp files.

The four tasks of a clay workflow (EM, NVT, NPT, MD) are set up the way the
editor stores them, run through md.gmxtask, and the result is compared line
by line with the author's .mdp after normalising (comments dropped, - and _
alike, case and spacing ignored, numbers compared as numbers).  What stays
different is listed.  Then grompp runs on the author's topology with ECCE's
.mdp and the index file gromacs.mkndx writes from the structure.

The workflow's files are not part of this repository: give their directory
(the one holding EM/em.mdp, ... and gmx_minff/) in ECCE_GMX_ORACLE.

    ECCE_GMX_ORACLE=/path/to/minff_MD tests/gromacs_md/oracle_compare.py
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import run_tests as rt  # noqa: E402
from run_settings_test import tag  # noqa: E402

PARSERS = rt.PARSERS
ORACLE = os.environ.get("ECCE_GMX_ORACLE")

GROUPS = "MMT = MMT\nWater_and_ions = SOL Cu"
DEFINES = "-Dcis_Oct_Mg2cis_Fe3cis_k500 -DOPC3_IOD_LM"

# keys the pages have no widget for: these go in the extra-lines field
COMMON_EXTRA = ["verlet-buffer-tolerance = 0.005", "fourierspacing = 0.16",
                "DispCorr = EnerPres", "ns_type = grid"]


def interaction(cutoff, nstlist, defines, posres, extra, groups=False):
    s = (tag("ShortRangeCutoff", cutoff) + tag("ShortListUpdateFrequency", nstlist)
         + tag("PeriodicMolecules", 1) + tag("GmxDefines", defines))
    if posres:
        s += tag("GmxPosres", 1) + tag("GmxPosresName", "POSRES_noH")
    if groups:
        s += tag("GmxGroups", GROUPS) + tag("GmxComGroups", 1)
    return s + tag("GmxExtraMdp", "\n".join(extra))


def dyn(resume, steps, dt, nstcomm, tau_t, npt=False, tau_p=2):
    s = ("    <ResumeOpt> %d</ResumeOpt>\n    <EquilibrationSteps> 0</EquilibrationSteps>\n"
         "    <TimeStep> %g</TimeStep>\n    <DataSteps> %d</DataSteps>\n"
         "    <RemoveCM> 1</RemoveCM>\n    <RemoveCMFreq> %d</RemoveCMFreq>\n"
         "    <UseNVT> 1</UseNVT>\n    <NVTTemperature> 298</NVTTemperature>\n"
         "    <SolventRxTime> %g</SolventRxTime>\n    <UseAnnealing> 0</UseAnnealing>\n"
         % (resume, dt, steps, nstcomm, tau_t))
    if npt:
        s += ("    <UseNPT> 1</UseNPT>\n    <NPTPressure> 100000</NPTPressure>\n"
              "    <NPTRxTime> %g</NPTRxTime>\n    <Compressibility> 4.5e-10</Compressibility>\n"
              % tau_p)
    else:
        s += "    <UseNPT> 0</UseNPT>\n"
    return s


def files(nlog, nxtc):
    return ("    <PrintMD> 1</PrintMD>\n    <PrintMDFreq> %d</PrintMDFreq>\n"
            "    <WriteTrajectory> 1</WriteTrajectory>\n"
            "    <CoordinatesSoluteFreq> %d</CoordinatesSoluteFreq>\n" % (nlog, nxtc))


def tasks():
    out = {}
    out["EM"] = (rt.model(
        "ecceMdOptimize",
        interaction=interaction(1.0, 1, DEFINES + " -DFLEXIBLE", True,
                                ["nstxout = 100", "constraint_algorithm = lincs",
                                 "lincs_iter = 1", "lincs_order = 4", "ns_type = grid",
                                 "rlist = 1.0"]),
        optimize="    <UseSD>1</UseSD>\n    <SDMaxIterations>1000</SDMaxIterations>\n"
                 "    <SDInitialStepSize>0.01</SDInitialStepSize>\n"
                 "    <SDTolerance>500</SDTolerance>\n    <UseCG>0</UseCG>\n"
                 "    <CGMaxIterations>100</CGMaxIterations>\n").replace(
        "<UseSHAKE>1", "<UseSHAKE>0"), "EM/em.mdp")
    zero = ["nstxout = 0", "nstvout = 0", "nstfout = 0"]
    out["NVT"] = (rt.model(
        "ecceMdEquilibrate",
        interaction=interaction(1.2, 20, DEFINES + " -DFLEXIBLE", True,
                                COMMON_EXTRA + zero + ["rlist = 1.0",
                                "refcoord-scaling = com", "title = NVT equilibration"],
                                groups=True),
        dynamics=dyn(0, 50000, 0.0001, 10, 2),
        files=files(100, 1000)).replace("<UseSHAKE>1", "<UseSHAKE>0"), "NVT/nvt.mdp")
    out["NPT"] = (rt.model(
        "ecceMdEquilibrate",
        interaction=interaction(1.2, 20, DEFINES, False,
                                COMMON_EXTRA + zero + ["rlist = 1.2",
                                "refcoord-scaling = com", "title = NPT equilibration"],
                                groups=True)
        + tag("GmxBarostat", 1) + tag("GmxPcoupltype", 2) + tag("GmxNstpcouple", 20),
        dynamics=dyn(1, 100000, 0.001, 1000, 1, npt=True, tau_p=2),
        files=files(100, 1000)).replace("<UseSHAKE>1", "<UseSHAKE>0"), "NPT/npt.mdp")
    out["MD"] = (rt.model(
        "ecceMdDynamics",
        interaction=interaction(1.2, 20, DEFINES, False,
                                COMMON_EXTRA + zero + ["rlist = 1.0",
                                "refcoord-scaling = com", "title = MD equilibration"],
                                groups=True),
        dynamics=dyn(1, 1000000, 0.001, 1000, 2),
        files=files(1000, 1000)).replace("<UseSHAKE>1", "<UseSHAKE>0"), "MD/md.mdp")
    return out


def normalise(text):
    d = {}
    for line in text.splitlines():
        line = line.split(";")[0]
        m = re.match(r"^\s*(\S+?)\s*=\s*(.*?)\s*$", line)
        if m:
            d[m.group(1).lower().replace("_", "-")] = " ".join(m.group(2).lower().split())
    return d


def same(a, b):
    if a == b:
        return True
    try:
        return [float(x) for x in a.split()] == [float(x) for x in b.split()]
    except ValueError:
        return False


def main():
    if not ORACLE or not os.path.isdir(ORACLE):
        print("SKIP: set ECCE_GMX_ORACLE to the workflow directory")
        return 77
    mdps = {}
    for name, (xml, rel) in tasks().items():
        rc, mdp = rt.run([os.path.join(PARSERS, "md.gmxtask")], input=xml.encode())
        if rc != 0:
            print("%s: md.gmxtask failed\n%s" % (name, mdp))
            return 1
        mdps[name] = mdp
        mine = normalise(mdp)
        theirs = normalise(open(os.path.join(ORACLE, rel)).read())
        print("== %s   (%s)" % (name, rel))
        for k in sorted(set(mine) | set(theirs)):
            if k in mine and k in theirs:
                if not same(mine[k], theirs[k]):
                    print("  DIFFERS  %-24s ECCE: %-34s theirs: %s" % (k, mine[k], theirs[k]))
            elif k in mine:
                print("  ECCE ONLY %-23s %s" % (k, mine[k]))
            else:
                print("  THEIRS ONLY %-21s %s" % (k, theirs[k]))

    # grompp on their topology with our .mdp and our index file
    gmx = rt.find_gmx()
    if gmx is None:
        print("no gmx: grompp not run")
        return 0
    base = os.environ.get("XDG_CACHE_HOME") or os.path.expanduser("~/.cache")
    os.makedirs(base, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix="gromacs-oracle-", dir=base)
    try:
        src = os.path.join(ORACLE, "gmx_minff")
        for f in os.listdir(src):
            p = os.path.join(src, f)
            if f == "min.ff":
                shutil.copytree(p, os.path.join(tmp, f))
            elif os.path.isfile(p) and f.split(".")[-1] in ("top", "itp", "gro", "ndx"):
                shutil.copy(p, tmp)
        for name, mdp in mdps.items():
            open(os.path.join(tmp, name + ".mdp"), "w").write(mdp)
        print("== index file")
        rc, out = rt.run([os.path.join(PARSERS, "gromacs.mkndx"),
                          os.path.join(tmp, "preem.gro"), os.path.join(tmp, "NVT.mdp"),
                          os.path.join(tmp, "ecce.ndx")])
        print(out.strip() or "written")
        mine = ndx(os.path.join(tmp, "ecce.ndx"))
        theirs = ndx(os.path.join(tmp, "index.ndx"))
        for g in ("MMT", "Water_and_ions"):
            print("  %-15s ECCE %6d atoms   theirs %6d atoms   %s" % (
                g, len(mine.get(g, [])), len(theirs.get(g, [])),
                "same atoms" if mine.get(g) == theirs.get(g) else "DIFFERENT atoms"))
        for name in mdps:
            for maxwarn in (0, 9):
                rc, out = rt.run([gmx, "grompp", "-f", name + ".mdp", "-c", "preem.gro",
                                  "-r", "preem.gro", "-p", "topol.top", "-n", "ecce.ndx",
                                  "-o", name + ".tpr", "-maxwarn", str(maxwarn)], cwd=tmp)
                warns = re.findall(r"^WARNING \d+ \[[^\]]*\]:\n(.*?)\n\n", out, re.M | re.S)
                print("== grompp %s -maxwarn %d: rc=%d, %d warning(s)" % (name, maxwarn, rc, len(warns)))
                if maxwarn == 0:
                    for w in warns:
                        print("     " + " ".join(w.split())[:150])
                    for e in re.findall(r"^ERROR \d+ \[[^\]]*\]:\n(.*?)\n\n", out, re.M | re.S):
                        print("     ERROR " + " ".join(e.split())[:150])
                if rc == 0 and maxwarn == 9:
                    pass
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    return 0


def ndx(path):
    out, cur = {}, None
    for line in open(path):
        m = re.match(r"^\[\s*(\S+)\s*\]", line)
        if m:
            cur = m.group(1)
            out[cur] = []
        elif cur:
            out[cur] += [int(x) for x in line.split()]
    return out


if __name__ == "__main__":
    sys.exit(main())
