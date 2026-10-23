#!/usr/bin/env python3
"""Organizer > Run Mgmt > Launch Selected (#492): several ready calculations
are submitted in one go with the launch settings each stores.

Three CP2K calculations (the code is /bin/true) and one with no machine are
made in a local data folder; the Organizer hook `launchselected URL...` runs
the batch without its dialogs, and `launchselectedshot PNG URL...` saves the
summary dialog.  Every complete calculation must leave "ready" through the
real Launch, and the one without settings must be skipped.

    tests/launch/launch_selected_test.py [--build DIR] [--keep] [--shot PNG]

Exit status 77 (CTest SKIP) without Xvfb or the built binaries.
"""

import argparse
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

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--shot", default="")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "launchsel", {"CP2K": "/bin/true"},
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
    s.registerMachine("localhost", "Shell", {"CP2K": "/bin/true"})
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    os.makedirs(work, exist_ok=True)
    os.makedirs(rundir, exist_ok=True)
    deck = os.path.join(work, "a.cp2kin")
    open(deck, "w").write("&GLOBAL\n  RUN_TYPE ENERGY\n&END GLOBAL\n")

    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        urls = []
        for n in range(3):
            url, out = s.create("sel%d" % n, "cp2k_es", deck, "cp2k.cp2kin",
                                rundir, extra=("procs=1", "queue=normal",
                                               "nodes=1", "wall=0 1:0"))
            if not check(url is not None, "calculation %d created" % n):
                say(out)
                return 1
            urls.append(url)
        #  No machine stored: listed, skipped.
        bad, out = s.create("nomachine", "cp2k_es", deck, "cp2k.cp2kin",
                            rundir, machine="", extra=("procs=1",))
        say("  nomachine: %s" % (bad or out.strip()[-120:]))
        states = [s.state_of(u) for u in urls]
        check(all(x == "ready" for x in states), "all start ready: %s" % states)

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

        shot = args.shot or os.path.join(s.state, "summary.png")
        every = urls + ([bad] if bad else [])
        ans = send("launchselectedshot %s %s" % (shot, " ".join(every)))
        check(ans == "not launched" and os.path.getsize(shot) > 1000,
              "summary dialog saved (%s)" % ans)
        check(all(s.state_of(u) == "ready" for u in urls),
              "the summary alone submits nothing")

        ans = send("launchselected %s" % " ".join(every))
        say("  answer: %s" % ans)
        check(ans.startswith("ok "), "batch answered ok")
        for n in range(3):
            check(("sel%d: submitted" % n) in ans, "sel%d reported submitted" % n)
        if bad:
            check("nomachine: skipped" in ans, "the calculation without settings is skipped")
        for n, u in enumerate(urls):
            st = s.waitState(u, seconds=120,
                             want=harness.FINAL_STATES + ("submitted", "running"))
            check(st not in ("ready", "created", ""), "sel%d left ready: %s" % (n, st))
    finally:
        if org and org.poll() is None:
            org.terminate()
        if display:
            display.__exit__(None, None, None)
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)

    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
