#!/usr/bin/env python3
"""A job whose monitoring stops is "waiting for login", and is caught up (#208).

When the client goes away (SIGTERM at shutdown or logout) or the monitor's
restart budget runs out, eccejobstore must not fail the calculation: it
parks it as "waiting", recorded in ~/.ECCE/waiting.  The next session start
(`launchjob catchup`, what the Organizer runs) reconnects it, so the job
ends as it really ended.  Runs on localhost/Shell with a MOPAC stand-in
that waits for a `release` file in its run directory:

  a. the job finishes while the client is away -> waiting -> completed, with
     its properties
  b. the job still runs at return               -> monitoring resumes, then
     completed
  c. the job dies while away                    -> a failure with the
     reason that it vanished, not killed and not a lost monitor
  d. the restart budget runs out (the monitor is killed twice, with
     ECCE_JOB_MAXCONNECTS=2)                    -> waiting, not system
     failure; then completed after catch-up
  e. as (a) with the client's own home apart from the data server's, as
     a -remote client has it: the record is the client's, the server's
     state is untouched

    tests/launch/waiting_test.py [--build build] [--keep] [--only a,b,...]

Exit status 77 (CTest SKIP) when a prerequisite is missing.
"""

import argparse
import glob
import os
import shutil
import signal
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import harness  # noqa: E402
from harness import say  # noqa: E402

DECK = os.path.join(REPO, "tests", "e2e", "fixtures", "mopac", "mos", "mopac.mop")
REQUIRED_PROPS = ("TE", "GEOMTRACE")


def procs(needle):
    """PIDs whose command line contains every string of `needle`."""
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


def cwdPids(directory, needle):
    """PIDs running `needle` with their working directory in `directory`."""
    found = []
    for pid in procs([needle]):
        try:
            if os.readlink("/proc/%d/cwd" % pid).startswith(directory):
                found.append(pid)
        except OSError:
            pass
    return found


