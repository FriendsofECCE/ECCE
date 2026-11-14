#!/usr/bin/env python3
"""A plain GROMACS run made from the user's own files (#1038), launched.

File > New GROMACS Run from Files... through its Organizer hook
`gmxrun PARENT MDP|- GRO|- TOP|- [TPR]`: the inputs are stored, Launch ships
them, the job script runs grompp (not for a .tpr) and mdrun, and the
monitor stores energies, the trajectory and the final structure.

    tests/launch/gromacs_run_test.py [--build DIR] [--keep]

Cases: the small water fixture (.mdp + .gro + .top); the same system as a
ready .tpr; real MINFF inputs with a force-field folder and an index file,
when they exist on this machine.  Exit status 77 (CTest SKIP) without gmx,
Xvfb or the built binaries.
"""

import argparse
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

FIX = os.path.join(REPO, "tests", "e2e", "fixtures", "gromacs")
MINFF = "/home/andy/tmp/Andy/0_MD_gromacs_minff/minff_MD"


def where(u):
    return u[len("file://"):].rstrip("/")


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
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "gmxrun", {"GROMACS": gmx}, local=True,
                        keep=args.keep)
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
    s.registerMachine("localhost", "Shell", {"GROMACS": gmx})
    work = os.path.join(s.state, "inputs")
    os.makedirs(work)

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

        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")],
            env=dict(genv, ECCE_TEST_ORGANIZER=cmds),
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        def send(line, timeout=120):
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

        def run_it(case, args_, expect_inputs, name):
            ans = send("gmxrun %s %s" % (proj, args_))
            if not check(ans.startswith("ok "), "%s: gmxrun: %s" % (case, ans)):
                return None
            url = ans.split()[1].rstrip("/") + "/"
            check(url.rstrip("/").endswith("/" + name),
                  "%s: the calculation is named %s: %s" % (case, name, url))
            task = where(url)
            have = sorted(os.listdir(os.path.join(task, "Inputs")))
            say("  Inputs/: %s" % have)
            for f in expect_inputs:
                check(f in have, "%s: %s is stored" % (case, f))
            check(s.state_of(url) == "ready", "%s: the run is Ready" % case)
            rundir = os.path.join(s.state, "run-" + case)
            rc, out = s.driver("mdsetup", url, "localhost", rundir, s.user())
            check(rc == 0, "%s: launch settings stored" % case)
            rc, out = s.launch(url)
            check(rc == 0, "%s: Launch accepted the run: %s"
                  % (case, out.strip().splitlines()[-3:]))
            state = s.waitState(url, 600)
            check(state in ("completed", "loaded"), "%s: ran to %s" % (case, state))
            props = s.props(url)
            say("  properties: %s" % props)
            check("TE" in props or "TEVEC" in props,
                  "%s: energies were stored" % case)
            outs = sorted(os.listdir(os.path.join(task, "Outputs"))) \
                if os.path.isdir(os.path.join(task, "Outputs")) else []
            say("  Outputs/: %s" % outs)
            check(any(o.endswith(".gro") for o in outs) and "chemsys.pdb" in outs,
                  "%s: the final structure (.gro, chemsys.pdb) was stored" % case)
            check(any(o.endswith(".xtc") or o.endswith(".trr") for o in outs)
                  or any(o.endswith(".edr") for o in outs),
                  "%s: the trajectory or energy file was stored" % case)
            return url

        # ---- 1: .mdp + .gro + .top -------------------------------------
        say("case 1: .mdp, .gro, .top")
        mdp, gro, top = (os.path.join(FIX, f)
                         for f in ("gromacs.mdp", "conf.gro", "topol.top"))
        run_it("files", "%s %s %s" % (mdp, gro, top),
               ("gromacs.mdp", "conf.gro", "topol.gmxtop"), "gromacs")

        # ---- 2: a ready .tpr --------------------------------------------
        say("case 2: a ready .tpr")
        tpr = os.path.join(work, "ready.tpr")
        p = subprocess.run([gmx, "grompp", "-f", mdp, "-c", gro, "-p", top,
                            "-o", tpr, "-maxwarn", "99"], cwd=work,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if check(p.returncode == 0 and os.path.exists(tpr), "a .tpr was made"):
            run_it("tpr", "- - - %s" % tpr, ("run.gmxtpr", "gromacs.mdp"), "ready")

        # ---- 3: MINFF, force-field folder and index file ------------------
        em = os.path.join(MINFF, "EM", "em.mdp")
        pre = os.path.join(MINFF, "gmx_minff", "preem.gro")
        mtop = os.path.join(MINFF, "gmx_minff", "topol.top")
        if all(os.path.exists(f) for f in (em, pre, mtop)):
            say("case 3: MINFF")
            short = os.path.join(work, "em.mdp")
            text = re.sub(r"(?m)^nsteps\b.*$", "nsteps = 20", open(em).read())
            open(short, "w").write(text)
            run_it("minff", "%s %s %s" % (short, pre, mtop),
                   ("gromacs.mdp", "preem.gro", "topol.gmxtop"), "em")
            ans = send("gmxrun %s %s %s %s" % (proj, short, pre, mtop))
            check(ans.startswith("ok ") and ans.split()[1].rstrip("/").endswith("/em_1"),
                  "a second run of the same name is made unique: %s" % ans)
        else:
            say("case 3: the MINFF inputs are not on this machine; skipped")
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
