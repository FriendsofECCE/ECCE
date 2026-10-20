#!/usr/bin/env python3
"""The help page gromacs-first-calculation, replayed headless.

A protein (1L2Y, the Trp-cage) goes through New GROMACS MD study, Prepare,
Optimize, Equilibrate and Dynamics, each task made through the Organizer's
New hook and edited in its real editor (ECCE_TEST_MDED), then launched and
run by gmx.  The settings are the ones the page tells a student to use; if
this test changes, the page changes.

    tests/launch/gromacs_protein_walk_test.py [--build DIR] [--png DIR] [--keep]

Exit status 77 (CTest SKIP) without gmx, Xvfb or the built binaries.
"""

import argparse
import glob
import os
import re
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
from gromacs_test import Editor, check, failures, gmx_check, gmx_series, \
    last_run_file, prop_value, shot_when_window, stop  # noqa: E402

FIXTURES = os.path.join(REPO, "tests", "gromacs_md", "fixtures")
STEPS = {"optimize": "200", "equilibrate": "1000", "dynamics": "2000"}


def where(u):
    return u[len("file://"):].rstrip("/")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--save-example", help="write the small files of the run here")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    gmx = shutil.which("gmx")
    if not gmx:
        if os.environ.get("ECCE_GROMACS_REQUIRE"):
            say("FAIL: gmx is not installed and ECCE_GROMACS_REQUIRE is set")
            return 1
        harness.skip("gmx is not installed")
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    exes = ("mdoptimize", "mddynamics", "gmxprepare", "builder", "organizer")
    for exe in exes:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "gmxwalk", {"GROMACS": gmx}, local=True,
                        keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in exes:
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
    s.registerMachine("localhost", "Shell", {"GROMACS": gmx})
    rundir = os.path.join(s.state, "run")

    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        display = xdisplay.Display().__enter__()
        cmddir = os.path.join(s.state, "mded")
        os.makedirs(cmddir)
        genv = s.env({"DISPLAY": display.name, "ECCE_TEST_MDED": cmddir,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})

        # A project to work in; the study inside it is made by New.
        rc, out = s.driver("gromacsstudy", s.userUrl(), "scratch")
        urls = [l for l in out.splitlines() if l.startswith("file://")]
        if not check(rc == 0 and urls, "a project to work in"):
            return 1
        proj = urls[0].rsplit("/", 2)[0] + "/"
        pdir = where(proj)
        before = set(os.listdir(pdir))

        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")],
            env=dict(genv, ECCE_TEST_ORGANIZER=cmds),
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        def send(line, timeout=120):
            with open(cmds, "a") as h:
                h.write(line + "\n")
            want = "ECCE_TEST_ORGANIZER: %s: " % line
            end = time.time() + timeout
            while time.time() < end:
                if org.poll() is not None:
                    return "the Organizer ended (%s)" % org.returncode
                for t in open(olog, errors="replace").read().splitlines():
                    if t.startswith(want):
                        return t[len(want):]
                time.sleep(0.5)
            return "no answer"

        def subdirs(d):
            return sorted(x for x in os.listdir(d) if not x.startswith(".")
                          and os.path.isdir(os.path.join(d, x)))

        # ---- 1. New > GROMACS MD Study ---------------------------------
        check(send("new %s gromacs_md_study" % proj) == "ok", "New: GROMACS MD Study")
        study = [x for x in subdirs(pdir) if x not in before
                 and not subdirs(os.path.join(pdir, x))]
        if not check(len(study) == 1, "the study was made: %s" % study):
            return 1
        study_url = proj.rstrip("/") + "/" + study[0]
        tasks = {}
        for kind in ("prepare", "optimize", "equilibrate", "dynamics"):
            have = set(subdirs(where(study_url)))
            ans = send("new %s gromacs_md_%s" % (study_url, kind))
            new = sorted(set(subdirs(where(study_url))) - have)
            check(ans == "ok" and len(new) == 1, "New: GROMACS MD %s -> %s (%s)" % (kind, new, ans))
            if len(new) != 1:
                return 1
            tasks[kind] = study_url + "/" + new[0] + "/"
            rc, out = s.driver("mdsetup", tasks[kind], "localhost", rundir, s.user())
            check(rc == 0, "launch settings (localhost) for %s" % kind)
        say("study %s holds %s" % (study[0], subdirs(where(study_url))))
        shot_when_window(display, genv, "Organizer", os.path.join(png, "01-organizer.png"), 60)

        # ---- 2. Prepare -------------------------------------------------
        say("Prepare")
        ed = Editor(s, display, genv, wrappers, cmddir, "gmxprepare", "GMXPrepare")
        try:
            if not ed.up():
                return 1
            check(ed.cmd("open " + tasks["prepare"]) == "ok", "the Prepare opens")
            say("  fields at first: %s" % ed.cmd("fields"))
            check(ed.cmd("set source protein") == "ok", "Source: Protein")
            check(ed.cmd("set structure " + os.path.join(FIXTURES, "1l2y.pdb")) == "ok",
                  "Structure: PDB file... 1l2y.pdb")
            say("  fields: %s" % ed.cmd("fields"))
            say("  verify: %s" % ed.cmd("verify"))
            check(ed.cmd("launch") == "enabled", "Launch is enabled")
            ed.cmd("save")
            ed.cmd("snap " + os.path.join(png, "02-prepare.png"))
            say("  recipe: %s" % ed.cmd("recipe"))
        finally:
            ed.close()
        rc, out = s.launch(tasks["prepare"])
        check(rc == 0, "Launch accepted the Prepare: %s" % out.strip()[-120:])
        state = s.waitState(tasks["prepare"], 600)
        check(state in ("completed", "loaded"), "Prepare ran to %s" % state)
        ptask = where(tasks["prepare"])
        say("  Prepare Outputs: %s" % sorted(os.listdir(os.path.join(ptask, "Outputs"))))
        pgro = glob.glob(os.path.join(ptask, "Outputs", "*.gro"))
        check(pgro and gmx_check(gmx, pgro[0]), "the prepared structure is a valid .gro")
        top = [o for o in os.listdir(os.path.join(ptask, "Outputs")) if o.endswith(".gmxtop")]
        mols = []
        if top:
            mol = open(os.path.join(ptask, "Outputs", top[0])).read()
            mols = [l.split()[0] for l in mol.split("[ molecules ]")[-1].splitlines()
                    if l.strip() and not l.lstrip().startswith(";")]
        say("  molecules: %s" % mols)
        check("SOL" in mols and "CL" in mols, "the protein is in water with Cl- ions")

        # ---- 3. Optimize, 4. Equilibrate, 5. Dynamics -------------------
        for kind, binary, app, label in (
                ("optimize", "mdoptimize", "MDOptimize", "Optimize"),
                ("equilibrate", "mddynamics", "MDDynamics", "Equilibrate"),
                ("dynamics", "mddynamics", "MDDynamics", "Dynamics")):
            say(label)
            ed = Editor(s, display, genv, wrappers, cmddir, binary, app)
            try:
                if not ed.up():
                    return 1
                check(ed.cmd("open " + tasks[kind]) == "ok", "the %s opens" % kind)
                say("  title: %s" % ed.cmd("title"))
                tabs = ed.cmd("tabs")
                say("  pages: %s" % tabs)
                out = ed.cmd("inputs")
                if "Ready" in out:
                    check(ed.cmd("inputs-warning") == "none",
                          "no missing-inputs warning beside a Ready Inputs page")
                say("  inputs: %s" % out)
                check("Ready" in out and "(from" in out,
                      "topology and structure found on an earlier task")
                check(ed.cmd("set-steps " + STEPS[kind]) == "ok", "%s steps" % STEPS[kind])
                check(ed.cmd("save") == "ok", "Save")
                check(ed.cmd("launch") == "enabled", "Launch is enabled")
                for tab in tabs.split("|"):
                    if ed.cmd("tab " + tab) == "ok":
                        ed.cmd("snap " + os.path.join(
                            png, "03-%s-%s.png" % (kind, tab.replace(" ", "_").lower())))
                mdp = os.path.join(where(tasks[kind]), "Inputs", "gromacs.mdp")
                text = open(mdp).read()
                say("  mdp: %s" % text.replace("\n", " | "))
                if kind == "equilibrate":
                    check(ed.cmd("resume") == "off", "Equilibrate starts with Continue unticked")
                    check(re.search(r"^gen[-_]vel\s*=\s*yes", text, re.M) is not None,
                          "Equilibrate draws new velocities (gen-vel yes)")
                if kind == "dynamics":
                    check(ed.cmd("resume") == "on",
                          "Dynamics after Equilibrate opens with Continue ticked")
                    check(re.search(r"^gen[-_]vel\s*=\s*no", text, re.M) is not None
                          and re.search(r"^continuation\s*=\s*yes", text, re.M) is not None,
                          "Dynamics .mdp continues (continuation yes, gen-vel no)")
                    prev = glob.glob(os.path.join(where(tasks["equilibrate"]), "Outputs", "*.gro"))
                    first = open(prev[0]).read().splitlines()[2] if prev else ""
                    check(len(first) > 44, "the Equilibrate structure it continues from holds velocities")
            finally:
                ed.close()
            rc, out = s.launch(tasks[kind])
            check(rc == 0, "Launch accepted the %s: %s" % (kind, out.strip()[-120:]))
            t0 = time.time()
            state = s.waitState(tasks[kind], 900)
            check(state in ("completed", "loaded"),
                  "%s ran to %s in %d s" % (kind, state, time.time() - t0))
            say("  properties: %s" % s.props(tasks[kind]))
            say("  Outputs: %s" % sorted(os.listdir(os.path.join(where(tasks[kind]), "Outputs"))))

        # ---- results ----------------------------------------------------
        dyn = tasks["dynamics"]
        gro = os.path.join(where(dyn), "Outputs", "ecceSession_ecceMdDynamics.gro")
        check(gmx_check(gmx, gro), "the final structure is a valid .gro (gmx check)")
        drun = last_run_file(rundir, "ecceSession_ecceMdDynamics.gro")
        te_gmx = gmx_series(gmx, drun and os.path.dirname(drun), "Total-Energy")
        te = prop_value(s, dyn, "TE")
        check(te_gmx and te is not None and abs(te - te_gmx[-1]) < 0.5,
              "TE is what gmx energy reads (%s vs %s)" % (te, te_gmx[-1] if te_gmx else None))
        temps = gmx_series(gmx, drun and os.path.dirname(drun), "Temperature")
        say("  last temperature %s K" % (temps[-1] if temps else None))
        check(temps and 150 < temps[-1] < 450, "near the target temperature")
        # The run's frames are stored as a geometry trace, one step per
        # trajectory frame (steps/interval + 1: mdrun writes step 0 too).
        mdp = open(os.path.join(os.path.dirname(drun), "gromacs.mdp")).read() \
            if drun and os.path.exists(os.path.join(os.path.dirname(drun), "gromacs.mdp")) else ""
        m = re.search(r"nstxout-compressed\s*=\s*(\d+)", mdp)
        want = int(STEPS["dynamics"]) // int(m.group(1)) + 1 if m else None
        gt = glob.glob(os.path.join(where(dyn), "Props", "GEOMTRACE*"))
        text = open(gt[0], errors="replace").read() if gt else ""
        got = len(re.findall(r"<step number=", text))
        natoms = int(re.search(r'rows="(\d+)"', text).group(1)) if gt and re.search(r'rows="(\d+)"', text) else 0
        say("  trajectory: %d frames of %d atoms (want %s), %d bytes"
            % (got, natoms, want, len(text)))
        check(want and got == want, "Dynamics stores %s trajectory frames" % want)
        gro_atoms = int(open(gro).read().split("\n")[1])
        check(natoms == gro_atoms, "frames hold every atom of the system (%d)" % gro_atoms)
        kept = [f for f in os.listdir(os.path.join(where(dyn), "Outputs"))
                if f.startswith("ecce-parse-")]
        check(not kept, "the trajectory is stored once, as the trace; %s kept "
              "beside it: %s (trace %d bytes)" % (len(kept), kept, len(text)))
        for name, url, panel in (("structure", tasks["optimize"], ""),
                                 ("energies", dyn, "Geometry Step Plots"),
                                 ("trajectory", dyn, "Geometry Trace")):
            env = dict(genv, ECCE_PANEL_FULLSCREEN="1")
            if panel:
                env["ECCE_OPEN_PANEL"] = panel
            b = subprocess.Popen(
                [os.path.join(wrappers, "ecce-builder"), "-context", url], env=env,
                cwd=os.path.join(s.home, "bin"),
                stdout=open(os.path.join(s.state, name + ".log"), "w"),
                stderr=subprocess.STDOUT, start_new_session=True)
            shot_when_window(display, genv, "Builder",
                             os.path.join(png, "04-results-%s.png" % name), 120, settle=12)
            stop(b)
        if args.save_example:
            save_example(args.save_example, gmx, tasks, rundir)
    finally:
        if org is not None:
            stop(org)
        if display is not None:
            display.__exit__(None, None, None)
        leftovers = s.stop()
        if leftovers:
            say("left running: %s" % leftovers)
    say("%d failure(s); screenshots in %s" % (len(failures), png))
    return 1 if failures else 0


