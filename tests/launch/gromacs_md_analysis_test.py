#!/usr/bin/env python3
"""Speed Distribution and Displacement data for every GROMACS run (#877).

Liquid argon (the General Chemistry example's topology and structure, 1000
atoms) runs 500 steps with nstvout through the real Launch, gensub, job
monitor and job store, on 2 threads:

  * MDMSD and MDSPEED are stored, and the mean argon speed is near the
    Maxwell-Boltzmann mean at the reference temperature;
  * the job script's own analysis function, run again in a copy of the run
    directory without velocities, writes the MDSPEED record "none" (the
    Speed panel then says why it has no curve), and without the program
    only logs a line;
  * the run directory imported with the Organizer hook gmximport gets the
    same two properties, and a copy without velocities gets "none";
  * the Builder's panels (script command mdcsv) report the curves.

    tests/launch/gromacs_md_analysis_test.py [--build build-cmake] [--keep]

Exit status 77 (CTest SKIP) without gmx, Xvfb or the built binaries.
"""
import argparse
import glob
import math
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402

AR = os.path.join(REPO, "data", "client", "examples", "genchem",
                  "gases-liquids-solids", "gromacs-ar-liquid", "1-dynamics")
REF_T = 85.0
MDP = """integrator = md
dt = 0.002
nsteps = 500
cutoff-scheme = Verlet
nstlist = 20
coulombtype = Cut-off
rcoulomb = 1.2
vdwtype = Cut-off
vdw-modifier = Potential-shift
rvdw = 1.2
DispCorr = EnerPres
pbc = xyz
tcoupl = v-rescale
tc-grps = System
tau-t = 1.0
ref-t = %g
pcoupl = no
gen-vel = yes
gen-temp = %g
gen-seed = 877
nstcalcenergy = 50
nstenergy = 50
nstlog = 100
nstxout-compressed = 10
""" % (REF_T, REF_T)
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def where(u):
    return u[len("file://"):].rstrip("/")


def records(task, key):
    """The strings of a stored PropVecString, or None."""
    for path in glob.glob(os.path.join(task, "Props", key + "*")):
        text = open(path, errors="replace").read()
        body = text.split(">", 2)[-1].rsplit("</vecstring>", 1)[0]
        return body.split()
    return None


def waitProps(task, keys, seconds=120):
    """File= properties are stored after the rest; wait for all of keys."""
    end = time.time() + seconds
    while time.time() < end:
        if all(records(task, k) is not None for k in keys):
            return True
        time.sleep(1)
    return False


