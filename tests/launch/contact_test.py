#!/usr/bin/env python3
"""Out of contact (#284, #208): silence from the job monitor never ends a job.

A real ECCE-QM water GeoVib runs on localhost through Launch, gensub,
eccejobmaster, eccejobstore and eccejobmonitor, as in ecceqm_test.py, with
the out-of-contact time shortened (ECCE_JOB_CONTACT_SECONDS).  The test
listens on ecce_url_progress as the Organizer does.

  a. the monitor is stopped (SIGSTOP), and the job with it: the job store
     sends "contact lost" and stores it; the state stays running; Check Job
     on Machine finds the job there; after SIGCONT the store sends "contact
     ok", clears it, and the run ends completed;
  b. the monitor is stopped and ecce-qm killed: the state stays running
     while nothing is heard; Check Job on Machine reports the job gone and
     its script ended; only when the monitor reads on does the state become
     final, and it is not a system failure;
  c. the monitor is stopped past the reconnect time
     (ECCE_JOB_RECONNECT_SECONDS): eccejobmaster starts a fresh store and
     monitor, the state stays running throughout (never waiting or
     submitted), contact comes back, and the run ends completed.

    tests/launch/contact_test.py [--build build-cmake] [--keep] [--only a,b,c]

Exit status 77 (CTest SKIP) when a prerequisite is missing.
"""

import argparse
import glob
import os
import shutil
import signal
import subprocess
import sys
import time
import types

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))
sys.path.insert(0, os.path.join(REPO, "tests", "qm"))
sys.path.insert(0, HERE)

import isolate        # noqa: E402
import run_tests as launch   # noqa: E402
import ecceqm_test as qm     # noqa: E402

say = launch.say
check = qm.check
CONTACT = 20     # longer than the local heartbeat (about 13 s)
RECONNECT = 40


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
        if needle in cmd:
            found.append(int(entry))
    return found


def cwdPids(directory, needle):
    found = []
    for pid in procs(needle):
        try:
            if os.readlink("/proc/%d/cwd" % pid).startswith(directory):
                found.append(pid)
        except OSError:
            pass
    return found


def signalAll(pids, sig):
    for pid in pids:
        try:
            os.kill(pid, sig)
        except OSError:
            pass


