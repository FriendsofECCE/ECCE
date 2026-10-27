#!/usr/bin/env python3
"""Reconnect while the old job store is still alive (#601).

Run Mgmt > Reconnect (single and Selected) publishes ecce_ejs_kill and starts
a new job store.  The old store must stop without touching the run directory,
and the Organizer must wait until it has gone before starting the new one:

  * a store whose run directory lost eccejobmonitor.conf ends Waiting, never
    Failed (a monitor FATAL before any job state is not the job's failure);
  * old stores that take 3 s to exit (ECCE_TEST_EJS_KILL_DELAY_MS): both
    reconnect paths wait, the new monitor starts with its config present and
    the jobs complete;
  * an old store that takes 20 s: both paths report "previous monitor still
    running" after 15 s and start nothing; a second Reconnect then works.

    tests/launch/reconnect_race_test.py [--build DIR] [--keep]

Exit status 77 (CTest SKIP) without Xvfb, mopac or the built binaries.
"""

import argparse
import glob
import os
import shutil
import signal
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402

DECK = os.path.join(REPO, "tests", "e2e", "fixtures", "mopac", "mos", "mopac.mop")
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def procs(needle):
    found = []
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        try:
            with open("/proc/%s/cmdline" % entry, "rb") as handle:
                cmd = handle.read().replace(b"\0", b" ").decode("utf-8", "replace")
        except OSError:
            continue
        if all(n in cmd for n in needle):
            found.append(int(entry))
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ("mopac",))
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "reconnrace", {}, local=True, keep=args.keep)
    wrapper = os.path.join(s.state, "held-mopac")
    with open(wrapper, "w") as h:
        h.write("#!/bin/sh\n"
                "n=0\n"
                "while [ ! -e release ] && [ $n -lt 900 ]; do sleep 1; n=$((n+1)); done\n"
                "exec %s \"$@\"\n" % shutil.which("mopac"))
    os.chmod(wrapper, 0o755)
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
    s.registerMachine("localhost", "Shell", {"MOPAC": wrapper})
    jobs = os.path.join(s.state, "jobs")
    os.makedirs(jobs, exist_ok=True)
    shutil.rmtree(os.path.join(s.state, ".ECCE", "waiting"), ignore_errors=True)

    #  No "0" in the names: see waiting_test.py.
    stamp = int(str(int(time.time()) % 100000).replace("0", "9"))
    display = org = None

    def logtext(name):
        logs = glob.glob(os.path.join(s.state, "tmp", "*", "jobs",
                                      name + "__*", "eccejob*.log*"))
        return "".join(open(l, errors="replace").read() for l in logs)

    def launch(name, delay):
        #  The job store reads the hook when ecce_ejs_kill arrives.
        if delay:
            os.environ["ECCE_TEST_EJS_KILL_DELAY_MS"] = str(delay)
        else:
            os.environ.pop("ECCE_TEST_EJS_KILL_DELAY_MS", None)
        url, out = s.create(name, "mopac_es", DECK, "mopac.mop", jobs)
        if not check(url is not None, "%s created" % name):
            say(out)
            return None, ""
        rc, out = s.launch(url)
        if not check(rc == 0, "%s launched" % name):
            say(out[-600:])
            return None, ""
        ran = [l.split(":", 1)[1].strip() for l in out.splitlines()
               if l.startswith("run directory:")]
        rundir = ran[-1] if ran else ""
        for _ in range(120):
            if "job monitor running on" in logtext(name):
                break
            time.sleep(1)
        else:
            check(False, "%s is monitored" % name)
        return url, rundir

    def finish(rundir):
        open(os.path.join(rundir, "release"), "w").close()

    try:
        if not s.services(True):
            check(False, "services came up")
            return 1

        #  1. A monitor without its configuration parks the job.
        nm = "race-noconf-%d" % stamp
        url, rundir = launch(nm, 0)
        if not url:
            return 1
        os.unlink(os.path.join(rundir, "eccejobmonitor.conf"))
        for pid in procs(["eccejobmonitor", "-configFile"]):
            try:
                if os.readlink("/proc/%d/cwd" % pid) == os.path.realpath(rundir):
                    os.kill(pid, signal.SIGKILL)
            except OSError:
                pass
        st = s.waitState(url, seconds=180, want=("waiting", "failed"))
        check(st == "waiting", "no monitor configuration: Waiting, not Failed (%s)" % st)
        for _ in range(120):
            if "stays waiting" in logtext(nm):
                break
            time.sleep(1)
        check("stays waiting" in logtext(nm),
              "eccejobmaster logged that the job stays waiting")
        finish(rundir)

        #  2. Old stores that need 3 s to exit: both paths wait.
        a, ra = launch("race-single-%d" % stamp, 3000)
        b, rb = launch("race-batch-%d" % stamp, 3000)
        #  3. Old stores that need 20 s: both paths give up after 15 s.
        c, rc_ = launch("race-late-batch-%d" % stamp, 20000)
        d, rd = launch("race-late-single-%d" % stamp, 20000)
        if not all((a, b, c, d)):
            return 1

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        import sessionkey
        sid = genv.get("ECCE_SESSION_ID", "")
        with open(os.path.join(s.state, ".ECCE", "waiting", "session"), "w") as h:
            h.write(sessionkey.key(sid, genv) + "\n")
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

        def stores(name):
            return procs(["eccejobstore", name])

        #  Delayed exit, single path.
        old = stores("race-single")
        check(len(old) == 1, "one old store for the single job (%s)" % old)
        t0 = time.time()
        ans = send("reconnectjob %s" % a)
        took = time.time() - t0
        say("  single: %s (%.1f s)" % (ans, took))
        check(ans.startswith("reconnected"), "single Reconnect succeeded")
        check(took >= 2.5, "single Reconnect waited for the old store (%.1f s)" % took)
        check(not (set(old) & set(stores("race-single"))), "the old store is gone")
        check(os.path.exists(os.path.join(ra, "eccejobmonitor.conf")),
              "the run directory kept eccejobmonitor.conf")

        #  Delayed exit, batch path.
        t0 = time.time()
        ans = send("reconnectselected %s" % b)
        took = time.time() - t0
        say("  batch: %s (%.1f s)" % (ans, took))
        check("race-batch-%d: reconnected" % stamp in ans, "batch Reconnect succeeded")
        check(took >= 2.5, "batch Reconnect waited for the old store (%.1f s)" % took)
        check(os.path.exists(os.path.join(rb, "eccejobmonitor.conf")),
              "the run directory kept eccejobmonitor.conf")
        for r in (ra, rb):
            finish(r)
        for u in (a, b):
            st = s.waitState(u, seconds=180)
            check(st == "completed", "completed after the reconnect (%s)" % st)
            check("TE" in s.props(u), "TE stored")

        #  Exit later than the wait: nothing is started.
        for what, u, nm, cmd in (("batch", c, "race-late-batch", "reconnectselected"),
                                 ("single", d, "race-late-single", "reconnectjob")):
            old = stores(nm)
            ans = send("%s %s" % (cmd, u))
            say("  late %s: %s" % (what, ans))
            check("previous monitor still running" in ans,
                  "late %s: reported 'previous monitor still running'" % what)
            check(stores(nm) == old, "late %s: no second store started (%s)" %
                  (what, stores(nm)))
        for _ in range(60):
            if not stores("race-late"):
                break
            time.sleep(1)
        #  The old stores have exited by now; a new Reconnect works.
        for r in (rc_, rd):
            finish(r)
        ans = send("reconnectselected %s" % c)
        check("race-late-batch-%d: reconnected" % stamp in ans,
              "late batch: the next Reconnect works")
        ans = send("reconnectjob %s" % d)
        check(ans.startswith("reconnected"), "late single: the next Reconnect works")
        for u in (c, d):
            st = s.waitState(u, seconds=180)
            check(st == "completed", "completed after the second Reconnect (%s)" % st)
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
