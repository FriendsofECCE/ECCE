#!/usr/bin/env python3
"""ecce-launch (#687): the command line launches Ready calculations inside
the running Organizer's session.

Two Ready calculations (one given as a path, one as a URL) and one that is
already submitted go to `ecce-launch` against an isolated session with the
Organizer running; each gets its line and the exit status is 1. A second
call on the launched ones is refused. Without a session the command says so
and exits 2.

    tests/launch/ecce_launch_test.py [--build DIR] [--keep]

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
    exe = os.path.join(build, "ecce-launch")
    if not os.access(exe, os.X_OK):
        say("FAIL: ecce-launch is not built in %s" % build)
        return 1
    import xdisplay

    s = harness.Session(build, "ecce-launch", {"CP2K": "/bin/true"},
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
        urls = {}
        for name in ("one", "two", "gone"):
            url, out = s.create(name, "cp2k_es", deck, "cp2k.cp2kin", rundir,
                                extra=("procs=1", "queue=normal", "nodes=1",
                                       "wall=0 1:0"))
            if not check(url is not None, "calculation %s created" % name):
                say(out)
                return 1
            urls[name] = url
        #  "gone" is submitted outside the Organizer first: not Ready any more.
        rc, out = s.launch(urls["gone"])
        st = s.waitState(urls["gone"], seconds=120,
                         want=harness.FINAL_STATES + ("submitted", "running"))
        check(st not in ("ready", "created", ""), "gone is no longer ready (%s)" % st)

        def run(*argv, **kw):
            r = subprocess.run([exe] + list(argv), env=s.env(kw.get("env")),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=120)
            return r.returncode, r.stdout.decode("utf-8", "replace"), \
                r.stderr.decode("utf-8", "replace")

        #  No session: said so, nothing launched.
        empty = tempfile.mkdtemp(prefix="ecce-launch-nosession-")
        try:
            rc, out, err = run(urls["one"], env={
                "ECCE_SESSION_ID": "0123456789abcdef",
                "ECCE_REALUSERHOME": empty})
        finally:
            shutil.rmtree(empty, ignore_errors=True)
        check(rc == 2 and "ECCE is not running: start ECCE first" in err,
              "without a session: exit 2 and the message (rc=%s %r)" % (rc, err))
        check(s.state_of(urls["one"]) == "ready", "nothing was launched without a session")

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")], env=genv,
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        #  The Organizer needs a moment to subscribe: ack arrives, or retry.
        path_one = urls["one"][len("file://"):]
        import time
        rc = out = err = None
        end = time.time() + 120
        while time.time() < end:
            rc, out, err = run(path_one, urls["two"], urls["gone"])
            if rc != 2:
                break
            time.sleep(3)
        say(out)
        say(err)
        check(rc == 1, "exit status 1 as one calculation was not launched (%s)" % rc)
        lines = out.splitlines()
        check(any(re.match(r"^one: job id \S+$", l) for l in lines),
              "path argument: 'one: job id N'")
        check(any(re.match(r"^two: job id \S+$", l) for l in lines),
              "URL argument: 'two: job id N'")
        check(any(l.startswith("gone: not launched: not ready") for l in lines),
              "submitted calculation refused with the reason")
        for name in ("one", "two"):
            st = s.waitState(urls[name], seconds=120,
                             want=harness.FINAL_STATES + ("submitted", "running"))
            check(st not in ("ready", "created", ""), "%s left ready: %s" % (name, st))

        rc, out, err = run(path_one, urls["two"])
        say(out)
        check(rc == 1 and out.count("not launched: not ready") == 2 and
              "job id" not in out,
              "a second call on the launched calculations is refused (rc=%s)" % rc)
        rc, out, err = run(os.path.join(s.state, "decks"))
        check(rc == 1 and "not inside the data folder" in out,
              "a path outside the data folder is refused (rc=%s)" % rc)
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