class Run(object):
    """One calculation, launched, with a listener on its messages."""

    def __init__(self, suite, state, work, deck, output, label, env):
        self.s = suite
        self.stateDir = state
        self.label = label
        self.url = None
        self.rundir = None
        self.listenFile = os.path.join(state, "listen-%s.tsv" % label)
        name = "contact-%s-%d" % (label, int(time.time()))
        rc, out = suite.driver("create", suite.userUrl(), name, qm.RESOURCE, deck,
                               "ecceqm.qmin", "localhost",
                               os.path.join(state, "jobs"), suite.user())
        if not check(rc == 0, "%s: calculation created" % label):
            say(out)
            return
        self.url = out.strip().splitlines()[-1]
        rc, out = suite.driver("setup", self.url, work, os.path.basename(output))
        if not check(rc == 0, "%s: molecule, basis and theory stored" % label):
            say(out)
            self.url = None
            return
        self.listener = subprocess.Popen(
            [os.path.join(suite.home, "bin", "launchjob"), "-pipe", os.devnull,
             "listen", "ecce_url_progress", "900", self.listenFile],
            env=suite.env(), cwd=os.path.join(suite.home, "bin"),
            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        for _ in range(50):
            if os.path.exists(self.listenFile) and "# listening" in open(self.listenFile).read():
                break
            time.sleep(0.2)
        rc, out = suite.driver("launch", self.url, extra=env)
        ran = [l.split(":", 1)[1].strip() for l in out.splitlines()
               if l.startswith("run directory:")]
        self.rundir = ran[-1] if ran else None
        if not check(rc == 0 and self.rundir, "%s: launched (run directory %s)"
                     % (label, self.rundir)):
            say(out[-800:])
            self.url = None

    def messages(self):
        """[(arrival, progress text, contact)] for this calculation."""
        out = []
        if not os.path.exists(self.listenFile):
            return out
        for line in open(self.listenFile).read().splitlines():
            if line.startswith("#"):
                continue
            parts = line.split("\t")
            if len(parts) < 3 or parts[1].rstrip("/") != self.url.rstrip("/"):
                continue
            contact = ""
            if len(parts) > 3 and parts[3].startswith("contact="):
                contact = parts[3][len("contact="):]
            out.append((float(parts[0]), parts[2], contact))
        return out

    def waitFor(self, test, seconds):
        deadline = time.time() + seconds
        while time.time() < deadline:
            if test():
                return True
            time.sleep(0.5)
        return False

    def contacts(self):
        return [c for _, _, c in self.messages() if c]

    def state(self):
        rc, out = self.s.driver("state", self.url)
        return out.strip().splitlines()[-1] if out.strip() else ""

    def stored(self, what):
        rc, out = self.s.driver(what, self.url)
        return out.strip()

    def monitors(self):
        return cwdPids(self.rundir, "eccejobmonitor")

    def engine(self):
        return cwdPids(self.rundir, "ecce-qm")

    def checkJob(self):
        rc, out = self.s.driver("checkjob", self.url)
        say("    | " + "\n    | ".join(out.strip().splitlines()))
        fields = {}
        for line in out.splitlines():
            if line.startswith("reached="):
                fields = dict(w.split("=", 1) for w in line.split())
        return fields, out

    def started(self):
        """The monitor runs and the job has sent its first progress."""
        return self.waitFor(lambda: self.monitors() and self.engine() and
                            any(p for _, p, c in self.messages() if p and not c), 120)

    def finish(self):
        state = self.s.waitState(self.url, seconds=240)
        self.listener.terminate()
        try:
            self.listener.wait(timeout=30)
        except subprocess.TimeoutExpired:
            self.listener.kill()
        return state

    def storeLog(self):
        text = ""
        for log in sorted(glob.glob(os.path.join(self.stateDir, "tmp", "*", "jobs",
                                                 "*", "eccejob*.log*"))):
            if os.path.basename(os.path.dirname(log)).startswith(
                    os.path.basename(self.url.rstrip("/"))):
                with open(log, errors="replace") as handle:
                    text += "==== %s\n%s" % (log, handle.read())
        return text


def watchStates(run, seconds, until=None):
    """The states seen, polled about twice a second."""
    seen = []
    deadline = time.time() + seconds
    while time.time() < deadline:
        st = run.state()
        if not seen or seen[-1] != st:
            seen.append(st)
        if until and until():
            break
        time.sleep(0.5)
    return seen


def caseStopped(run):
    """a. stopped monitor (and job), then resumed."""
    if not check(run.started(), "a: the monitor runs and progress arrived"):
        return
    mons, eng = run.monitors(), run.engine()
    stoppedAt = time.time()
    signalAll(mons, signal.SIGSTOP)
    signalAll(eng, signal.SIGSTOP)
    say("    stopped monitor %s and ecce-qm %s" % (mons, eng))
    seen = watchStates(run, CONTACT + 25, until=lambda: "lost" in " ".join(run.contacts()))
    lost = [c for c in run.contacts() if c.startswith("lost ")]
    check(lost, "a: the Organizer's topic carried 'contact lost' (%s)" % run.contacts())
    if lost:
        heard = float(lost[-1].split()[1])
        check(stoppedAt - CONTACT - 5 <= heard <= stoppedAt + 2,
              "a: last heard %.0f s before the stop" % (stoppedAt - heard))
    check(run.stored("contact").startswith("lost "),
          "a: stored ecce:contact = %r" % run.stored("contact"))
    check(set(seen) <= {"running"}, "a: the state stayed running while silent %s" % seen)
    fields, out = run.checkJob()
    check(fields.get("reached") == "1" and fields.get("present") == "1",
          "a: Check Job on Machine finds the job there")
    check("Output ecceqm.qmout last written" in out,
          "a: and says when the output was last written")
    signalAll(eng, signal.SIGCONT)
    signalAll(mons, signal.SIGCONT)
    check(run.waitFor(lambda: "ok" in run.contacts(), 30),
          "a: 'contact ok' after SIGCONT (%s)" % run.contacts())
    state = run.finish()
    check(state == "completed", "a: the run ended completed (last: %s)" % state)
    check(run.stored("contact") == "", "a: the stored contact state was cleared")
    check("heartbeat" not in run.stored("reason").splitlines()[0]
          if run.stored("reason") else True, "a: no lost-monitor reason")


def caseKilled(run):
    """b. stopped monitor, killed engine: gone, final only from the output."""
    if not check(run.started(), "b: the monitor runs and progress arrived"):
        return
    mons, eng = run.monitors(), run.engine()
    signalAll(mons, signal.SIGSTOP)
    signalAll(eng, signal.SIGKILL)
    say("    stopped monitor %s, killed ecce-qm %s" % (mons, eng))
    status = os.path.join(run.rundir, ".ecce.status")
    check(run.waitFor(lambda: os.path.exists(status), 60),
          "b: the job script ended (wrote .ecce.status)")
    seen = watchStates(run, CONTACT + 25, until=lambda: "lost" in " ".join(run.contacts()))
    check(any(c.startswith("lost ") for c in run.contacts()),
          "b: 'contact lost' sent (%s)" % run.contacts())
    seen += watchStates(run, 5)
    check(set(seen) <= {"running"},
          "b: no final state while only silence is known %s" % seen)
    fields, out = run.checkJob()
    check(fields.get("reached") == "1" and fields.get("present") == "0",
          "b: Check Job on Machine reports the job gone")
    check(fields.get("status", "-") != "-", "b: and the job script's status (%s)"
          % fields.get("status"))
    check("no longer on the machine" in out and "Reconnect reads the output" in out,
          "b: in plain sentences")
    check(run.state() == "running", "b: the check itself changed no state")
    signalAll(mons, signal.SIGCONT)
    state = run.finish()
    check(state in ("unsuccessful", "failed"),
          "b: the monitor read the end: %s, not system_failure" % state)


def caseReconnect(run):
    """c. silence past the reconnect time: a fresh store, still running."""
    if not check(run.started(), "c: the monitor runs and progress arrived"):
        return
    mons, eng = run.monitors(), run.engine()
    signalAll(mons, signal.SIGSTOP)
    signalAll(eng, signal.SIGSTOP)
    say("    stopped monitor %s and ecce-qm %s" % (mons, eng))
    fresh = lambda: [p for p in run.monitors() if p not in mons]
    seen = watchStates(run, RECONNECT + 60, until=lambda: bool(fresh()))
    check(fresh(), "c: a fresh monitor was started (%s)" % fresh())
    seen += watchStates(run, 4)
    check(set(seen) <= {"running"},
          "c: the state stayed running through the reconnect %s" % seen)
    check(run.waitFor(lambda: "ok" in run.contacts(), 60),
          "c: contact came back with the fresh monitor (%s)" % run.contacts())
    log = run.storeLog()
    check("reconnecting; the run state is left as it is" in log,
          "c: the first store handed back without parking the job")
    check("restart count is 1" in log, "c: eccejobmaster started it again")
    signalAll(eng, signal.SIGCONT)
    signalAll(mons, signal.SIGCONT)     # the old monitor's pipe is gone: it ends
    state = run.finish()
    check(state == "completed", "c: the run ended completed (last: %s)" % state)
    check(run.stored("contact") == "", "c: the stored contact state was cleared")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    parser.add_argument("--keep", action="store_true")
    parser.add_argument("--only", default="a,b,c")
    opts = parser.parse_args()
    build = os.path.abspath(opts.build)

    qmExe = os.path.join(build, "src", "qm", "ecce-qm")
    for exe in ("launchjob", "eccejobstore", "eccejobmaster"):
        if not os.access(os.path.join(build, exe), os.X_OK):
            launch.skip("%s is not built in %s" % (exe, build))
    if not os.access(qmExe, os.X_OK):
        launch.skip("ecce-qm is not built (cmake -DECCE_BUILD_QM=ON)")
    for tool in ("mosquitto", "perl"):
        if not shutil.which(tool):
            launch.skip("%s is not installed" % tool)
    if not os.path.exists(os.path.join(build, "siteconfig-local", "DataServers")):
        launch.skip("%s/siteconfig-local is missing (run cmake)" % build)

    state = isolate.resolveStateDir(isolate.runState("contact", keep=opts.keep))
    os.makedirs(state, exist_ok=True)
    launch.sweep(state)
    for sub in ("jobs", "tmp", "localdata", "work"):
        shutil.rmtree(os.path.join(state, sub), ignore_errors=True)
    os.makedirs(os.path.join(state, "tmp"))
    os.makedirs(os.path.join(state, "work"))
    os.makedirs(os.path.join(state, "jobs"))
    os.environ["ECCE_TMPDIR"] = os.path.join(state, "tmp")
    install = launch.treeInstall(state, build)
    launch.link(qmExe, os.path.join(install, "bin", "ecce-qm"))
    settings = isolate.apply(install, state)
    home = settings["ECCE_HOME"]
    os.unlink(os.path.join(home, "siteconfig", "DataServers"))
    os.environ["ECCE_TEST_HOME"] = install
    say(isolate.describe(settings))

    args = types.SimpleNamespace(
        local=True, folder=False, machine="localhost", cwd=None,
        legacy_transport=None, job_comms=None, shared_connection=False,
        hold=False, drop=False, remote_user="", expect_keepalive=False)
    suite = launch.Suite(args, build, state, home)
    for name in ("eccejobmaster", "eccejobstore", "launchjob"):
        path = os.path.realpath(os.path.join(home, "bin", name))
        check(path == os.path.join(build, name), "%s -> %s" % (name, path))
    check(os.path.realpath(os.path.join(home, "bin", "ecce-qm")) == os.path.realpath(qmExe),
          "ecce-qm is the one in the build tree")

    env = suite.env()
    env["ECCE_HOME"] = REPO
    cases = {
        "a": (caseStopped, {"ECCE_JOB_CONTACT_SECONDS": str(CONTACT)}),
        "b": (caseKilled, {"ECCE_JOB_CONTACT_SECONDS": str(CONTACT)}),
        "c": (caseReconnect, {"ECCE_JOB_CONTACT_SECONDS": str(CONTACT),
                              "ECCE_JOB_RECONNECT_SECONDS": str(RECONNECT)}),
    }
    try:
        if not suite.services(True):
            check(False, "services started")
        else:
            work = os.path.join(state, "work", "water")
            os.makedirs(work)
            qm.writeCalcFiles(work, "water", "h2o", 1, "GeoVib")
            deck = qm.generateDeck(env, work, "water")
            output = qm.importFromOutput(env, work, "water", qmExe, deck)
            for key in opts.only.split(","):
                fn, extra = cases[key]
                say("--- " + fn.__doc__.strip())
                run = Run(suite, state, work, deck, output, key, extra)
                if run.url:
                    fn(run)
                    if any(f.startswith(key + ":") for f in qm.failures):
                        say(run.storeLog()[-4000:])
    finally:
        if not opts.keep:
            suite.services(False)
            note, n = launch.sweep(state)
            left = launch.procsUnder(state)
            say("cleanup: %d extra process(es) stopped, %d left" % (n, len(left)))
            if left:
                qm.failures.append("processes left running: %r" % left)

    say("")
    if qm.failures:
        say("FAILED (%d): %s" % (len(qm.failures), "; ".join(qm.failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
