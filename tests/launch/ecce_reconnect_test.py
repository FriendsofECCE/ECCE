#!/usr/bin/env python3
"""ecce-reconnect: the command line asks the running Organizer to reconnect
calculations, as Run Mgmt > Reconnect Selected does.

One calculation runs a stand-in that sleeps and is reconnected by path
(ok, with its job id); a second is terminated first and is refused as
already finished; a third is Ready and is refused as not launched; a path
the Organizer does not know is "not a calculation in this session". Without
a session the command says so and exits 2.

    tests/launch/ecce_reconnect_test.py [--build DIR] [--keep]

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
    exe = os.path.join(build, "ecce-reconnect")
    if not os.access(exe, os.X_OK):
        say("FAIL: ecce-reconnect is not built in %s" % build)
        return 1
    import xdisplay

    s = harness.Session(build, "ecce-reconnect", {"CP2K": "/bin/true"},
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
        for name in ("one", "done", "idle"):
            url, out = s.create(name, "cp2k_es", deck, "cp2k.cp2kin", rundir,
                                extra=("procs=1", "queue=normal", "nodes=1",
                                       "wall=0 1:0"))
            if not check(url is not None, "calculation %s created" % name):
                say(out)
                return 1
            urls[name] = url
        for name in ("one", "done"):
            rc, out = s.launch(urls[name])
            check(rc == 0 and monitoring(s, urls[name]),
                  "%s is running under its monitor" % name)

        def run(*argv, **kw):
            r = subprocess.run([exe] + list(argv), env=s.env(kw.get("env")),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=300)
            return r.returncode, r.stdout.decode("utf-8", "replace"), \
                r.stderr.decode("utf-8", "replace")

        #  No session: said so.
        empty = tempfile.mkdtemp(prefix="ecce-reconnect-nosession-")
        try:
            rc, out, err = run(urls["one"], env={
                "ECCE_SESSION_ID": "0123456789abcdef",
                "ECCE_REALUSERHOME": empty})
        finally:
            shutil.rmtree(empty, ignore_errors=True)
        check(rc == 2 and "ECCE is not running: start ECCE first" in err,
              "without a session: exit 2 and the message (rc=%s %r)" % (rc, err))
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

        #  Finish one calculation so that it is no longer reconnectable.
        term = os.path.join(build, "ecce-terminate")
        end = time.time() + 120
        while time.time() < end:
            r = subprocess.run([term, urls["done"]], env=s.env(),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=120)
            if r.returncode != 2:
                break
            time.sleep(3)
        st = s.waitState(urls["done"], seconds=240)
        check(st == "killed", "done is finished (%s)" % st)

        path_one = urls["one"][len("file://"):]
        bogus = os.path.join(s.state, "run", "no-such-calculation")
        rc, out, err = run(path_one, urls["done"], urls["idle"],
                           "file://" + bogus)
        say(out)
        say(err)
        lines = out.splitlines()
        check(rc == 1, "exit status 1 as some were refused (%s)" % rc)
        check(any(re.match(r"^one: reconnected, job id \S+$", l) for l in lines),
              "a running calculation: 'one: reconnected, job id N'")
        check(any(l.startswith("done: not reconnected: ") and
                  "already finished" in l for l in lines),
              "a finished calculation is refused with the reason")
        check(any(l.startswith("idle: not reconnected: ") and
                  "not launched" in l for l in lines),
              "a Ready calculation is refused with the reason")
        check(any("not reconnected: not a calculation in this session" in l
                  for l in lines), "an unknown path is not a calculation")
        check(s.state_of(urls["one"]) in ("submitted", "running"),
              "the reconnected calculation is still running")
        rc, out, err = run(path_one)
        say(out)
        check(rc == 0 and re.search(r"^one: reconnected, job id \S+$", out, re.M),
              "reconnecting alone: exit 0 (rc=%s)" % rc)
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
