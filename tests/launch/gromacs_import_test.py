#!/usr/bin/env python3
"""The GROMACS run importer (#375): a finished run becomes a study, headless.

A run made outside ECCE (the parser fixtures: a Prepare log, a steepest-
descents minimisation and a 500-step NVT run) is laid out one directory per
task and imported by the Organizer hook `gmximport PARENT RUNDIR`.  The study
must hold the three tasks with their inputs attached as a live run's are,
and the job monitor must have stored the energies from the logs.

    tests/launch/gromacs_import_test.py [--build DIR] [--keep]

Exit status 77 (CTest SKIP) without Xvfb or the built binaries.
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
from gromacs_test import check, failures, stop  # noqa: E402

PARSERS = os.path.join(REPO, "tests", "parsers", "fixtures", "gromacs")
MDFIX = os.path.join(REPO, "tests", "gromacs_md", "fixtures")


def where(u):
    return u[len("file://"):].rstrip("/")


def lay_out(run):
    """The fixtures as a run: one directory per task, in run order."""
    def put(task, src, name=None):
        d = os.path.join(run, task)
        os.makedirs(d, exist_ok=True)
        shutil.copy(src, os.path.join(d, name or os.path.basename(src)))

    put("1-prepare", os.path.join(PARSERS, "prepare.gmxlog"))
    put("2-optimize", os.path.join(PARSERS, "water_em.mdp"))
    put("2-optimize", os.path.join(MDFIX, "topol.top"))
    put("2-optimize", os.path.join(MDFIX, "conf.gro"))
    put("2-optimize", os.path.join(PARSERS, "water_em.log"))
    put("2-optimize", os.path.join(MDFIX, "conf.gro"), "confout.gro")
    put("3-dynamics", os.path.join(PARSERS, "water_md.mdp"))
    put("3-dynamics", os.path.join(PARSERS, "water_md.log"))


def last_te(log, term="Total Energy"):
    """The Total Energy of the last step block of an mdrun log (not the
    averages): the oracle for what the monitor stored."""
    text = open(log, errors="replace").read().split("A V E R A G E S")[0]
    lines = text.splitlines()
    last = None
    for i, line in enumerate(lines[:-1]):
        if term in line:
            cells = [line[k:k + 15] for k in range(0, len(line), 15)]
            vals = [lines[i + 1][k:k + 15] for k in range(0, len(lines[i + 1]), 15)]
            for c, v in zip(cells, vals):
                if term in c:
                    try:
                        last = float(v)
                    except ValueError:
                        pass  # the .mdp echo, not an energy block
    return last


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "gmximport", {"GROMACS": "/bin/true"},
                        local=True, keep=args.keep)
    harness.link(os.path.join(build, "organizer"),
                 os.path.join(s.home, "bin", "organizer"))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    s.registerMachine("localhost", "Shell", {"GROMACS": "/bin/true"})
    run = os.path.join(s.state, "gmxrun")
    lay_out(run)

    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
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
            # Only answers after this command: a repeated command must not
            # find the answer to its earlier copy.
            seen = len(open(olog, errors="replace").read().splitlines()) \
                if os.path.exists(olog) else 0
            with open(cmds, "a") as h:
                h.write(line + "\n")
            want = "ECCE_TEST_ORGANIZER: %s: " % line
            end = time.time() + timeout
            while time.time() < end:
                if org.poll() is not None:
                    return "the Organizer ended (%s)" % org.returncode
                for t in open(olog, errors="replace").read().splitlines()[seen:]:
                    if t.startswith(want):
                        return t[len(want):]
                time.sleep(0.5)
            return "no answer"

        ans = send("gmximport %s %s" % (proj, run))
        check(ans.startswith("ok "), "gmximport: %s" % ans)
        study = [x for x in os.listdir(pdir) if x not in before
                 and os.path.isdir(os.path.join(pdir, x, "Inputs")) is False]
        sdir = os.path.join(pdir, study[0]) if study else ""
        tasks = sorted(d for d in os.listdir(sdir)
                       if os.path.isdir(os.path.join(sdir, d))) if sdir else []
        say("study %s holds %s" % (study, tasks))
        check(len(tasks) == 3 and set(tasks) == {"prepare", "optimize", "dynamics"},
              "the study holds Prepare, Optimize and Dynamics")
        if len(tasks) != 3:
            return 1
        t = {k: os.path.join(sdir, k) for k in tasks}

        inputs = sorted(os.listdir(os.path.join(t["optimize"], "Inputs")))
        say("optimize inputs: %s" % inputs)
        check("gromacs.mdp" in inputs and "topol.gmxtop" in inputs
              and "conf.gro" in inputs,
              "the Optimize carries its .mdp, topology and start structure")
        check(open(os.path.join(t["optimize"], "Inputs", "gromacs.mdp")).read()
              == open(os.path.join(PARSERS, "water_em.mdp")).read(),
              "the stored .mdp is the run's")
        check("gromacs.mdp" in os.listdir(os.path.join(t["dynamics"], "Inputs")),
              "the Dynamics carries its .mdp")

        # The monitor stores the properties after the call returned.
        end = time.time() + 180
        while time.time() < end:
            have = {k: [os.path.basename(p) for p in
                        glob.glob(os.path.join(t[k], "Props", "*"))] for k in t}
            if all(any(n.startswith("TE") for n in have[k])
                   for k in ("optimize", "dynamics")) and \
                    any(n.startswith("NATOMS") for n in have["prepare"]):
                time.sleep(5)
                break
            time.sleep(2)
        say("properties: %s" % have)
        check(any(n.startswith("TE") for n in have["optimize"]),
              "the Optimize has the total energy")
        check(any(n.startswith("TE") for n in have["dynamics"]),
              "the Dynamics has the total energy")
        check(any(n.startswith("NATOMS") for n in have["prepare"]),
              "the Prepare has its system size")
        for kind, log, term in (("optimize", "water_em.log", "Potential"),
                                ("dynamics", "water_md.log", "Total Energy")):
            from gromacs_test import prop_value
            te = prop_value(s, "file://" + t[kind], "TE")
            want = last_te(os.path.join(PARSERS, log), term)
            check(te is not None and want is not None and abs(te - want) < 0.01,
                  "%s: TE is the log's last step (%s vs %s)" % (kind, te, want))
        outs = os.listdir(os.path.join(t["optimize"], "Outputs")) \
            if os.path.isdir(os.path.join(t["optimize"], "Outputs")) else []
        check(any(o.endswith(".gro") for o in outs),
              "the Optimize stores its final structure: %s" % outs)
        # File > Import GROMACS Run...: the study is named after the
        # directory, and a second import beside it gets another name.
        base = os.path.basename(run.rstrip("/"))
        for want in (base, base + "_1"):
            ans = send("gmximportsel %s %s" % (proj, run))
            check(ans.startswith("ok ") and ans.rstrip("/").endswith("/" + want),
                  "gmximportsel makes %s: %s" % (want, ans))
    finally:
        if org is not None:
            stop(org)
        if display is not None:
            display.__exit__(None, None, None)
        leftovers = s.stop()
        if leftovers:
            say("left running: %s" % leftovers)
    say("%d failure(s)" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
