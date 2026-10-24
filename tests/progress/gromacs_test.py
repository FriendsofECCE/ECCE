#!/usr/bin/env python3
"""GROMACS plots in the Progress box (#477): real short gmx runs, then the real
eccejobmonitor over their log (parse file) and stderr (jobOutputFile2), as a
job's monitor reads them, and the snapshot it sends.

  - minimisation: potential energy and maximum force per step, the force
    against the em tolerance (needs mdrun -v, which gensub passes);
  - dynamics (NPT): potential energy, temperature, pressure and density per
    log step; the run averages that end the log are not a point.

    tests/progress/gromacs_test.py     SKIP (77) without gmx and mpirun/perl.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
MONITOR = os.path.join(ROOT, "scripts", "eccejobmonitor")
DESC = os.path.join(ROOT, "scripts", "parsers", "gromacs.desc")
CONF = os.path.join(ROOT, "tests", "gromacs_md", "fixtures", "dodeca_water.gro")

failures = []


def check(ok, what):
    print("%s %s" % ("ok   " if ok else "FAIL ", what))
    if not ok:
        failures.append(what)


def gmx_command():
    for name in ("gmx", "gmx_mpi"):
        path = shutil.which(name)
        if path:
            return [path], name.endswith("_mpi")
    return None, False


def run(cmd, cwd, out=None, err=None):
    return subprocess.run(cmd, cwd=cwd, stdin=subprocess.DEVNULL,
                          stdout=out or subprocess.PIPE,
                          stderr=err or subprocess.STDOUT, timeout=300)


def monitor(work, parse, second, name):
    results = os.path.join(work, name + ".msg")
    cmd = ["perl", MONITOR, "-commType", "stdio", "-monitoringMode", "post",
           "-jobOutputFile", parse, "-jobOutputFile2", second,
           "-parseDescriptorFile", DESC, "-parseTypes", "ALL",
           "-outputFile", results, "-calcName", "p",
           "-logFile", os.path.join(work, name + ".mon"),
           "-filesToTransfer", "", "-importDir", work + os.sep, "-progress", "1"]
    p = subprocess.run(cmd, cwd=work, stdin=subprocess.DEVNULL,
                       capture_output=True, text=True, timeout=120)
    if p.returncode:
        print(p.stdout[-600:], p.stderr[-600:], open(os.path.join(work, name + ".mon")).read()[-800:] if os.path.exists(os.path.join(work, name + ".mon")) else "")
    check(p.returncode == 0, "%s: monitor ran (rc=%d)" % (name, p.returncode))
    if p.returncode:
        return ["" ]
    raw = open(results, "rb").read().decode("utf-8", "replace")
    msgs, i = [], 0
    while True:
        i = raw.find("\001", i)
        if i < 0:
            break
        length = int(raw[i + 2:i + 6])
        payload = raw[i + 6:i + 6 + length]
        i += 6 + length
        if payload.startswith("jmPROG") or "PROG" in payload[:8]:
            msgs.append(payload.replace("\002", "\n"))
    return msgs[-1:] or [""]


def history(snapshot, stage):
    m = re.search(r"^%s~ \w+=(\S+)$" % re.escape(stage), snapshot, re.M)
    return [float(v) for v in m.group(1).split(",")] if m else []


def main():
    gmx, mpi = gmx_command()
    if not gmx or not shutil.which("perl") or (mpi and not shutil.which("mpirun")):
        print("SKIP: needs gmx, perl (and mpirun for gmx_mpi)")
        return 77
    work = tempfile.mkdtemp(prefix="ecce-gmxprogress-")
    try:
        shutil.copy(CONF, os.path.join(work, "conf.gro"))
        n = int(open(CONF).read().split("\n")[1]) // 3
        with open(os.path.join(work, "topol.top"), "w") as h:
            h.write('#include "oplsaa.ff/forcefield.itp"\n#include "oplsaa.ff/spc.itp"\n'
                    "[ system ]\nw\n[ molecules ]\nSOL %d\n" % n)
        common = "cutoff-scheme=Verlet\ncoulombtype=PME\nrcoulomb=0.8\nrvdw=0.8\nnstlog=%d\n"
        with open(os.path.join(work, "em.mdp"), "w") as h:
            h.write("integrator=steep\nemtol=100\nemstep=0.01\nnsteps=40\n" + common % 1)
        with open(os.path.join(work, "md.mdp"), "w") as h:
            h.write("integrator=md\ndt=0.002\nnsteps=400\nconstraints=h-bonds\n"
                    "tcoupl=v-rescale\ntc-grps=System\ntau-t=0.1\nref-t=300\n"
                    "pcoupl=c-rescale\ntau-p=1\ncompressibility=4.5e-5\nref-p=1\n"
                    "gen-vel=yes\ngen-temp=300\nnstcalcenergy=10\nnstenergy=50\n" + common % 50)

        def mdrun(tpr, log, conf, verbose, name):
            cmd = (["mpirun", "-np", "1"] if mpi else []) + gmx + ["mdrun"] + \
                  (["-v"] if verbose else []) + ["-s", tpr, "-g", log, "-c", conf, "-e", log[:-4] + ".edr"]
            cmd += ["-ntomp", "1"] if mpi else ["-ntmpi", "1", "-ntomp", "1"]
            with open(os.path.join(work, name + ".out"), "wb") as o:
                if verbose:
                    o.write(b"ECCE-EMTOL 100\n")     # what gensub echoes
                    o.flush()
                return run(cmd, work, out=o, err=subprocess.STDOUT).returncode

        for mdp, base, c, v in (("em", "em", "conf.gro", True), ("md", "md", "em.gro", False)):
            g = run(gmx + ["grompp", "-f", mdp + ".mdp", "-c", c, "-p", "topol.top",
                           "-o", base + ".tpr", "-maxwarn", "9"], work)
            if g.returncode:
                print(g.stdout.decode()[-800:])
            check(g.returncode == 0, base + ": grompp")
            check(mdrun(base + ".tpr", base + ".log", base + ".gro", v, base) == 0,
                  base + ": mdrun")

        # What gensub's density helper makes of the energy file
        subprocess.run("echo Density | %s energy -f md.edr -o dens.xvg" % " ".join(gmx),
                       shell=True, cwd=work, capture_output=True)
        with open(os.path.join(work, "md.out"), "ab") as o:
            for l in open(os.path.join(work, "dens.xvg")):
                if l[0] not in "#@":
                    o.write(("ECCE-DENSITY %s\n" % " ".join(l.split()[:2])).encode())

        # Minimisation: log, and stderr with its carriage returns
        snap = monitor(work, os.path.join(work, "em.log"), os.path.join(work, "em.out"), "em")[0]
        print(snap)
        ep, fm = history(snap, "gmx-epot"), history(snap, "gmx-fmax")
        check(len(ep) >= 10 and ep[-1] < ep[0], "em: potential energy per step, falling (%d points)" % len(ep))
        check(len(fm) >= 10 and fm[-1] < fm[0], "em: maximum force per step, falling (%d points)" % len(fm))
        check(re.search(r"^gmx-fmax step=\d+ maxgrad=\S+/100(\.0)?$", snap, re.M) is not None,
              "em: the force comes with the em tolerance")

        # As a real job: the declared output file is the log alone while the
        # job runs (mdrun's stderr and the ECCE-* lines are in ecce.submit.log,
        # appended to the declared file only at the end), and it is the
        # monitor's only file.  Points must show mid-run, and not double up
        # once the submit log has been appended.
        live = os.path.join(work, "live")
        os.mkdir(live)
        sub = open(os.path.join(work, "em.out"), "rb").read()
        half = sub[:sub.rfind(b"\r", 0, len(sub) // 2) + 1]
        shutil.copy(os.path.join(work, "em.log"), os.path.join(live, "gromacs.gmxlog"))
        open(os.path.join(live, "ecce.submit.log"), "wb").write(half)
        mid = monitor(live, os.path.join(live, "gromacs.gmxlog"),
                      os.path.join(live, "gromacs.gmxlog"), "mid")[0]
        mid_ep = history(mid, "gmx-epot")
        check(len(mid_ep) >= 5 and len(mid_ep) < len(ep),
              "em, mid-run: %d of %d points from the submit log alone" % (len(mid_ep), len(ep)))
        open(os.path.join(live, "ecce.submit.log"), "wb").write(sub)
        with open(os.path.join(live, "gromacs.gmxlog"), "ab") as o:
            o.write(b"\n-----ECCE Log Information-----\n" + sub)
        end = monitor(live, os.path.join(live, "gromacs.gmxlog"),
                      os.path.join(live, "gromacs.gmxlog"), "end")[0]
        check(history(end, "gmx-epot") == ep,
              "em, job over: the appended submit log adds no second set of points")

        # Dynamics
        snap = monitor(work, os.path.join(work, "md.log"), os.path.join(work, "md.out"), "md")[0]
        print(snap)
        for stage in ("gmx-epot", "gmx-temp", "gmx-pres", "gmx-dens"):
            h = history(snap, stage)
            check(len(h) == 9, "md: %s has %d points (400 steps, log every 50, plus step 0)" % (stage, len(h)))
        t = history(snap, "gmx-temp")
        check(t and 150 < t[-1] < 450, "md: temperature near 300 K (%s)" % (t[-1:] or None))
        d = history(snap, "gmx-dens")
        check(d and 700 < d[-1] < 1300, "md: density of water-like order (%s)" % (d[-1:] or None))
        check("gmx-epot step=400 " in snap, "md: last point is step 400 (the averages are not a point)")
    finally:
        if not os.environ.get("KEEP"):
            shutil.rmtree(work, ignore_errors=True)
        else:
            print("kept", work)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
