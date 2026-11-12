#!/usr/bin/env python3
"""Umbrella sampling on 2BEG (Abeta 17-42 fibril), the whole lab chain through ECCE (#762).

The worked example of help/src/umbrella-sampling.md, replayed headless and
shortened: Prepare (gromos53a6 + SPC, chains not merged, an off-centre box
from Box size + Centre, neutralised + 0.1 M NaCl), Optimize, NPT at 310 K with
every chain restrained, a fast pull of chain A away from chain B (B
restrained) along z, windows made by the Organizer's umbrellawindows hook,
each window's NPT and production launched through ECCE, then WHAM on the Pull
Force panel.  It checks that a PMF is drawn, that neighbouring window
histograms overlap, and that the PMF has a minimum.  The pull is 25 times
faster and every run far shorter than the lab's, so the PMF is not a result;
the Na+ Cl- test checks the science.

    tests/launch/umbrella_2beg_test.py [--build DIR] [--png DIR] [--keep]
        [--host USER@HOST --rundir DIR]   (jobs through Slurm on that machine)

Lengths (steps of 2 fs) come from the environment: OPT_STEPS (1000),
NPT_STEPS (10000), PULL_STEPS (25000), PULL_RATE (0.05 nm/ps), WIN_SPACING
(0.2), WIN_FROM (0.5), WIN_TO (1.5), WIN_EQ (5000), WIN_PROD (20000), BOX and
CENTRE (nm), PROCS (4) per job.  Wall times per stage go to times.json in the
state folder.  Exit status 77 (CTest SKIP) without gmx, Xvfb or the binaries.
"""

import argparse
import glob
import json
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
from gromacs_test import Editor, check, failures, stop  # noqa: E402
import builder_overlay_test as bo  # noqa: E402
from umbrella_panel_test import Organizer  # noqa: E402

UMB = os.path.join(REPO, "scripts", "ecce_umbrella.py")
ENV = os.environ.get
STEPS = {"optimize": ENV("OPT_STEPS", "1000"), "equilibrate": ENV("NPT_STEPS", "10000"),
         "dynamics": ENV("PULL_STEPS", "25000")}
PROCS = ENV("PROCS", "4")
RATE = ENV("PULL_RATE", "0.05")
FIXTURE = os.path.join(REPO, "tests", "gromacs_md", "fixtures", "2beg_model1.pdb")
CHAINS = "ABCDE"
# the lab's box is 6.560 x 4.362 x 12 nm with the fibril at a fifth of z; the
# short pull needs less room
SIZE = ENV("BOX", "6.560 4.362 8")
CENTER = ENV("CENTRE", "3.280 2.181 1.65")


def where(u):
    return u[len("file://"):].rstrip("/")


