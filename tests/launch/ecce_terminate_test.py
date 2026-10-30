#!/usr/bin/env python3
"""ecce-terminate (#695): the command line cancels running calculations
inside the running Organizer's session.

Two calculations run a stand-in that sleeps; a third is Ready. One call with
--reason names the first (a path), the second (a URL) and the Ready one:
the running ones are killed and keep the reason as run status reason, the
Ready one is refused with its state and the exit status is 1. A calculation
terminated without --reason has no reason. Without a session the command
says so and exits 2.

    tests/launch/ecce_terminate_test.py [--build DIR] [--keep]

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


def monitoring(s, url):
    """The job monitor is watching the job (the state stays submitted
    until the code writes output)."""
    import glob
    import time
    name = url.rstrip("/").rsplit("/", 1)[1]
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
    exe = os.path.join(build, "ecce-terminate")
    if not os.access(exe, os.X_OK):
        say("FAIL: ecce-terminate is not built in %s" % build)
        return 1
    import xdisplay

    s = harness.Session(build, "ecce-terminate", {"CP2K": "/bin/true"},
                        local=True, keep=args.keep)
    slow = os.path.join(s.state, "slow-cp2k")
    with open(slow, "w") as h:
        h.write("#!/bin/sh\nsleep 300\nexec /bin/true \"$@\"\n")
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
    s.registerMachine("localhost", "Shell", {"CP2K": slow})
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
        for name in ("one", "two", "three", "idle"):
            url, out = s.create(name, "cp2k_es", deck, "cp2k.cp2kin", rundir,
                                extra=("procs=1", "queue=normal", "nodes=1",
                                       "wall=0 1:0"))
            if not check(url is not None, "calculation %s created" % name):
                say(out)
                return 1
            urls[name] = url
        for name in ("one", "two", "three"):
            rc, out = s.launch(urls[name])
            check(rc == 0 and monitoring(s, urls[name]),
                  "%s is running under its monitor" % name)

        def run(*argv, **kw):
            r = subprocess.run([exe] + list(argv), env=s.env(kw.get("env")),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=120)
            return r.returncode, r.stdout.decode("utf-8", "replace"), \
                r.stderr.decode("utf-8", "replace")

        #  No session: said so, nothing terminated.
        empty = tempfile.mkdtemp(prefix="ecce-terminate-nosession-")
        try:
            rc, out, err = run(urls["one"], env={
                "ECCE_SESSION_ID": "0123456789abcdef",
                "ECCE_REALUSERHOME": empty})
        finally:
            shutil.rmtree(empty, ignore_errors=True)
        check(rc == 2 and "ECCE is not running: start ECCE first" in err,
              "without a session: exit 2 and the message (rc=%s %r)" % (rc, err))
        check(s.state_of(urls["one"]) in ("submitted", "running"),
              "nothing was terminated without a session")
        rc, out, err = run()
        check(rc == 64, "no argument: usage, exit 64 (rc=%s)" % rc)

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")], env=genv,
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        reason = "stopped by the test: wrong basis"
        path_one = urls["one"][len("file://"):]
        rc = out = err = None
        end = time.time() + 120
        while time.time() < end:
            rc, out, err = run("--reason", reason, path_one, urls["two"],
                               urls["idle"])
            if rc != 2:
                break
            time.sleep(3)
        say(out)
        say(err)
        check(rc == 1, "exit status 1 as one calculation was refused (%s)" % rc)
        lines = out.splitlines()
        check(any(re.match(r"^one: terminated, job id \S+$", l) for l in lines),
              "path argument: 'one: terminated, job id N'")
        check(any(re.match(r"^two: terminated, job id \S+$", l) for l in lines),
              "URL argument: 'two: terminated, job id N'")
        check(any(l.startswith("idle: not terminated: ") and "state is" in l
                  for l in lines), "a Ready calculation is refused with its state")
        for name in ("one", "two"):
            st = s.waitState(urls[name], seconds=240)
            check(st == "killed", "%s is killed (%s)" % (name, st))
            end = time.time() + 60
            got = ""
            while time.time() < end:
                got = s.reason_of(urls[name])
                if got == reason:
                    break
                time.sleep(2)
            check(got == reason, "%s keeps the reason after the job store's "
                  "final pass (%r)" % (name, got))
        check(s.state_of(urls["idle"]) in ("ready", "created"),
              "the Ready calculation was left alone")

        rc, out, err = run(urls["three"])
        say(out)
        check(rc == 0 and re.search(r"^three: terminated, job id \S+$", out, re.M),
              "without --reason: terminated, exit 0 (rc=%s)" % rc)
        st = s.waitState(urls["three"], seconds=240)
        check(st == "killed", "three is killed (%s)" % st)
        check(s.reason_of(urls["three"]) == "", "three has no reason")

        rc, out, err = run(urls["one"])
        say(out)
        check(rc == 1 and "not terminated: state is killed" in out,
              "a second call on a killed calculation is refused (rc=%s)" % rc)
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