class Case(object):
    def __init__(self, s, label, home, stamp, env=None):
        self.s = s
        self.label = label
        self.home = home          # the client's ECCE_REALUSERHOME
        self.env = dict(env or {})
        self.env["ECCE_REALUSERHOME"] = home
        self.name = "wait-%s-%d" % (label, stamp)
        self.url = None
        self.rundir = None
        self.jobid = None

    def say(self, text):
        say("  [%s] %s" % (self.label, text))

    def check(self, ok, what):
        return self.s.check(ok, "%s: %s" % (self.label, what))

    def drive(self, *argv, extra=None, timeout=180):
        env = dict(self.env)
        env.update(extra or {})
        pipe = self.s.authFile()
        return self.s.run([os.path.join(self.s.home, "bin", "launchjob"),
                           "-pipe", pipe] + list(argv), extra=env,
                          cwd=os.path.join(self.s.home, "bin"), timeout=timeout)

    def state(self):
        rc, out = self.drive("state", self.url)
        return out.strip().splitlines()[-1] if out.strip() else ""

    def waitState(self, want, seconds=180):
        state = ""
        deadline = time.time() + seconds
        while time.time() < deadline:
            state = self.state()
            if state in want:
                break
            time.sleep(1)
        return state

    def reason(self):
        rc, out = self.drive("reason", self.url)
        return out

    def start(self, launchEnv=None):
        rc, out = self.drive("create", self.s.userUrl(), self.name, "mopac_es",
                             DECK, "mopac.mop", "localhost",
                             os.path.join(self.s.state, "jobs"), self.s.user())
        if not self.check(rc == 0, "calculation created"):
            say(out)
            return False
        self.url = out.strip().splitlines()[-1]
        rc, out = self.drive("launch", self.url, extra=launchEnv)
        if not self.check(rc == 0, "Launch ran to the end"):
            say(out[-800:])
            return False
        ran = [l.split(":", 1)[1].strip() for l in out.splitlines()
               if l.startswith("run directory:")]
        self.rundir = ran[-1] if ran else None
        rc, out = self.drive("jobid", self.url)
        jobid = out.strip().splitlines()[-1] if out.strip() else ""
        self.jobid = int(jobid) if jobid.isdigit() else None
        return self.check(self.rundir and self.jobid, "run directory %s, job %s"
                          % (self.rundir, self.jobid))

    def storeLogs(self):
        pattern = os.path.join(self.s.state, "tmp", "*", "jobs",
                               self.name + "__*", "eccejob*.log*")
        text = ""
        for log in sorted(glob.glob(pattern)):
            try:
                with open(log, errors="replace") as handle:
                    text += "==== %s\n%s" % (log, handle.read())
            except OSError:
                pass
        return text

    def _master(self):
        for log in glob.glob(os.path.join(self.s.state, "tmp", "*", "jobs",
                                          self.name + "__*",
                                          "eccejobmaster.log")):
            with open(log, errors="replace") as handle:
                return handle.read()
        return ""

    def monitoring(self, count=1, seconds=120):
        """Wait until `count` job store runs have seen the monitor alive."""
        for _ in range(seconds):
            if self.storeLogs().count("job monitor running on") >= count:
                return True
            time.sleep(1)
        return False

    def stores(self):
        return procs(["eccejobstore", self.name]) + procs(["eccejobmaster", self.url])

    def goAway(self):
        """What shutdown or logout does: SIGTERM to the job store and master."""
        stores = procs(["eccejobstore", self.name])
        masters = procs(["eccejobmaster", self.url])
        self.check(stores and masters, "job store %s and master %s running"
                   % (stores, masters))
        for pid in stores + masters:
            try:
                os.kill(pid, signal.SIGTERM)
            except OSError:
                pass
        for _ in range(60):
            if not self.stores():
                break
            time.sleep(0.5)
        self.check(not self.stores(), "job store and master exited")

    def release(self):
        open(os.path.join(self.rundir, "release"), "w").close()

    def finished(self, seconds=120):
        """The job itself ended (gensub writes its status file at the end)."""
        status = os.path.join(self.rundir, ".ecce.status")
        for _ in range(seconds):
            if os.path.exists(status) and not cwdPids(self.rundir, "mopac"):
                return True
            time.sleep(1)
        return False

    def recorded(self):
        found = []
        for path in glob.glob(os.path.join(self.home, ".ECCE", "waiting", "*.job")):
            with open(path) as handle:
                found.append(handle.read().strip())
        return self.url in found

    def parked(self):
        state = self.waitState(("waiting",), 60)
        self.check(state == "waiting", "state is waiting (last: %s)" % state)
        self.check(self.recorded(), "recorded in %s/.ECCE/waiting" % self.home)
        self.check("Monitoring stopped" in self.reason(),
                   "the reason says monitoring stopped")

    def catchUp(self):
        rc, out = self.drive("catchup")
        lines = [l for l in out.splitlines() if self.url in l]
        self.say("catchup: " + (lines[-1] if lines else out.strip()[-300:]))
        ok = self.check(rc == 0 and lines and lines[-1].startswith("ok "),
                        "catch-up reconnected the job")
        self.check(not self.recorded(), "no longer recorded as waiting")
        return ok

    def completed(self):
        state = self.waitState(harness.FINAL_STATES + ("system_failure",), 180)
        if not self.check(state == "completed",
                          "state is completed (last: %s)" % state):
            say(self.storeLogs()[-3000:])
            return
        rc, out = self.drive("props", self.url)
        props = out.split()
        for prop in REQUIRED_PROPS:
            self.check(prop in props, "%s stored" % prop)
        self.check("Monitoring stopped" not in self.reason().splitlines()[0],
                   "the waiting reason was cleared")


def finishWhileAway(c):
    if not c.start():
        return
    c.check(c.monitoring(), "the monitor is watching the job")
    c.goAway()
    c.parked()
    c.release()
    c.check(c.finished(), "the job finished while the client was away")
    c.check(c.state() == "waiting", "still waiting before catch-up")
    if c.catchUp():
        c.completed()


def runningAtReturn(c):
    if not c.start():
        return
    c.check(c.monitoring(), "the monitor is watching the job")
    c.goAway()
    c.parked()
    if c.catchUp():
        c.check(c.monitoring(2), "a new monitor watches the running job")
        state = c.state()
        c.check(state in ("submitted", "running"),
                "monitoring resumed (state %s)" % state)
        c.check(procs(["eccejobmaster", c.url]), "a job master runs again")
        c.release()
        c.completed()


def diedWhileAway(c):
    if not c.start():
        return
    c.check(c.monitoring(), "the monitor is watching the job")
    c.goAway()
    c.parked()
    try:
        os.killpg(os.getpgid(c.jobid), signal.SIGKILL)
    except OSError as err:
        c.check(False, "could not kill the job's process group (%s)" % err)
    for _ in range(30):
        if not cwdPids(c.rundir, "mopac"):
            break
        time.sleep(1)
    if c.catchUp():
        state = c.waitState(harness.FINAL_STATES + ("system_failure",), 180)
        c.check(state == "system_failure",
                "the vanished job is a failure, not killed (last: %s)" % state)
        text = c.reason()
        c.check("not cancelled by the user" in text,
                "the reason says the job vanished: %s"
                % text.splitlines()[0][:120] if text else "")
        c.check("heartbeat" not in text.splitlines()[0] if text else True,
                "the reason is not the lost monitor")