def verify(csv, nwin):
    """The panel's CSV: a PMF, histograms that overlap, a minimum named."""
    if not check(os.path.exists(csv), "the panel exported the PMF as CSV"):
        return
    rows = [l.split(",") for l in open(csv).read().splitlines() if l and l[0] != "#"]
    head, rows = rows[0], rows[1:]
    hcols = [i for i, h in enumerate(head) if h.startswith("hist_")]
    sup = [r for r in rows if len(r) > 2 and r[2] == "1"]
    pmf = [float(r[1]) for r in sup]
    check(len(sup) > 20 and all(abs(v) < 1e4 for v in pmf),
          "a PMF of %d supported points (%.1f to %.1f kJ/mol)" % (len(sup), min(pmf or [0]), max(pmf or [0])))
    check(len(hcols) == nwin, "one histogram per window (%d of %d)" % (len(hcols), nwin))
    hist = [[float(r[i]) if i < len(r) and r[i] else 0.0 for r in rows] for i in hcols]
    low = []
    for k, (a, b) in enumerate(zip(hist, hist[1:])):
        ta, tb = sum(a) or 1.0, sum(b) or 1.0
        shared = sum(min(x / ta, y / tb) for x, y in zip(a, b))
        say("  windows %d and %d share %.0f %%" % (k + 1, k + 2, 100 * shared))
        if shared < 0.05:
            low.append(k + 1)
    check(not low, "every pair of neighbouring histograms overlaps by at least 5 %% (not after window %s)" % low)
    mins = [r for r in rows if len(r) > 4 and r[3] == "minimum"]
    check(mins, "the summary names a minimum (%s)" % ", ".join(
        "%.2f nm %.1f kJ/mol" % (float(r[0]), float(r[1])) for r in mins[:3]))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--host", help="USER@HOST of a Slurm machine to run the jobs on (default: this machine)")
    ap.add_argument("--rundir", help="run folder root on that machine")
    ap.add_argument("--gmx-remote", default="/usr/bin/gmx")
    ap.add_argument("--partition", default="normal")
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    remote = args.host is not None
    if remote and not args.rundir:
        ap.error("--host needs --rundir")
    ruser, rhost = args.host.split("@") if remote else (None, None)
    gmx = shutil.which("gmx")
    if not gmx:
        harness.skip("gmx is not installed")
    if not os.path.exists(FIXTURE):
        harness.skip("no 2BEG fixture")
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    exes = ("mdoptimize", "mddynamics", "gmxprepare", "builder", "organizer")
    for exe in exes:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "umb2beg", {"GROMACS": gmx}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    times = {}

    def note(k, v):
        times[k] = v
        json.dump(times, open(os.path.join(s.state, "times.json"), "w"), indent=1)
        say("TIME %s %s" % (k, v))

    for exe in exes:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    harness.link(UMB, os.path.join(s.home, "bin", "ecce-umbrella"))
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
    if remote:
        s.registerMachine("nerdegg", "Slurm", {"GROMACS": args.gmx_remote},
                          queues=(args.partition,), limits={args.partition: (1, 12, 2880)},
                          host=rhost)
    pdb = FIXTURE

    def setup(url, machine="nerdegg"):
        name = os.path.basename(where(url))
        if machine == "localhost" or not remote:
            return s.driver("mdsetup", url, "localhost", os.path.join(s.state, "run"), s.user(),
                            "procs=%s" % PROCS)[0] == 0
        return s.driver("mdsetup", url, "nerdegg", "%s/%s" % (args.rundir, name), ruser,
                        "queue=%s" % args.partition, "procs=%s" % PROCS, 'wall=0 12:00')[0] == 0

    def run_task(url, label, limit):
        t0 = time.time()
        rc, out = s.launch(url)
        if not check(rc == 0, "Launch accepted %s: %s" % (label, out.strip()[-160:])):
            return False
        state = s.waitState(url, limit)
        note(label, "%.0f s (%s)" % (time.time() - t0, state))
        if state not in ("completed", "loaded"):
            dump_failure(url, out)
        return check(state in ("completed", "loaded"), "%s ran to %s" % (label, state))

    def dump_failure(url, out):
        # The run folder goes with the session, so the GROMACS error has to
        # reach the log now.
        say("  reason: %s" % s.reason_of(url))
        m = re.search(r"^run directory: (.*)$", out, re.M)
        rdir = m.group(1).strip() if m else ""
        if not rdir or not os.path.isdir(rdir):
            say("  (no local run directory %r)" % rdir)
            return
        say("  run directory files: %s" % sorted(os.listdir(rdir)))
        for f in sorted(os.listdir(rdir)):
            if f.endswith((".mdp", ".log", ".err", ".out", ".sh")) or "std" in f or "grompp" in f:
                try:
                    lines = open(os.path.join(rdir, f), errors="replace").read().splitlines()
                except OSError:
                    continue
                say("  ---- %s (last 60 of %d lines)" % (f, len(lines)))
                for l in lines[-60:]:
                    say("  | " + l)

    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        display = xdisplay.Display(screen="1500x1000x24").__enter__()
        cmddir = os.path.join(s.state, "mded")
        os.makedirs(cmddir)
        genv = s.env({"DISPLAY": display.name, "ECCE_TEST_MDED": cmddir, "GTK_THEME": "Adwaita",
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        rc, out = s.driver("gromacsstudy", s.userUrl(), "scratch")
        urls = [l for l in out.splitlines() if l.startswith("file://")]
        if not check(rc == 0 and urls, "a project to work in"):
            return 1
        proj = urls[0].rsplit("/", 2)[0] + "/"
        pdir = where(proj)
        before = set(os.listdir(pdir))
        org = Organizer(s, display, wrappers, genv)
        time.sleep(10)

        def subdirs(d):
            return sorted(x for x in os.listdir(d) if not x.startswith(".")
                          and os.path.isdir(os.path.join(d, x)))

        check(org.ask("new %s gromacs_md_study" % proj) == "ok", "New: GROMACS MD Study")
        study = [x for x in subdirs(pdir) if x not in before and not subdirs(os.path.join(pdir, x))]
        study_url = proj.rstrip("/") + "/" + study[0]
        tasks = {}
        for kind in ("prepare", "optimize", "equilibrate", "dynamics"):
            have = set(subdirs(where(study_url)))
            ans = org.ask("new %s gromacs_md_%s" % (study_url, kind))
            new = sorted(set(subdirs(where(study_url))) - have)
            if not check(ans == "ok" and len(new) == 1, "New: %s" % kind):
                return 1
            tasks[kind] = study_url + "/" + new[0] + "/"
            check(setup(tasks[kind], "localhost" if kind == "prepare" else "nerdegg"),
                  "launch settings for %s" % kind)

        # ---- Prepare (light: this machine) ------------------------------
        ed = Editor(s, display, genv, wrappers, cmddir, "gmxprepare", "GMXPrepare")
        try:
            if not ed.up():
                return 1
            check(ed.cmd("open " + tasks["prepare"]) == "ok", "the Prepare opens")
            for k, v in (("source", "protein"), ("structure", pdb), ("ff", "gromos53a6"),
                         ("water", "spc"), ("ignh", "1"), ("merge", "0"), ("size", SIZE),
                         ("center", CENTER), ("neut", "1"), ("conc", "0.1")):
                check(ed.cmd("set %s %s" % (k, v)) == "ok", "Prepare: %s %s" % (k, v))
            say("  fields: %s" % ed.cmd("fields"))
            ed.cmd("save")
            ed.cmd("snap " + os.path.join(png, "01-prepare.png"))
        finally:
            ed.close()
        if not run_task(tasks["prepare"], "prepare", 1800):
            return 1
        out = os.path.join(where(tasks["prepare"]), "Outputs")
        gro = glob.glob(os.path.join(out, "*.gro"))
        if gro:
            lines = open(gro[0]).read().splitlines()
            say("  prepared: %s atoms, box %s" % (lines[1].strip(), lines[-1].strip()))

        # ---- Optimize, NPT, pull (NerdEgg) ------------------------------
        for kind, binary, app in (("optimize", "mdoptimize", "MDOptimize"),
                                  ("equilibrate", "mddynamics", "MDDynamics"),
                                  ("dynamics", "mddynamics", "MDDynamics")):
            ed = Editor(s, display, genv, wrappers, cmddir, binary, app)
            try:
                if not ed.up():
                    return 1
                check(ed.cmd("open " + tasks[kind]) == "ok", "the %s opens" % kind)
                check(ed.cmd("set-steps " + STEPS[kind]) == "ok", "%s steps %s" % (kind, STEPS[kind]))
                extra = []
                if kind == "optimize":
                    extra = ["emtol = 1000"]
                else:
                    check(ed.cmd("ensemble-set npt") == "npt", "%s: NPT" % kind)
                    check(ed.cmd("temperature-set 310") == "ok", "%s: 310 K" % kind)
                    extra = ["nstenergy = 500"]
                    if kind == "equilibrate":
                        for c in CHAINS:
                            check(ed.cmd("run-set chain:%s 1" % c) == "ok", "NPT: restrain chain %s" % c)
                    else:
                        check(ed.cmd("run-set chain:B 1") == "ok", "pull: chain B restrained")
                        for k, v in (("groups", "Chain_A = mol:Protein_chain_A\nChain_B = mol:Protein_chain_B"),
                                     ("pull", "1"), ("pullgroup1", "Chain_B"), ("pullgroup2", "Chain_A"),
                                     ("pullgeom", "0"), ("pullrate", RATE), ("pullk", "1000")):
                            check(ed.cmd("run-set %s %s" % (k, v.replace("\n", "\\n"))) == "ok",
                                  "pull: %s" % k)
                        # z only, as the lab; a frame every 0.05 nm of pull
                        every = max(1, int(float(ENV("PULL_FRAME_NM", "0.05")) / (float(RATE) * 0.002)))
                        extra += ["pull-coord1-dim = N N Y", "nstxout-compressed = %d" % every]
                check(ed.cmd("run-set extra " + "\\n".join(extra)) == "ok", "%s extra lines" % kind)
                say("  run: %s" % ed.cmd("run"))
                check(ed.cmd("save") == "ok", "Save")
                check(ed.cmd("launch") == "enabled", "Launch is enabled")
                if kind == "dynamics":
                    for tab in ed.cmd("tabs").split("|"):
                        if ed.cmd("tab " + tab) == "ok":
                            ed.cmd("snap " + os.path.join(png, "02-pull-%s.png" % tab.replace(" ", "_").lower()))
                mdp = open(os.path.join(where(tasks[kind]), "Inputs", "gromacs.mdp")).read()
                if kind == "equilibrate":
                    m = re.search(r"^gen[-_]temp\s*=\s*(\S+)", mdp, re.M)
                    check(m and float(m.group(1)) == 310, "NPT draws velocities at 310 K (gen-temp %s)"
                          % (m.group(1) if m else None))
                say("  mdp: %s" % mdp.replace("\n", " | ")[:3000])
            finally:
                ed.close()
            if not run_task(tasks[kind], kind, 3 * 3600):
                return 1
            say("  properties: %s" % s.props(tasks[kind]))

        # ---- windows ----------------------------------------------------
        pull = tasks["dynamics"]
        eq, prod = ENV("WIN_EQ", "5000"), ENV("WIN_PROD", "20000")
        spacing, lo, hi = ENV("WIN_SPACING", "0.2"), ENV("WIN_FROM", "0.5"), ENV("WIN_TO", "1.5")
        t0 = time.time()
        ans = org.ask("umbrellawindows %s %s %s %s %s %s" % (pull, spacing, lo, hi, eq, prod), 1800)
        say("umbrellawindows: %s" % ans[:300])
        note("windows made", "%.0f s" % (time.time() - t0))
        sdir = os.path.dirname(where(pull))
        base = os.path.basename(where(pull))
        wins = sorted(d for d in subdirs(sdir) if re.match(re.escape(base) + r"-w\d\d$", d))
        if not check(len(wins) >= 4, "%d windows (%s eq + %s prod steps)" % (len(wins), eq, prod)):
            return 1
        for w in wins:
            for t in (w + "-eq", w):
                setup("file://%s/%s/" % (sdir, t))
        # each window's equilibration, then its production, one window at a time
        # (one machine; Slurm would take them side by side)
        t0 = time.time()
        for w in wins:
            if not (run_task("file://%s/%s-eq/" % (sdir, w), w + "-eq", 3600)
                    and run_task("file://%s/%s/" % (sdir, w), w, 3600)):
                return 1
        note("windows ran", "%.0f s for %d windows" % (time.time() - t0, len(wins)))

        # ---- panel: WHAM, PMF, pull page, RMSD / Rg ----------------------
        csv = os.path.join(s.state, "pmf.csv")
        for name, lines in (
                ("pull", ["wait 8000", "shot pullpage", "umbrellaanalysis rms", "wait 4000",
                          "shot rmsd", "umbrellaanalysis gyrate", "wait 4000", "shot gyrate", "quit"]),
                ("pmf", ["wait 8000", "umbrellapage", "umbrellafind %s %s %s" % (spacing, lo, hi), "umbrellawham",
                         "umbrellaexport %s" % csv, "umbrellastate", "wait 2000", "shot umbpage",
                         "umbrellashot %s 900 700" % os.path.join(png, "pmf-shot.png"), "quit"])):
            text, shots = bo.runScript(
                s, display, genv, wrappers, ["-context", pull], lines, png, name,
                {"ECCE_OPEN_PANEL": "Pull Force", "ECCE_OPEN_PANEL_SIZE": "1100x900"})
            for l in text.splitlines():
                if l.startswith("BUILDER: umbrella"):
                    say(l)
            for k, v in shots.items():
                shutil.copy(v, os.path.join(png, "%s-%s.png" % (name, k)))
        verify(csv, len(wins))
        if os.path.exists(csv):
            shutil.copy(csv, png)
    finally:
        if org is not None:
            org.stop()
        if display is not None:
            display.__exit__(None, None, None)
        leftovers = s.stop()
        if leftovers:
            say("left running: %s" % leftovers)
    say("%d failure(s); pictures in %s; times %s" % (len(failures), png, json.dumps(times)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
