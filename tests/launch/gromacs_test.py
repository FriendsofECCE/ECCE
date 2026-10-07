#!/usr/bin/env python3
"""A GROMACS MD study, from the classes behind the New menu to its results.

Local data, no data server.  On a private X server, with the tree's own
binaries:

  * a project and a GROMACS MD study with an Optimize, an Equilibrate and a
    Dynamics task are made through Resource::createChild and
    Session::addMemberAsTarget, as the Organizer's New menu does
    (launchjob gromacsstudy);
  * each task is opened in the real MD editor, which is driven through its
    test hook (ECCE_TEST_MDED, MDEdBase::runTestCommand): the notebook, the
    Inputs page, attaching a topology, a structure and an include file
    exactly as the Attach buttons do, Save (which runs md.gmxtask), the
    Launch button's state;
  * a launch before anything is attached is refused, in words;
  * the three tasks run through the real Launch, gensub, eccejobmaster,
    eccejobstore and eccejobmonitor, each starting from the structure the
    one before ended with and finding the topology on the first task;
  * the energies and the final structure are what gmx itself says they are
    (gmx energy on the .edr, gmx check on the .gro), and the results are
    opened in the Builder.

Screenshots of every GROMACS editor tab, of the Organizer and of the
Builder with the energy plot go to --png.

    tests/launch/gromacs_test.py [--build build-cmake] [--png DIR] [--keep]

Exit status 77 (CTest SKIP) without gmx, Xvfb or the built binaries;
ECCE_GROMACS_REQUIRE=1 turns the missing gmx into a failure.
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

GUI = ("mdoptimize", "mddynamics", "mdenergy", "builder", "organizer")
FIXTURES = os.path.join(REPO, "tests", "gromacs_md", "fixtures")

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
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
    for exe in GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "gromacs", {"GROMACS": gmx}, local=True,
                        keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    say("screenshots: %s" % png)

    # The apps and the scripts their wrappers read, beside the launch
    # binaries the tree install already has.
    for exe in GUI:
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

    display = None
    editors = {}
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        display = xdisplay.Display().__enter__()
        say("X display %s" % display.name)
        cmddir = os.path.join(s.state, "mded")
        os.makedirs(cmddir)
        genv = s.env({"DISPLAY": display.name, "ECCE_TEST_MDED": cmddir,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})

        # ---- the study, made through the classes ------------------------
        rc, out = s.driver("machines", "GROMACS")
        check(rc == 0 and "localhost" in out.split(),
              "the Launcher offers localhost for GROMACS (its CONFIG gives a path)")
        rc, out = s.driver("gromacsstudy", s.userUrl(), "water")
        urls = [l for l in out.splitlines() if l.startswith("file://")]
        if not check(rc == 0 and len(urls) == 4,
                     "a GROMACS study with three tasks was created: %s"
                     % out.strip().replace("\n", " | ")[:300]):
            return 1
        study, opt, eq, dyn = [u.rstrip("/") + "/" for u in urls]
        rundir = os.path.join(s.state, "run")
        for u in (opt, eq, dyn):
            rc, out = s.driver("mdsetup", u, "localhost", rundir, s.user())
            check(rc == 0, "launch settings stored for %s" % u.rstrip("/").rsplit("/", 1)[-1])

        # ---- the Organizer shows it -------------------------------------
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")],
            env=dict(genv, ECCE_ORGANIZER_OPEN=eq), cwd=os.path.join(s.home, "bin"),
            stdout=open(os.path.join(s.state, "organizer.log"), "w"),
            stderr=subprocess.STDOUT, start_new_session=True)
        editors["organizer"] = org
        shot_when_window(display, genv, "Organizer", os.path.join(png, "00-organizer-study.png"), 90)
        stop(org)
        editors.pop("organizer")

        # ---- Optimize: the editor, the Inputs page, a refused launch ----
        say("Optimize")
        ed = Editor(s, display, genv, wrappers, cmddir, "mdoptimize", "MDOptimize")
        editors["opt"] = ed
        if not ed.up():
            return 1
        out = ed.cmd("open " + opt)
        check(out == "ok", "the Optimize task opens in the editor: %s" % out)
        out = ed.cmd("title")
        check("GROMACS" in out and "Optimize" in out, "the window says GROMACS Optimize: %s" % out)
        tabs = ed.cmd("tabs").split("|")
        say("  pages: %s" % tabs)
        check(tabs[0] == "Inputs" and "Optimize" in tabs and "Control" not in tabs
              and "Thermodynamics" not in tabs,
              "Inputs first; NWChem's Control and Thermodynamics pages are not there")
        ed.shots(png, "01-optimize", ["Inputs", "Interactions", "Constraints",
                                      "Optimize", "Files"], "empty")
        out = ed.cmd("inputs")
        check("none attached" in out and "Not ready" in out, "nothing attached yet: %s" % out)
        check(ed.cmd("save") == "ok", "Save generates the input file with nothing attached")
        check(ed.cmd("launch") == "disabled",
              "Launch is disabled without a topology and a structure")
        rc, out = s.launch(opt)
        check(rc != 0 and "No starting structure" in out,
              "launching anyway is refused, in words: %s"
              % re.sub(r"\s+", " ", out.strip().splitlines()[-1] if out.strip() else "")[:140])

        out = ed.cmd("attach-gro " + os.path.join(FIXTURES, "conf.gro"))
        check(out.startswith("ok") and "1398 atoms" in out, "the structure attaches: %s" % out)
        check(ed.cmd("attach-gro " + os.path.join(FIXTURES, "topol.top")).startswith("refused"),
              "a topology is not accepted as a structure")
        out = ed.cmd("attach-top " + os.path.join(FIXTURES, "topol.top"))
        check(out.startswith("ok"), "the topology attaches: %s" % out)
        incl = os.path.join(s.state, "extra.itp")
        open(incl, "w").write("; an include file the topology might read\n")
        out = ed.cmd("attach-inc " + incl)
        check(out.startswith("ok"), "an include file attaches: %s" % out)
        out = ed.cmd("inputs")
        check("conf.gro" in out and "topol.gmxtop" in out and "includes: 1" in out
              and "Ready" in out, "the page lists them and says Ready: %s" % out)
        task = opt.replace("file://", "").rstrip("/")
        files = sorted(os.listdir(os.path.join(task, "Inputs")))
        say("  Inputs/: %s" % files)
        check("topol.gmxtop" in files and "conf.gro" in files and "extra.itp" in files,
              "the files are in the task's Inputs under their own types")
        check(not any(f.endswith(".top") for f in files),
              "no .top is stored, so nothing is typed as an NWChem topology")
        check(ed.cmd("set-steps 100") == "ok", "the step count changes")
        check(ed.cmd("save") == "ok", "Save, with the system attached")
        check(ed.cmd("launch") == "enabled", "Launch is enabled")
        ed.shots(png, "02-optimize", ["Inputs", "Optimize"], "attached")
        mdp = open(os.path.join(task, "Inputs", "gromacs.mdp")).read()
        check(re.search(r"integrator\s*=\s*steep", mdp) and re.search(r"nsteps\s*=\s*100\b", mdp)
              and re.search(r"constraints\s*=\s*h-bonds", mdp),
              "the .mdp is steepest descent, 100 steps, h-bonds constrained")
        state = s.state_of(opt)
        check(state == "ready", "the task is ready: %s" % state)
        ed.close()
        editors.pop("opt")

        say("running Optimize")
        rc, out = s.launch(opt)
        check(rc == 0, "Launch accepted the task: %s" % out.strip().splitlines()[-1:])
        state = s.waitState(opt, 240)
        check(state in ("completed", "loaded"), "Optimize ran to %s" % state)
        props = s.props(opt)
        say("  properties: %s" % props)
        check("TE" in props and "TEVEC" in props and "PRESSVEC" in props,
              "energies were stored: TE, TEVEC, PRESSVEC")
        outs = sorted(os.listdir(os.path.join(task, "Outputs"))) \
            if os.path.isdir(os.path.join(task, "Outputs")) else []
        say("  Outputs/: %s" % outs)
        check("ecceSession_ecceMdOptimize.gro" in outs and "chemsys.pdb" in outs
              and any(o.startswith("gromacs.gmxlog") for o in outs),
              "the final structure (.gro and .pdb) and the log were stored")
        gro1 = os.path.join(task, "Outputs", "ecceSession_ecceMdOptimize.gro")
        rungro = last_run_file(rundir, "ecceSession_ecceMdOptimize.gro")
        pot = gmx_series(gmx, rungro and os.path.dirname(rungro), "Potential")
        te = prop_value(s, opt, "TE")
        check(pot and te is not None and abs(te - pot[-1]) < 0.5,
              "TE is the last Potential gmx energy reads from ener.edr (%s vs %s)"
              % (te, pot[-1] if pot else None))
        check(gmx_check(gmx, gro1), "the stored final structure is a valid .gro (gmx check)")

        # ---- Equilibrate: inherits topology and structure ----------------
        say("Equilibrate")
        ed = Editor(s, display, genv, wrappers, cmddir, "mddynamics", "MDDynamics")
        editors["eq"] = ed
        if not ed.up():
            return 1
        check(ed.cmd("open " + eq) == "ok", "the Equilibrate task opens in the Dynamics editor")
        out = ed.cmd("title")
        check("Equilibrate" in out, "the window says Equilibrate: %s" % out)
        tabs = ed.cmd("tabs").split("|")
        say("  pages: %s" % tabs)
        check(tabs[0] == "Inputs" and "Dynamics" in tabs and "Control" not in tabs,
              "Inputs, Dynamics and no Control page")
        out = ed.cmd("inputs")
        check("(from optimize)" in out and "Ready" in out,
              "the topology and structure are found on the Optimize task: %s" % out)
        check(ed.cmd("set-steps 300") == "ok", "300 steps")
        check(ed.cmd("save") == "ok", "Save")
        check(ed.cmd("launch") == "enabled", "Launch is enabled with nothing attached here")
        ed.shots(png, "03-equilibrate", ["Inputs", "Interactions", "Constraints",
                                         "Dynamics", "Files"], "equilibrate")
        ed.close()
        editors.pop("eq")
        say("running Equilibrate")
        rc, out = s.launch(eq)
        check(rc == 0, "Launch accepted the task")
        state = s.waitState(eq, 300)
        check(state in ("completed", "loaded"), "Equilibrate ran to %s" % state)
        props = s.props(eq)
        check("TEMPVEC" in props and "PRESSURE" in props and "TE" in props,
              "temperature, pressure and total energy were stored: %s" % props)
        etask = eq.replace("file://", "").rstrip("/")
        check(os.path.exists(os.path.join(etask, "Outputs", "ecceSession_ecceMdEquilibrate.gro")),
              "the final structure was stored")
        rundir_eq = last_run_file(rundir, "ecceSession_ecceMdEquilibrate.gro")
        if rundir_eq:
            started = open(os.path.join(os.path.dirname(rundir_eq), "conf.gro")).read()
            check(started == open(gro1).read(),
                  "Equilibrate started from the structure Optimize ended with")
            tpr_top = os.path.join(os.path.dirname(rundir_eq), "topol.top")
            check(os.path.exists(tpr_top) and open(tpr_top).read().startswith("#include"),
                  "and from the topology attached to the first task, staged as topol.top")
            check(os.path.exists(os.path.join(os.path.dirname(rundir_eq), "extra.itp")),
                  "and the include file came along")
        temps = gmx_series(gmx, rundir_eq and os.path.dirname(rundir_eq), "Temperature")
        check(temps and 150 < temps[-1] < 450,
              "the system is near its target temperature (gmx energy: %s K)"
              % (round(temps[-1]) if temps else None))

        # ---- Dynamics -----------------------------------------------------
        say("Dynamics")
        ed = Editor(s, display, genv, wrappers, cmddir, "mddynamics", "MDDynamics")
        editors["dyn"] = ed
        if not ed.up():
            return 1
        check(ed.cmd("open " + dyn) == "ok", "the Dynamics task opens")
        check("Dynamics" in ed.cmd("title") and "GROMACS" in ed.cmd("title"),
              "the window says GROMACS Dynamics")
        check(ed.cmd("set-steps 300") == "ok", "300 steps")
        check(ed.cmd("save") == "ok", "Save")
        ed.shots(png, "04-dynamics", ["Inputs", "Dynamics", "Files"], "dynamics")
        ed.close()
        editors.pop("dyn")
        say("running Dynamics")
        rc, out = s.launch(dyn)
        check(rc == 0, "Launch accepted the task")
        state = s.waitState(dyn, 300)
        check(state in ("completed", "loaded"), "Dynamics ran to %s" % state)
        props = s.props(dyn)
        check("TEVEC" in props and "TEMPVEC" in props, "traces stored: %s" % props)
        dtask = dyn.replace("file://", "").rstrip("/")
        d_run = last_run_file(rundir, "ecceSession_ecceMdDynamics.gro")
        te_gmx = gmx_series(gmx, d_run and os.path.dirname(d_run), "Total-Energy")
        te = prop_value(s, dyn, "TE")
        check(te_gmx and te is not None and abs(te - te_gmx[-1]) < 0.5,
              "TE is the last Total-Energy gmx energy reads (%s vs %s)"
              % (te, te_gmx[-1] if te_gmx else None))

        # ---- the results in the Builder ---------------------------------
        for name, url, panel in (("results-optimize-structure", opt, ""),
                                 ("results-dynamics-energies", dyn, "Geometry Step Plots")):
            env = dict(genv, ECCE_PANEL_FULLSCREEN="1")
            if panel:
                env["ECCE_OPEN_PANEL"] = panel
            b = subprocess.Popen(
                [os.path.join(wrappers, "ecce-builder"), "-context", url], env=env,
                cwd=os.path.join(s.home, "bin"),
                stdout=open(os.path.join(s.state, name + ".log"), "w"),
                stderr=subprocess.STDOUT, start_new_session=True)
            editors[name] = b
            shot_when_window(display, genv, "Builder",
                             os.path.join(png, "05-%s.png" % name), 120, settle=12)
            stop(b)
            editors.pop(name)
    finally:
        for p in editors.values():
            if hasattr(p, "kill"):
                stop(p)
            elif hasattr(p, "close"):
                p.close()
        if display is not None:
            display.__exit__(None, None, None)
        leftovers = s.stop()
        if leftovers:
            say("left running: %s" % leftovers)

    say("%d failure(s)" % len(failures))
    return 1 if failures else 0


# ---- helpers --------------------------------------------------------------

def stop(proc):
    if proc.poll() is None:
        try:
            os.killpg(proc.pid, 15)
            proc.wait(timeout=20)
        except Exception:
            try:
                os.killpg(proc.pid, 9)
            except Exception:
                pass


def shot_when_window(display, env, title, path, timeout, settle=6):
    """Wait for a window whose title holds `title`, then save the root window."""
    deadline = time.time() + timeout
    found = None
    while time.time() < deadline and not found:
        found = next((w for w in display.windows() if title in (w[1] or "")), None)
        time.sleep(0.5)
    check(found is not None, "a %s window opened" % title)
    time.sleep(settle)
    subprocess.run(["import", "-window", "root", path], env=env, timeout=60,
                   stderr=subprocess.DEVNULL)
    check(os.path.exists(path), "screenshot %s" % os.path.basename(path))


class Editor(object):
    """An MD task editor, driven through ECCE_TEST_MDED."""

    def __init__(self, s, display, env, wrappers, cmddir, binary, appname):
        self.display, self.env, self.cmddir, self.appname = display, env, cmddir, appname
        self.cmdfile = os.path.join(cmddir, appname + ".cmd")
        self.outfile = os.path.join(cmddir, appname + ".out")
        for f in (self.cmdfile, self.outfile):
            if os.path.exists(f):
                os.unlink(f)
        open(self.cmdfile, "w").close()
        self.count = 0
        before = set(w for w, _ in display.windows())
        self.log = open(os.path.join(s.state, binary + ".log"), "a")
        self.proc = subprocess.Popen(
            [os.path.join(wrappers, "ecce-" + binary)], env=env,
            cwd=os.path.join(s.home, "bin"), stdout=self.log,
            stderr=subprocess.STDOUT, start_new_session=True)
        self.before = before

    def up(self):
        deadline = time.time() + 120
        while time.time() < deadline and self.proc.poll() is None:
            if [w for w in self.display.windows() if w[0] not in self.before and w[1]]:
                break
            time.sleep(0.5)
        ok = check(self.proc.poll() is None and any(
            w[0] not in self.before and w[1] for w in self.display.windows()),
            "%s opened a window" % self.appname)
        time.sleep(3)
        return ok

    def cmd(self, line, timeout=90):
        with open(self.cmdfile, "a") as h:
            h.write(line + "\n")
        self.count += 1
        deadline = time.time() + timeout
        while time.time() < deadline:
            if os.path.exists(self.outfile):
                lines = open(self.outfile).read().splitlines()
                if len(lines) >= self.count:
                    answer = lines[self.count - 1]
                    prefix = line + ": "
                    return answer[len(prefix):] if answer.startswith(prefix) else answer
            time.sleep(0.3)
        return "TIMEOUT"

    def shots(self, png, prefix, tabs, tag):
        for tab in tabs:
            if self.cmd("tab " + tab) != "ok":
                check(False, "page %s exists" % tab)
                continue
            time.sleep(1)
            path = os.path.join(png, "%s-%s.png" % (prefix, tab.lower()))
            r = self.cmd("snap " + path)
            check(r == "saved" and os.path.exists(path), "window shot: %s" % os.path.basename(path))

    def close(self):
        try:
            self.cmd("quit", timeout=10)
        except Exception:
            pass
        stop(self.proc)
        self.log.close()


def last_run_file(rundir, name):
    found = sorted(glob.glob(os.path.join(rundir, "**", name), recursive=True),
                   key=os.path.getmtime)
    return found[-1] if found else None


def gmx_series(gmx, directory, term):
    if not directory:
        return []
    edr = os.path.join(directory, "ener.edr")
    if not os.path.exists(edr):
        return []
    subprocess.run([gmx, "energy", "-f", edr, "-o", "gmxtest.xvg"],
                   input=(term + "\n0\n").encode(), cwd=directory,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    vals = []
    try:
        for line in open(os.path.join(directory, "gmxtest.xvg")):
            if line.strip() and line[0] not in "#@":
                vals.append(float(line.split()[1]))
    except OSError:
        pass
    return vals


def gmx_check(gmx, gro):
    r = subprocess.run([gmx, "check", "-f", gro], stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
    return r.returncode == 0 and b"Coords" in r.stdout


def prop_value(s, url, key):
    rc, out = s.driver("props", url)
    task = url.replace("file://", "").rstrip("/")
    for path in glob.glob(os.path.join(task, "Props", key + "*")):
        text = open(path, errors="replace").read()
        nums = re.findall(r"-?\d+\.\d+(?:[eE][-+]?\d+)?", text)
        if nums:
            return float(nums[-1])
    return None


if __name__ == "__main__":
    sys.exit(main())