def stop(proc):
    if proc is not None and proc.poll() is None:
        try:
            os.killpg(proc.pid, 15)
            proc.wait(timeout=20)
        except Exception:
            try:
                os.killpg(proc.pid, 9)
            except Exception:
                pass


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    gmx = shutil.which("gmx")
    if not gmx:
        harness.skip("gmx is not installed")
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    for exe in ("mdanalysis", "organizer", "builder"):
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import builder_overlay_test as bo
    import xdisplay

    s = harness.Session(build, "gmxmdanalysis", {"GROMACS": gmx}, local=True,
                        keep=args.keep)
    # The job script runs $ECCE_HOME/bin/mdanalysis, as an installed ECCE has it.
    for exe in ("mdanalysis",) + bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = org = None
    try:
        s.registerMachine("localhost", "Shell", {"GROMACS": gmx})
        if not check(s.services(True), "services came up"):
            return 1
        rc, out = s.driver("gromacsstudy", s.userUrl(), "argon")
        urls = [l for l in out.splitlines() if l.startswith("file://")]
        if not check(rc == 0 and len(urls) == 4, "a GROMACS study was created"):
            return 1
        study, opt, eq, dyn = [u.rstrip("/") + "/" for u in urls]
        rundir = os.path.join(s.state, "run")
        check(s.driver("mdsetup", dyn, "localhost", rundir, s.user(), "queue=normal",
                       "procs=2")[0] == 0, "launch settings stored")
        check(s.driver("mdfile", dyn, os.path.join(AR, "topol.top"), "topol.gmxtop")[0] == 0
              and s.driver("mdfile", dyn, os.path.join(AR, "confout.gro"), "start.gro")[0] == 0,
              "topology and structure attached")
        check(s.driver("mdmdp", dyn)[0] == 0, ".mdp written")
        with open(os.path.join(where(dyn), "Inputs", "gromacs.mdp"), "w") as h:
            h.write(MDP + "nstvout = 10\n")
        t0 = time.time()
        check(s.launch(dyn)[0] == 0, "Dynamics submitted")
        state = s.waitState(dyn, 180)
        check(state in ("completed", "loaded"), "Dynamics ran to %s" % state)
        d = where(dyn)
        check(waitProps(d, ("MDMSD", "MDSPEED")), "the run stored MDMSD and MDSPEED")
        say("run and store took %.0f s" % (time.time() - t0))
        msd = records(d, "MDMSD") or []
        check(len(msd) == 1 and msd[0].startswith("AR;1000;"),
              "one MSD curve for 1000 AR: %s" % [m[:20] for m in msd])
        sp = records(d, "MDSPEED") or []
        ok = len(sp) == 1 and sp[0].startswith("Ar;") and len(sp[0].split(";")) == 8
        check(ok, "one argon speed curve")
        if ok:
            mean = float(sp[0].split(";")[4])
            mb = math.sqrt(8 * 1.380649e-23 * REF_T / (math.pi * 39.948 * 1.66053907e-27))
            check(abs(mean / mb - 1) < 0.1,
                  "mean argon speed %.0f m/s, Maxwell-Boltzmann %.0f m/s at %g K"
                  % (mean, mb, REF_T))

        runs = [os.path.dirname(p) for p in glob.glob(
            os.path.join(rundir, "**", "traj.trr"), recursive=True)]
        if not check(len(runs) == 1, "the run directory: %s" % runs):
            return 1
        src = runs[0]

        # The job script's function again, as a run without velocities and
        # on a host without the program would call it.
        script = open(glob.glob(os.path.join(src, "submit__*"))[0]).read()
        a = script.index("  ecceMdAnalysis() {")
        func = script[a:script.index("\n  }\n", a) + 5]
        for case, mda in (("novel", os.path.join(s.home, "bin", "mdanalysis")),
                          ("noprog", "/nonexistent/mdanalysis")):
            w = os.path.join(s.state, case)
            shutil.rmtree(w, ignore_errors=True)
            os.makedirs(w)
            for f in ("traj.xtc", "ecce_pp.top", "topol.top"):
                shutil.copy(os.path.join(src, f), w)
            with open(os.path.join(w, "gromacs.mdp"), "w") as h:
                h.write(MDP)
            sh = func.replace(os.path.join(src, "ecce.submit.log"), "job.log")
            # A PATH with only the tools the step uses: an installed
            # ecce-mdanalysis must not stand in for the missing program.
            tools = os.path.join(w, "tools")
            os.makedirs(tools)
            for tool in ("sh", "grep", "sed", "tail", "cat", "rm"):
                os.symlink(shutil.which(tool), os.path.join(tools, tool))
            subprocess.run([os.path.join(tools, "sh"), "-c",
                            "mda='%s'\n%s\necceMdAnalysis" % (mda, sh)],
                           cwd=w, env=dict(s.env(), PATH=tools), timeout=60)
            got = open(os.path.join(w, "ecce-parse-md.txt")).read().splitlines() \
                if os.path.exists(os.path.join(w, "ecce-parse-md.txt")) else []
            log = open(os.path.join(w, "job.log")).read() \
                if os.path.exists(os.path.join(w, "job.log")) else ""
            if case == "novel":
                check(len(got) == 2 and got[0].startswith("ECCE-MDMSD AR;1000;")
                      and got[1] == "ECCE-MDSPEED none",
                      "job step without nstvout: MSD and 'ECCE-MDSPEED none': %s"
                      % [g[:24] for g in got])
            else:
                check(not got and "not installed on this host" in log,
                      "job step without the program: no file, a log line: %s" % log.strip())

        # Import the run as one made outside ECCE, and a copy without velocities.
        imp = os.path.join(s.state, "import")
        shutil.rmtree(imp, ignore_errors=True)
        for run, vel in (("withvel", True), ("novel", False)):
            t = os.path.join(imp, run, "1-dynamics")
            os.makedirs(t)
            for f in ("traj.xtc", "topol.top", "conf.gro") + (("traj.trr",) if vel else ()):
                shutil.copy(os.path.join(src, f), t)
            with open(os.path.join(t, "prod.mdp"), "w") as h:
                h.write(MDP + ("nstvout = 10\n" if vel else ""))
            for f in os.listdir(src):
                if f.endswith(".log") and f != "ecce.submit.log":
                    shutil.copy(os.path.join(src, f), os.path.join(t, "prod.log"))
                elif f.endswith("_ecceMdDynamics.gro"):
                    shutil.copy(os.path.join(src, f), os.path.join(t, "confout.gro"))

        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        proj = study.rstrip("/").rsplit("/", 1)[0] + "/"
        pdir = where(proj)
        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")],
            env=dict(env, ECCE_TEST_ORGANIZER=cmds, ECCE_GROMACS=gmx),
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)
        imported = {}
        for run in ("withvel", "novel"):
            before = set(os.listdir(pdir))
            line = "gmximport %s %s" % (proj, os.path.join(imp, run))
            with open(cmds, "a") as h:
                h.write(line + "\n")
            ans, end = "no answer", time.time() + 120
            while time.time() < end and ans == "no answer" and org.poll() is None:
                for t in open(olog, errors="replace").read().splitlines():
                    if t.startswith("ECCE_TEST_ORGANIZER: %s: " % line):
                        ans = t.split(": ", 2)[-1]
                time.sleep(0.5)
            check(ans.startswith("ok"), "gmximport %s: %s" % (run, ans))
            made = [x for x in os.listdir(pdir) if x not in before]
            imported[run] = os.path.join(pdir, made[0], "dynamics") if made else ""
        w, n = imported["withvel"], imported["novel"]
        check(w and waitProps(w, ("MDMSD", "MDSPEED"))
              and records(w, "MDSPEED") == records(d, "MDSPEED")
              and records(w, "MDMSD") == records(d, "MDMSD"),
              "the imported run has the live run's MDMSD and MDSPEED")
        check(n and waitProps(n, ("MDMSD", "MDSPEED")) and records(n, "MDSPEED") == ["none"]
              and records(n, "MDMSD") == records(d, "MDMSD"),
              "the import without velocities has MDMSD and MDSPEED 'none'")
        # The monitor the Organizer started stores them; only now may it go.
        stop(org)
        org = None

        # The panels, through the Builder's script commands.
        png = os.path.join(s.state, "png")
        os.makedirs(png, exist_ok=True)
        for u, panel, cmd, want in (
                (dyn, "Speed Distribution", "speed", "curves=2 ok"),
                (dyn, "Displacement (MSD)", "msd", "curves=2 ok"),
                ("file://" + n + "/", "Speed Distribution", "speed", "curves=0 ok")):
            csv = os.path.join(s.state, "panel-%s.csv" % cmd)
            text, _ = bo.runScript(
                s, display, env, wrappers, ["-context", u],
                ["wait 3000", "mdcsv %s %s" % (cmd, csv), "quit"], png, "md" + cmd,
                {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": panel})
            line = [l for l in text.splitlines() if "mdcsv curves=" in l]
            check(bool(line) and line[0].endswith(want),
                  "%s: the %s panel reports %s" % (u.rstrip("/").split("/", 3)[-1][-40:],
                                                    panel, line[:1]))
    finally:
        stop(org)
        if display is not None:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: %s" % failures if failures else "OK")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
