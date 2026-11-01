#!/usr/bin/env python3
"""A running GROMACS study task offers Terminate in the Organizer's
right-click menu, and Terminate ends it (#731).

The study's Equilibrate task runs a stand-in gmx that sleeps.  While it
runs, the tree's context menu for that task (the Organizer hook
`treecontextmenu`) must list "Terminate Calculation..."; before the launch
it must not.  ecce-terminate goes through the same terminateCalc and must
end the task as Killed.

    tests/launch/md_terminate_test.py [--build DIR] [--keep]

Exit status 77 (CTest SKIP) without Xvfb, gmx or the built binaries.
"""

import argparse
import glob
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

FIXTURES = os.path.join(REPO, "tests", "gromacs_md", "fixtures")
LABEL = "Terminate Calculation..."
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def monitoring(s, name):
    pattern = os.path.join(s.state, "tmp", "*", "jobs", name + "__*",
                           "eccejobstore.log")
    for _ in range(120):
        for log in glob.glob(pattern):
            try:
                if "job monitor running on" in open(log, errors="replace").read():
                    return True
            except OSError:
                pass
        time.sleep(1)
    return False


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
    for exe in ("organizer", "ecce-terminate"):
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "mdterminate", {"GROMACS": gmx}, local=True,
                        keep=args.keep)
    slow = os.path.join(s.state, "slow-gmx")
    with open(slow, "w") as h:
        h.write("#!/bin/sh\nsleep 300\nexec %s \"$@\"\n" % gmx)
    os.chmod(slow, 0o755)
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
    s.registerMachine("localhost", "Shell", {"GROMACS": slow})

    display = org = None
    try:
        if not check(s.services(True), "services came up"):
            return 1
        rc, out = s.driver("gromacsstudy", s.userUrl(), "term")
        urls = [l for l in out.splitlines() if l.startswith("file://")]
        if not check(rc == 0 and len(urls) == 4, "a GROMACS study was created"):
            return 1
        eq = urls[2].rstrip("/") + "/"
        name = eq.rstrip("/").rsplit("/", 1)[-1]
        rundir = os.path.join(s.state, "run")
        check(s.driver("mdsetup", eq, "localhost", rundir, s.user())[0] == 0,
              "launch settings stored")
        check(s.driver("mdfile", eq, os.path.join(FIXTURES, "conf.gro"),
                       "start.gro")[0] == 0, "structure attached")
        check(s.driver("mdfile", eq, os.path.join(FIXTURES, "topol.top"),
                       "topol.gmxtop")[0] == 0, "topology attached")
        check(s.driver("mdmdp", eq)[0] == 0, ".mdp written")

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")],
            env=dict(genv, ECCE_TEST_ORGANIZER=cmds),
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        def send(line, timeout=300):
            seen = len(open(olog, errors="replace").read().splitlines())
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

        ans = send("treecontextmenu %s" % eq)
        check(ans.startswith("ok") and LABEL not in ans,
              "not running: no Terminate in the menu (%s)" % ans)

        rc, out = s.launch(eq)
        check(rc == 0 and monitoring(s, name), "the task runs under its monitor")
        ans = send("treecontextmenu %s" % eq)
        check("|" + LABEL in ans, "running: the menu offers Terminate (%s)" % ans)

        r = subprocess.run([os.path.join(build, "ecce-terminate"), eq],
                           env=s.env(), stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=120)
        out = r.stdout.decode("utf-8", "replace")
        say(out)
        check(r.returncode == 0 and "terminated" in out,
              "ecce-terminate ends the task (rc=%s)" % r.returncode)
        st = s.waitState(eq, seconds=240)
        check(st == "killed", "the task is Killed (%s)" % st)
    finally:
        if org and org.poll() is None:
            org.terminate()
        if display:
            display.__exit__(None, None, None)
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)

    say("FAILED: %s" % failures if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
