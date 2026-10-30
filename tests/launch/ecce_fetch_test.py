#!/usr/bin/env python3
"""ecce-fetch: the command line asks the running Organizer to copy files from
a finished calculation's run directory into its stored Outputs.

A local-machine job finishes; x.cube and y.pdos are then written into its run
directory. ecce-fetch with no pattern stores both; --list shows them; a
pattern that matches nothing says so; a pattern with '/' is refused; a path
the Organizer does not know is "not a calculation in this session".

    tests/launch/ecce_fetch_test.py [--build DIR] [--keep]

Exit status 77 (CTest SKIP) without Xvfb or the built binaries.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

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
    import time
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
    exe = os.path.join(build, "ecce-fetch")
    if not os.access(exe, os.X_OK):
        say("FAIL: ecce-fetch is not built in %s" % build)
        return 1
    import xdisplay

    s = harness.Session(build, "ecce-fetch", {"CP2K": "/bin/true"},
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
        url, out = s.create("fin", "cp2k_es", deck, "cp2k.cp2kin", rundir,
                            extra=("procs=1", "queue=normal", "nodes=1",
                                   "wall=0 1:0"))
        if not check(url is not None, "calculation created"):
            say(out)
            return 1
        rc, out = s.launch(url)
        check(rc == 0, "launched")
        st = s.waitState(url, seconds=240)
        check(st in ("completed", "failed", "unsuccessful"),
              "the job has finished (%s)" % st)

        #  The run directory is the folder that holds the deck's copy.
        hits = []
        for root, dirs, files in os.walk(rundir):
            if "cp2k.cp2kin" in files:
                hits.append(root)
        if not check(len(hits) == 1, "one run directory found (%r)" % hits):
            return 1
        payload = {"x.cube": "cube data\n" * 10, "y.pdos": "pdos data\n" * 7}
        for name, text in payload.items():
            open(os.path.join(hits[0], name), "w").write(text)
        outputs = os.path.join(url[len("file://"):], "Outputs")

        def run(*argv, **kw):
            r = subprocess.run([exe] + list(argv), env=s.env(kw.get("env")),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=300)
            return r.returncode, r.stdout.decode("utf-8", "replace"), \
                r.stderr.decode("utf-8", "replace")

        empty = tempfile.mkdtemp(prefix="ecce-fetch-nosession-")
        try:
            rc, out, err = run(url, env={
                "ECCE_SESSION_ID": "0123456789abcdef",
                "ECCE_REALUSERHOME": empty})
        finally:
            shutil.rmtree(empty, ignore_errors=True)
        check(rc == 2 and "ECCE is not running: start ECCE first" in err,
              "without a session: exit 2 and the message (rc=%s %r)" % (rc, err))
        rc, out, err = run()
        check(rc == 64, "no argument: usage, exit 64 (rc=%s)" % rc)
        rc, out, err = run(url, "a/b")
        check(rc == 64, "a pattern with '/' is refused, exit 64 (rc=%s)" % rc)

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")], env=genv,
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        #  The Organizer needs a moment to be reachable.
        end = time.time() + 120
        while time.time() < end:
            rc, out, err = run("--list", url)
            if rc != 2:
                break
            time.sleep(3)
        say(out)
        say(err)
        check(rc == 0 and "x.cube" in out and "y.pdos" in out,
              "--list shows both files (rc=%s)" % rc)
        check(not os.path.exists(os.path.join(outputs, "x.cube")),
              "--list stored nothing")

        rc, out, err = run(url)
        say(out)
        say(err)
        lines = out.splitlines()
        check(rc == 0, "default patterns: *.cub matched nothing, still exit 0 (%s)" % rc)
        check("x.cube: fetched (100 bytes)" in lines, "x.cube is fetched")
        check("y.pdos: fetched (70 bytes)" in lines, "y.pdos is fetched")
        check("*.cub: no file matched" in lines, "*.cub says no file matched")
        for name, text in payload.items():
            p = os.path.join(outputs, name)
            check(os.path.exists(p) and open(p).read() == text,
                  "%s is in the stored Outputs" % name)

        open(os.path.join(hits[0], "x.cube"), "w").write("new\n")
        rc, out, err = run(url, "x.cube", "*.nothing")
        check("x.cube: fetched (4 bytes)" in out.splitlines() and
              "*.nothing: no file matched" in out.splitlines() and rc == 0,
              "a refetch replaces the stored file and a miss is reported")
        check(open(os.path.join(outputs, "x.cube")).read() == "new\n",
              "the stored x.cube is the new one")

        bogus = os.path.join(s.state, "run", "no-such-calculation")
        rc, out, err = run("file://" + bogus)
        say(out)
        check(rc == 1 and "not fetched: not a calculation in this session" in out,
              "an unknown path is not a calculation (rc=%s)" % rc)
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