def save_example(dest, gmx, tasks, rundir):
    """The run as GromacsImport.H lays it out: one directory per task."""
    shutil.rmtree(dest, ignore_errors=True)
    os.makedirs(dest)
    shutil.copy(os.path.join(FIXTURES, "1l2y.pdb"), dest)

    def put(task, src, name=None):
        d = os.path.join(dest, task)
        os.makedirs(d, exist_ok=True)
        shutil.copy(src, os.path.join(d, name or os.path.basename(src)))

    pout = os.path.join(where(tasks["prepare"]), "Outputs")
    put("1-prepare", os.path.join(where(tasks["prepare"]), "Inputs", "gromacs.gmxprep"),
        "prepare.recipe")
    put("1-prepare", os.path.join(FIXTURES, "1l2y.pdb"))
    for f in sorted(os.listdir(pout)):
        if f.endswith((".gro", ".gmxtop", ".itp", ".gmxlog", ".log")):
            put("1-prepare", os.path.join(pout, f))
    say("  example Prepare outputs: %s" % sorted(os.listdir(pout)))
    for n, kind in enumerate(("optimize", "equilibrate", "dynamics"), 2):
        task = "%d-%s" % (n, kind)
        put(task, os.path.join(where(tasks[kind]), "Inputs", "gromacs.mdp"), kind + ".mdp")
        run = last_run_file(rundir, "ecceSession_ecceMd%s.gro" % kind.capitalize())
        rdir = os.path.dirname(run) if run else None
        say("  example %s run dir: %s" % (kind, rdir and sorted(os.listdir(rdir))))
        if not rdir:
            continue
        put(task, run, "confout.gro")
        for f in sorted(os.listdir(rdir)):
            if f.endswith(".log") or f == "ecce-parse-traj.txt":
                put(task, os.path.join(rdir, f))


if __name__ == "__main__":
    sys.exit(main())