def budgetRunsOut(c):
    env = {"ECCE_JOB_MAXCONNECTS": "2", "ECCE_JOB_RESTARTRESET": "100000",
           "ECCE_JOB_MAXQUICKTIME": "1"}
    if not c.start(env):
        return
    killed = 0
    for round in (1, 2):
        if not c.check(c.monitoring(round), "monitor run %d is watching" % round):
            break
        time.sleep(3)
        pids = cwdPids(c.rundir, "eccejobmonitor")
        for pid in pids:
            os.kill(pid, signal.SIGKILL)
        killed += bool(pids)
    c.check(killed == 2, "the monitor was killed twice (%d)" % killed)
    for _ in range(60):
        if "exited with final status" in c._master():
            break
        time.sleep(1)
    log = c._master()
    c.check("exited with final status" in log, "eccejobmaster gave up")
    c.check("Restart budget used up" in log,
            "eccejobmaster says the calculation stays waiting")
    state = c.state()
    c.check(state == "waiting",
            "the used-up budget left it waiting, not system failure (%s)" % state)
    c.check(c.recorded(), "recorded as waiting")
    c.release()
    c.check(c.finished(), "the job finished meanwhile")
    if c.catchUp():
        c.completed()


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--build", default=os.path.join(REPO, "build"))
    parser.add_argument("--keep", action="store_true")
    parser.add_argument("--only", default="a,b,c,d,e")
    args = parser.parse_args()
    build = os.path.abspath(args.build)
    harness.prerequisites(build, ("mopac",))

    s = harness.Session(build, "waiting", {}, (8699, 8691), keep=args.keep)
    wrapper = os.path.join(s.state, "held-mopac")
    with open(wrapper, "w") as h:
        h.write("#!/bin/sh\n"
                "#  Runs only once the test drops a release file here.\n"
                "n=0\n"
                "while [ ! -e release ] && [ $n -lt 600 ]; do sleep 1; n=$((n+1)); done\n"
                "exec %s \"$@\"\n" % shutil.which("mopac"))
    os.chmod(wrapper, 0o755)
    with open(os.path.join(s.state, ".ECCE", "CONFIG.localhost"), "w") as h:
        h.write("MOPAC: %s\n" % wrapper)
    os.makedirs(os.path.join(s.state, "jobs"), exist_ok=True)
    shutil.rmtree(os.path.join(s.state, ".ECCE", "waiting"), ignore_errors=True)

    #  (e): the client's home, apart from the data server's account.
    client = os.path.join(s.state, "client-home")
    shutil.rmtree(client, ignore_errors=True)
    os.makedirs(os.path.join(client, ".ECCE"))
    shutil.copy(os.path.join(s.state, ".ECCE", "CONFIG.localhost"),
                os.path.join(client, ".ECCE"))

    #  No "0" in the URLs: DavEDSI listed a collection as its own member
    #  unless its URL had one, and Reconnect then deleted Props itself.
    stamp = int(str(int(time.time()) % 100000).replace("0", "9"))
    cases = {
        "a": ("a. the job finishes while the client is away", finishWhileAway, s.state),
        "b": ("b. the job still runs at return", runningAtReturn, s.state),
        "c": ("c. the job dies while the client is away", diedWhileAway, s.state),
        "d": ("d. the restart budget runs out", budgetRunsOut, s.state),
        "e": ("e. as (a), the client's home apart from the server's",
              finishWhileAway, client),
    }
    try:
        if not s.services(True):
            s.check(False, "services started")
        else:
            #  The client reaches the broker through its own copy of the
            #  session's broker file, as ecce-remote-setup gives it one.
            for path in glob.glob(os.path.join(s.state, ".ECCE", "broker_*")):
                shutil.copy(path, os.path.join(client, ".ECCE"))
            for key in args.only.split(","):
                title, fn, home = cases[key]
                say("--- " + title)
                fn(Case(s, key, home, stamp))
            if "e" in args.only:
                server = glob.glob(os.path.join(s.state, ".ECCE", "waiting", "*.job"))
                s.check(not server, "e: nothing recorded in the server account's "
                        "state (%s)" % server)
    finally:
        left = s.stop()
        if left:
            s.failures.append("processes left running: %r" % left)

    say("")
    if s.failures:
        say("FAILED (%d): %s" % (len(s.failures), "; ".join(s.failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
