#!/usr/bin/env python3
"""Submit one real job through Launch and assert it reaches "completed".

The gap in #107 part A: nothing else in the tree runs Launch, gensub,
eccejobmaster, eccejobstore and eccejobmonitor together.  This starts an
isolated data server and broker (tests/apps/isolate.py), creates a MOPAC
calculation on it, launches it on the `localhost` machine under the Shell
queue manager with the tree's own `launchjob` driver, waits for the run
state on the data server to become "completed", and checks that the
properties reached the calculation's Props/ collection.

Everything under test comes from the build and source trees, never from an
installed /opt/ecce: the test assembles its own $ECCE_HOME of symlinks
(binaries from --build, scripts/data/packaging from the repository) and
prints where each one resolves.

    tests/launch/run_tests.py [--build build-native] [--keep] [-v]
    tests/launch/run_tests.py --nwchem-restart     # #202: relaunch a finished NWChem job
    tests/launch/run_tests.py --local              # #216: data in a local folder
    tests/launch/run_tests.py --folder             # calc in a plain folder (Save As)
    tests/launch/run_tests.py --machine sshtest --remote-user bashuser \
                              [--drop]

With --machine the job runs on a machine reached over ssh instead of on
localhost.  The test registers it (MyMachines, Queues, CONFIG.<machine>) for
the isolated user and leaves ~/.ssh alone, so the caller has to have made
`--machine` and its host resolvable by ssh non-interactively; see
tests/launch/remote_test.sh, which does that inside a container.  MOPAC
there is a wrapper that sleeps first, so the remote monitor can be observed
while it runs: its stdin must not be a tty, and no scp may be started over
libssh.  --local keeps the data in a folder (ECCE_LOCAL_DATA, #216): no
data server is started and the tree install has no siteconfig/DataServers.
--drop adds a run in which the monitor's sshd session is killed
mid-job; the job must still end completed, through eccejobstore's restart.

Exit status 77 (CTest SKIP) when a prerequisite is missing.
"""

import argparse
import os
import re
import shutil
import signal
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import isolate  # noqa: E402

SKIP = 77
#  One session for the run (#233). DISPLAY is unset: these jobs need no X
#  server, and the session must not depend on one.
SESSION_ID = os.urandom(8).hex()
WAIT_SECONDS = 120

#  Properties a MOPAC energy job must leave behind.  TE is the total
#  energy; GEOMTRACE is the geometry.
REQUIRED_PROPS = ("TE", "GEOMTRACE")

DECK = os.path.join(REPO, "tests", "e2e", "fixtures", "mopac", "mos", "mopac.mop")


def say(text):
    print(text, flush=True)


def skip(reason):
    say("SKIP: " + reason)
    sys.exit(SKIP)


# --- the tree's own $ECCE_HOME -------------------------------------------

def link(source, target):
    os.makedirs(os.path.dirname(target), exist_ok=True)
    if os.path.lexists(target):
        os.unlink(target)
    os.symlink(source, target)


def treeInstall(state, build):
    """An install directory made only of the build and source trees."""
    home = os.path.join(state, "tree-install")
    shutil.rmtree(home, ignore_errors=True)
    for exe in ("eccejobmaster", "eccejobstore", "ecmd", "launchjob"):
        link(os.path.join(build, exe), os.path.join(home, "bin", exe))
    for sub in ("gateway", "dataserver"):
        scripts = os.path.join(REPO, "packaging", sub)
        for name in os.listdir(scripts):
            path = os.path.join(scripts, name)
            if name.startswith("ecce-") and (name.endswith(".sh") or
                                             os.access(path, os.X_OK)):
                link(path, os.path.join(home, "bin", name))
    link(os.path.join(REPO, "packaging", "nwchem", "ecce-nwchem-datadir"),
         os.path.join(home, "bin", "ecce-nwchem-datadir"))
    link(os.path.join(REPO, "scripts"), os.path.join(home, "scripts"))
    link(os.path.join(REPO, "data", "admin"), os.path.join(home, "data", "admin"))
    link(os.path.join(REPO, "data", "client"), os.path.join(home, "data", "client"))
    link(os.path.join(REPO, "packaging", "dataserver", "httpd.conf.ecce"),
         os.path.join(home, "server", "httpd-conf", "httpd.conf.ecce"))

    #  siteconfig is generated at install time for these two files.
    site = os.path.join(home, "siteconfig")
    shutil.copytree(os.path.join(REPO, "siteconfig"), site)
    for name in ("DataServers", "site_runtime"):
        shutil.copy(os.path.join(build, "siteconfig-local", name),
                    os.path.join(site, name))
    return home


# --- processes ------------------------------------------------------------

def procsUnder(state):
    """PIDs whose command line or working directory lies in `state`."""
    needle = os.fsencode(state.rstrip("/"))
    found = []
    for entry in os.listdir("/proc"):
        if not entry.isdigit() or int(entry) == os.getpid():
            continue
        try:
            with open("/proc/%s/cmdline" % entry, "rb") as handle:
                cmd = handle.read()
            cwd = os.fsencode(os.readlink("/proc/%s/cwd" % entry))
        except OSError:
            continue
        if needle in cmd or cwd.startswith(needle + b"/") or cwd == needle:
            found.append(int(entry))
    return found


def sweep(state):
    """Stop everything still running out of the state directory."""
    note = isolate.killLeftovers(state)
    pids = procsUnder(state)
    for pid in pids:
        try:
            os.kill(pid, signal.SIGTERM)
        except OSError:
            pass
    time.sleep(0.5)
    for pid in procsUnder(state):
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
    return note, len(pids)


def seenBinaries(home):
    """{name: realpath} of ECCE binaries running with this $ECCE_HOME."""
    found = {}
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        try:
            with open("/proc/%s/environ" % entry, "rb") as handle:
                env = handle.read()
            exe = os.readlink("/proc/%s/exe" % entry)
        except OSError:
            continue
        if ("ECCE_HOME=%s\0" % home).encode() in env + b"\0":
            name = os.path.basename(exe)
            if name.startswith("ecce") or name in ("launchjob",):
                found[name] = exe
    return found


def monitorStdin():
    """Where a running eccejobmonitor's stdin points: "pipe:[..]" or /dev/pts/N."""
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        try:
            with open("/proc/%s/cmdline" % entry, "rb") as handle:
                argv = handle.read().split(b"\0")
            if b"-configFile" not in argv or not any(
                    a.endswith(b"eccejobmonitor") for a in argv):
                continue
            #  Only this run's: another run's monitors are not ours.
            jobs = os.path.join(os.path.dirname(
                os.environ.get("ECCE_TMPDIR", "/nonexistent/x")), "jobs")
            if not os.readlink("/proc/%s/cwd" % entry).startswith(jobs + "/"):
                continue
            return os.readlink("/proc/%s/fd/0" % entry)
        except OSError:
            continue
    return None


#  pids of the remote monitors; the script is sh so the login shell may be csh.
MONITOR_PIDS = "pgrep -f '^perl eccejobmonitor'; true"

REMOTE_MONITOR = ("for p in $(pgrep -f '^perl eccejobmonitor'); do "
                  "echo \"$p $(readlink /proc/$p/fd/0)\"; done")

#  Kills the sshd session that carries the monitor: the closest ancestor
#  of the monitor whose command name is sshd*, which is the user's own
#  process after privilege separation.
DROP_SESSION = (
    "p=$(pgrep -f '^perl eccejobmonitor' | head -1); "
    "while [ -n \"$p\" ] && [ \"$p\" != 1 ]; do "
    "c=$(cat /proc/$p/comm); "
    "case $c in sshd*) kill -9 $p; echo \"killed $p $c\"; exit 0;; esac; "
    "p=$(ps -o ppid= -p $p | tr -d ' '); done; echo none")

#  Stands in for MOPAC on the remote machine; the delay is read at run time.
MOPAC_WRAPPER = ("#!/bin/sh\nsleep $(cat \"$HOME/.mopac-delay\" 2>/dev/null "
                 "|| echo 0)\nexec /usr/bin/mopac \"$@\"\n")


# --- the run --------------------------------------------------------------

class Suite(object):
    def __init__(self, args, build, state, home):
        self.args = args
        self.build = build
        self.state = state
        self.home = home
        self.failures = []
        self.seen = {}

    def env(self, extra=None):
        env = dict(os.environ)
        env.update({
            "HOST": env.get("HOST") or os.uname().nodename,
            "ECCE_SESSION_ID": SESSION_ID,
            "ECCE_REALUSER": os.environ.get("USER") or subprocess.check_output(
                ["id", "-un"]).decode().strip(),
            "PATH": "%s/scripts:%s/scripts/parsers:%s" % (
                self.home, self.home, os.environ["PATH"]),
        })
        env.pop("ECCE_NO_REAP", None)
        env.pop("DISPLAY", None)
        env.pop("ECCE_LOCAL_DATA", None)
        if self.args.local:
            env["ECCE_LOCAL_DATA"] = self.localData()
        env.pop("ECCE_TRANSPORT", None)
        if self.remote():
            env["ECCE_RCOM_LOGMODE"] = "1"
        env.update(extra or {})
        if self.remote():
            env["PATH"] = self.stubDir() + ":" + env["PATH"]
        return env

    def stubDir(self):
        """scp and friends that only record that they were started."""
        stubs = os.path.join(self.state, "stubs")
        os.makedirs(stubs, exist_ok=True)
        if self.args.shared_connection and os.path.exists(os.path.join(stubs, "ssh")):
            os.unlink(os.path.join(stubs, "ssh"))   # left by an earlier run
        #  With a shared connection the OpenSSH client is what is under test.
        for name in (("scp", "sftp", "sshpass") if self.args.shared_connection
                     else ("scp", "sftp", "ssh", "sshpass")):
            path = os.path.join(stubs, name)
            with open(path, "w") as handle:
                handle.write("#!/bin/sh\necho \"%s $*\" >> %s\nexit 99\n"
                             % (name, self.stubLog()))
            os.chmod(path, 0o755)
        return stubs

    def stubLog(self):
        return os.path.join(self.state, "stubs.log")

    def remoteMonitorStdin(self):
        """Where the remote eccejobmonitor's stdin points, or None."""
        rc, out = self.sshRun(REMOTE_MONITOR)
        for line in out.splitlines():
            parts = line.split(None, 1)
            if len(parts) == 2:
                return parts[1]
        return None

    def setMopacDelay(self, seconds):
        self.sshRun("cat > ~/mopac-slow <<'EOF'\n%sEOF\nchmod +x ~/mopac-slow; "
                    "echo %d > ~/.mopac-delay" % (MOPAC_WRAPPER, seconds))

    def remote(self):
        return self.args.machine != "localhost"

    def sshRun(self, command):
        """Run the sh script `command` on the remote machine, outside ECCE.

        The script goes on stdin, since the login shell may be csh.
        """
        result = subprocess.run(
            ["ssh", "-o", "BatchMode=yes", "%s@%s" % (self.args.remote_user,
                                                      self.args.machine),
             "/bin/sh -s"], input=command.encode(), stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, timeout=60)
        return result.returncode, result.stdout.decode("utf-8", "replace")

    def check(self, ok, what):
        say("  %s %s" % ("ok  " if ok else "FAIL", what))
        if not ok:
            self.failures.append(what)
        return ok

    def run(self, argv, extra=None, timeout=180, cwd=None):
        result = subprocess.run(argv, env=self.env(extra), cwd=cwd,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=timeout)
        return result.returncode, result.stdout.decode("utf-8", "replace")

    def localData(self):
        return os.path.join(self.state, "localdata")

    def services(self, start):
        servers = ["ecce-dataserver-start", "ecce-gateway-start"]
        if self.args.local:
            servers.remove("ecce-dataserver-start")
        if start:
            for script in servers:
                rc, out = self.run([os.path.join(self.home, "bin", script)],
                                   timeout=240)
                say("  %s: rc=%d %s" % (script, rc, out.strip().replace("\n", " | ")[:300]))
                if rc != 0:
                    return False
            return True
        for script in ("ecce-gateway-stop", "ecce-dataserver-stop"):
            try:
                self.run([os.path.join(self.home, "bin", script)], timeout=120)
            except subprocess.TimeoutExpired:
                pass
        return True

    def driver(self, *argv, extra=None):
        """One launchjob invocation, with credentials piped in."""
        pipe = (os.devnull if self.args.local
                else self.authFile(os.path.join(self.state, "auth.pipe")))
        #  The gateway starts every app with `cd $ECCE_HOME/bin && ./app`,
        #  and eccejobmaster runs "./eccejobstore" relative to that, so the
        #  default mirrors it.  --cwd . shows what happens otherwise.
        return self.run([os.path.join(self.home, "bin", "launchjob"),
                         "-pipe", pipe] + list(argv), extra=extra,
                        cwd=self.args.cwd or os.path.join(self.home, "bin"))

    def user(self):
        return self.env()["ECCE_REALUSER"]

    def folder(self):
        return os.path.join(self.state, "folder")

    def userUrl(self):
        if self.args.folder:
            # A plain folder, as Builder > Save As to the Local Filesystem.
            os.makedirs(self.folder(), exist_ok=True)
            return "file://" + self.folder()
        if self.args.local:
            # Local mode's home is always users/local (#216).
            return "file://%s/users/local" % self.localData()
        import fixture
        return "%s/users/%s" % (fixture.base().rsplit("/users/", 1)[0], self.user())

    def authFile(self, path):
        """Credentials for the calling user's own account, as AuthCache reads them.

        The client asks for the password of ECCE_REALUSER, so the calculation
        has to live in that account rather than the fixture's.
        """
        import fixture
        root = os.path.join(fixture.stateDir(), "htdocs", "Ecce", "users", self.user())
        realm = fixture.realm(root)
        prefix = "http://localhost:%s/" % os.environ["ECCE_DATASERVER_PORT"]
        with open(path, "w") as handle:
            handle.write("2\n%s%s|%s|ecce\n%s|%s|ecce\n"
                         % (prefix, realm, self.user(), prefix, self.user()))
        os.chmod(path, 0o600)
        return path

    def monitorPids(self):
        rc, out = self.sshRun(MONITOR_PIDS)
        return [int(w) for w in out.split() if w.isdigit()]

    def one(self, label, drop=False, kills=0, env=None, giveup=False):
        """One job.  drop: kill the monitor's sshd session once (#205);
        kills: kill the remote monitor process that many times, as a login
        node would (#206); giveup: expect monitoring to be abandoned."""
        say("--- %s" % label)
        self.dropNote = ""
        if self.remote():
            left = self.monitorPids()
            self.check(not left, "no eccejobmonitor left over from the "
                       "previous run%s" % (" (pids %s)" % left if left else ""))
            self.sshRun("pkill -9 -f '^perl eccejobmonitor'; "
                        "pkill -f mopac-slow; true")
            self.setMopacDelay(150 if kills else 30 if drop or self.args.hold else 12)
            if os.path.exists(self.stubLog()):
                os.unlink(self.stubLog())
        name = "mopac-ch4-%s-%d" % (label, int(time.time()))
        if self.remote():
            rundir = "/home/%s/ecce-jobs/%s" % (self.args.remote_user, name)
            runUser = self.args.remote_user
        else:
            rundir = os.path.join(self.state, "jobs")
            os.makedirs(rundir, exist_ok=True)
            runUser = self.env()["ECCE_REALUSER"]

        rc, out = self.driver("create", self.userUrl(), name, "mopac_es", DECK,
                              "mopac.mop", self.args.machine, rundir, runUser)
        if not self.check(rc == 0, "calculation created"):
            say(out)
            return
        url = out.strip().splitlines()[-1]
        say("  calculation: " + url)

        extra = {}
        if self.args.legacy_transport:
            extra["ECCE_TRANSPORT"] = self.args.legacy_transport
        extra.update(env or {})
        if self.args.job_comms:
            extra["ECCE_JOB_COMMS"] = self.args.job_comms
        rc, out = self.driver("launch", url, extra=extra)
        launchOut = out
        say("\n".join("  | " + line for line in out.strip().splitlines()))
        if not self.check(rc == 0, "Launch ran to the end"):
            return
        if self.args.legacy_transport == "pty":
            self.check("ECCE_TRANSPORT=pty is no longer supported" in out,
                       "ECCE_TRANSPORT=pty was noted and the job ran on")
        if self.remote():
            self.checkTransport(out)
            ran = [l.split(":", 1)[1].strip() for l in out.splitlines()
                   if l.startswith("run directory:")]
            rundir = ran[-1] if ran else rundir

        #  Watch for the jobmaster/jobstore binaries while the job is alive:
        #  a process's exe is the only proof of which build was started.
        state = ""
        stdin = None
        dropped = False
        oldPids = []
        killed = []             # (pid, time) of monitors killed by kills
        seenAt = {}             # pid -> first time seen
        self.monitorLog = ""
        wait = drop or self.args.hold
        deadline = time.time() + WAIT_SECONDS * (
            4 if kills else 2 if wait else 1)
        while time.time() < deadline:
            self.seen.update(seenBinaries(self.home))
            self.readMonitorLog(name)
            self.masterLog(name)
            if self.remote():
                found = self.remoteMonitorStdin()
                if found and not stdin:
                    stdin = found
                    say("  remote eccejobmonitor stdin: %s" % found)
                if kills:
                    now = time.time()
                    for pid in self.monitorPids():
                        seenAt.setdefault(pid, now)
                        #  Only a monitor that has run a while, so that
                        #  each kill is a separate, spaced failure.
                        if (len(killed) < kills and now - seenAt[pid] > 8
                                and not any(k[0] == pid for k in killed)):
                            self.sshRun("kill -9 %d" % pid)
                            killed.append((pid, now))
                            say("  killed remote monitor %d (%d of %d)"
                                % (pid, len(killed), kills))
                if found and drop and not dropped:
                    dropped = True
                    oldPids = self.monitorPids()
                    rc, out = self.sshRun(DROP_SESSION)
                    self.dropNote = out.strip()
                    say("  dropped the monitor's session: %s" % self.dropNote)
                    self.check(out.startswith("killed"),
                               "killed the monitor's sshd session")
                    #  #205: the orphan has to notice within a heartbeat.
                    gone = False
                    t0 = time.time()
                    for _ in range(30):
                        time.sleep(1)
                        alive = [p for p in self.monitorPids() if p in oldPids]
                        if not alive:
                            gone = True
                            break
                    took = time.time() - t0
                    self.check(gone and took < 12,
                               "the monitor on the dropped session exited by "
                               "itself within 12s (%s)" % (
                                   "still running: %s" % alive if not gone
                                   else "pids %s, %.0fs" % (oldPids, took)))
            else:
                for _ in range(20):
                    stdin = stdin or monitorStdin()
                    time.sleep(0.05)
            rc, out = self.driver("state", url)
            state = out.strip().splitlines()[-1] if out.strip() else ""
            #  A lost monitor shows "waiting" until eccejobmaster has
            #  restarted eccejobstore (#208); system_failure is a lost job.
            if giveup:
                if "exited with final status" in self.masterLog(name):
                    break
            elif state in ("completed", "loaded", "failed", "killed",
                           "unsuccessful") or (
                    state == "system_failure" and not wait and not kills):
                break
        say("  eccejobmonitor stdin: %s" % stdin)
        if self.remote():
            isPty = stdin is not None and stdin.startswith("/dev/pts/")
            if self.check(stdin is not None, "remote monitor seen while the job ran"):
                self.check(not isPty, "remote monitor stdin is not a tty (%s)" % stdin)
        elif stdin is not None:
            self.check(stdin.startswith("pipe:"),
                       "monitor stdin is a pipe under %s" % label)
        else:
            self.check(False, "monitor seen while the job ran")
        if giveup:
            text = self.masterLog(name)
            for line in text.splitlines():
                if "restart count" in line or "exited with" in line or "reset" in line:
                    say("  master: " + line.strip()[:120])
            self.check(len(killed) >= 2, "the monitor was killed at least twice"
                       " (%d)" % len(killed))
            self.check("exited with final status" in text
                       and "restart count reset" not in text,
                       "eccejobmaster gave up without a reset")
            self.check(state != "completed",
                       "monitoring was abandoned, the run did not reach "
                       "completed (last: %s)" % (state or "none"))
            #  #208: a used-up restart budget is not a failure of the job.
            self.check(state == "waiting",
                       "the calculation was left waiting for login (last: %s)"
                       % (state or "none"))
            self.check("TE" not in self.props(url), "no TE was stored")
            return
        self.check(state == "completed",
                   "run state reached completed within %ds (last: %s)"
                   % (WAIT_SECONDS, state or "none"))
        if self.args.job_comms in ("socket", "socketlocal"):
            #  Socket comms are gone; the request is read as stdio, said so
            #  in the log, and the job is monitored over the stream.
            self.readMonitorLog(name)
            self.check("no longer supported" in self.monitorLog,
                       "ECCE_JOB_COMMS=%s was read as stdio and logged"
                       % self.args.job_comms)
        if kills:
            self.check(len(killed) == kills, "the monitor was killed %d times "
                       "(%d)" % (kills, len(killed)))
            self.check("restart count reset" in self.masterLog(name),
                       "eccejobmaster reset the restart count")
        if state == "completed" and (drop or kills):
            #  Nothing may be left once the job is done: neither the
            #  dropped monitor nor a replaced one.
            for _ in range(30):
                left = self.monitorPids()
                if not left:
                    break
                time.sleep(1)
            self.check(not left, "no eccejobmonitor left after the job "
                       "completed%s" % (" (pids %s)" % left if left else ""))
        if state != "completed":
            say("  ---- eccejobmaster.log\n" + self.masterLog(name)[-1500:])
            say("  ---- eccejobstore.log (last run)\n" + self.monitorLog[-2500:])
            self.diagnose(url)
            return

        rc, out = self.driver("props", url)
        props = out.split()
        say("  properties: " + " ".join(props))
        for prop in REQUIRED_PROPS:
            self.check(prop in props, "%s present in Props/" % prop)
        if self.args.local or self.args.folder:
            self.checkLocalStore(url, launchOut)
        if self.remote():
            self.checkRemoteRun(rundir, name)
            if drop or self.args.hold:
                self.checkRestarted(name)
            if self.args.expect_keepalive:
                self.check("ssh keepalive: nothing heard" in self.storeLogs(),
                           "the ssh keepalive declared the stream dead")

    def checkLocalStore(self, url, launchOut):
        """#216: the results are files in the calculation's own folder."""
        calc = url[len("file://"):] if url.startswith("file://") else url
        calc = calc.rstrip("/")
        base = (self.folder() if self.args.folder
                else os.path.join(self.localData(), "users", "local"))
        self.check(calc.startswith(base + "/"),
                   "the calculation is in %s: %s" % (base, calc))
        outputs = os.listdir(os.path.join(calc, "Outputs"))
        say("  Outputs/: %s" % " ".join(sorted(outputs)))
        self.check("mopac.mopout" in outputs, "the output file was stored "
                   "in the calculation's Outputs/")
        with open(os.path.join(calc, "Props", "TE"), errors="replace") as handle:
            te = handle.read()
        self.check("<value" in te and 'name="TE"' in te,
                   "Props/TE is the property document")
        self.check("state after launch: submitted" in launchOut,
                   "Launch left the calculation submitted")
        with open(os.path.join(calc, "Outputs", "eccejobstorelog.ecce_run_log"),
                  errors="replace") as handle:
            changes = re.findall(r'name="Calculation State Change"[^>]*>\s*(\w+)',
                                 handle.read())
        self.check(changes[-2:] == ["Running", "Complete"], "the run log records "
                   "the state changes Running, Complete (%s)" % ", ".join(changes))
        ran = [l.split(":", 1)[1].strip() for l in launchOut.splitlines()
               if l.startswith("run directory:")]
        if not self.remote():
            # Outside the data folder, the run directory follows the path
            # below the user's home directory.
            home = self.env().get("HOME", "")
            if not self.args.folder:
                below = os.path.relpath(calc, base)
            elif home and calc.startswith(home.rstrip("/") + "/"):
                below = os.path.relpath(calc, home)
            else:
                below = calc.lstrip("/")
            want = os.path.join(self.state, "jobs", below)
            self.check(ran and ran[-1] == want, "the run directory has the "
                       "server-mode layout: %s" % (ran[-1] if ran else None))

    def waitState(self, url, want=("completed", "loaded", "failed", "killed",
                                  "unsuccessful", "system_failure"), seconds=180):
        state = ""
        deadline = time.time() + seconds
        while time.time() < deadline:
            rc, out = self.driver("state", url)
            state = out.strip().splitlines()[-1] if out.strip() else ""
            if state in want:
                break
            time.sleep(1)
        return state

    def props(self, url):
        rc, out = self.driver("props", url)
        return sorted(out.split())

    def storeLogs(self):
        """eccejobmaster/eccejobstore logs of every job, while they still exist."""
        import glob
        text = ""
        for log in sorted(glob.glob(os.path.join(self.state, "tmp", "*", "jobs",
                                                 "*", "eccejob*.log"))):
            with open(log, errors="replace") as handle:
                text += "== %s\n%s\n" % (log, handle.read())
        return text

    def nwchemRestart(self):
        """#202: run an NWChem optimisation, Reset for Restart, run it again.

        The restarted job must store its properties without eccejobstore
        aborting, and must not lose the ones the first run left.
        """
        say("--- NWChem optimisation, then Reset for Restart")
        name = "nwchem-co-restart-%d" % int(time.time())
        rundir = os.path.join(self.state, "jobs")
        os.makedirs(rundir, exist_ok=True)
        deck = os.path.join(REPO, "tests", "e2e", "fixtures", "nwchem", "co-opt.nw")
        rc, out = self.driver("create", self.userUrl(), name, "nwchem_es", deck,
                              "nwch.nw", self.args.machine, rundir, self.user())
        if not self.check(rc == 0, "calculation created"):
            say(out)
            return
        url = out.strip().splitlines()[-1]
        rc, out = self.driver("launch", url)
        say("\n".join("  | " + l for l in out.strip().splitlines()[-6:]))
        if not self.check(rc == 0, "first Launch ran to the end"):
            return
        state = self.waitState(url)
        self.check(state == "completed", "first run reached completed (last: %s)" % state)
        time.sleep(3)
        first = self.props(url)
        say("  first run properties (%d): %s" % (len(first), " ".join(first)))
        self.check("GEOMTRACE" in first and "TE" in first, "first run stored GEOMTRACE and TE")

        restartDeck = os.path.join(self.state, "restart.nw")
        with open(deck) as src, open(restartDeck, "w") as dst:
            for line in src:
                dst.write("restart CO\n" if line.strip().lower().startswith("start") else line)
        rc, out = self.driver("restart", url, restartDeck, "nwch.nw")
        if not self.check(rc == 0, "reset for restart and edited deck stored"):
            say(out)
            return
        rc, out = self.driver("launch", url)
        say("\n".join("  | " + l for l in out.strip().splitlines()[-6:]))
        if not self.check(rc == 0, "restart Launch ran to the end"):
            return
        state = self.waitState(url)
        self.check(state == "completed", "restarted run reached completed (last: %s)" % state)
        time.sleep(3)
        second = self.props(url)
        say("  restarted run properties (%d): %s" % (len(second), " ".join(second)))
        text = self.storeLogs()
        for line in text.splitlines():
            if any(k in line for k in ("Cannot re-parse", "restart count", "exited with",
                                       "Fatal", "terminating")):
                say("  log: " + line.strip()[:160])
        self.check("Cannot re-parse" not in text, "no 'Cannot re-parse frequency last' in the logs")
        self.check(set(first) <= set(second), "the restart kept the first run's properties (%s)"
                   % " ".join(sorted(set(first) - set(second))))

    def masterLog(self, name):
        import glob
        text = ""
        for log in glob.glob(os.path.join(self.state, "tmp", "*", "jobs",
                                          name + "__*", "eccejobmaster.log")):
            try:
                with open(log, errors="replace") as handle:
                    text += handle.read()
            except OSError:
                pass
        #  The cache directory goes once the job is done; keep the last text.
        if not hasattr(self, "masterSeen"):
            self.masterSeen = {}
        if text:
            self.masterSeen[name] = text
        return self.masterSeen.get(name, "")

    def checkRestarted(self, name):
        """The dropped monitor must have been replaced by a restart."""
        text = ""
        #  The state turns completed a little before eccejobstore exits.
        for _ in range(30):
            text = self.masterLog(name)
            if "exited with final status" in text:
                break
            time.sleep(1)
        for line in text.splitlines():
            if "restart count" in line or "exited with status" in line:
                say("  master: " + line.strip()[:120])
        self.check("exited with status value 4" in text
                   and "restart count is 1" in text,
                   "eccejobstore lost the monitor (exit 4) and was restarted")
        self.check("exited with final status value 0" in text,
                   "the restarted eccejobstore finished the job")

    def readMonitorLog(self, name):
        import glob
        for log in glob.glob(os.path.join(self.state, "tmp", "*", "jobs",
                                          name + "__*", "eccejobstore.log")):
            try:
                with open(log, errors="replace") as handle:
                    self.monitorLog = handle.read() or self.monitorLog
            except OSError:
                pass

    def checkTransport(self, launchOut):
        """The connection Launch made must be the one the machine needs."""
        which = "the OpenSSH client" if self.args.shared_connection else "libssh"
        self.check("ssh transport: commands run over " + which in launchOut,
                   "Launch connected over %s" % which)

    def checkRemoteRun(self, rundir, name):
        """Show that MOPAC ran in the sshd container, not here."""
        rc, out = self.sshRun("hostname; ls -A %s" % rundir)
        say("  remote host and run directory:\n" + "\n".join(
            "  | " + line for line in out.strip().splitlines()))
        files = out.split()
        self.check(rc == 0 and any(f.endswith(".out") for f in files),
                   "MOPAC output exists in %s on the remote machine" % rundir)
        #  Launch deletes it first, so only the remote job can have written it.
        self.check(".ecce.status" in files,
                   "the remote monitor reported status (.ecce.status)")
        text = self.monitorLog
        if not text:
            say("  (no eccejobstore.log seen: the cache is gone once it finishes)")
        else:
            for line in text.splitlines():
                if "Started job monitor" in line:
                    say("  monitor connection: " + line.strip()[:160])
                    break
            self.check("Started job monitor (stdio comms)" in text,
                       "the remote monitor ran over a channel")
        used = ""
        if os.path.exists(self.stubLog()):
            with open(self.stubLog()) as handle:
                #  `ssh -G` is how ECCE asks whether the host shares a
                #  connection; it connects to nothing.
                used = "\n".join(l for l in handle.read().splitlines()
                                 if not l.startswith("ssh -G ")).strip()
        self.check(used == "", "no scp/sftp/ssh was started over libssh%s"
                   % (": " + used.replace("\n", "; ") if used else ""))
        self.check(not os.path.exists(rundir),
                   "the run directory does not exist on this machine")

    def diagnose(self, url):
        jobs = os.path.join(self.state, "jobs")
        for base, _dirs, files in os.walk(jobs):
            for name in files:
                if name.endswith((".log", ".out", ".mopout")) or name.startswith("submit"):
                    path = os.path.join(base, name)
                    try:
                        with open(path, errors="replace") as handle:
                            tail = handle.read()[-1500:]
                    except OSError:
                        continue
                    say("  ---- %s\n%s" % (path, tail))
        for base, _dirs, files in os.walk(os.path.join(self.state, ".ECCE")):
            for name in files:
                if name.startswith("eccejobmaster") and name.endswith(".log"):
                    with open(os.path.join(base, name), errors="replace") as handle:
                        say("  ---- %s\n%s" % (name, handle.read()[-1500:]))


def registerRemote(state, machine, user):
    """Register `machine` for the isolated user as Machine Registration would.

    The Shell queue manager and a CONFIG file naming MOPAC's path on the
    remote side are all a workstation with the code installed needs.
    """
    prefs = os.path.join(state, ".ECCE")
    with open(os.path.join(prefs, "MyMachines"), "w") as handle:
        handle.write("%s\t%s\tGeneric\tRemote test host\tUnspecified\t4:1\t"
                     "ssh\t:MOPAC\tWS\n" % (machine, machine))
    with open(os.path.join(prefs, "Queues"), "w") as handle:
        handle.write("Queues: %s\n%s|queueMgrName:   Shell\n" % (machine, machine))
    with open(os.path.join(prefs, "CONFIG." + machine), "w") as handle:
        handle.write("MOPAC: /home/%s/mopac-slow\nperlPath: /usr/bin\n" % user)


def prerequisites(build, local):
    for exe in ("launchjob", "eccejobstore", "eccejobmaster", "ecmd"):
        if not os.access(os.path.join(build, exe), os.X_OK):
            skip("%s is not built in %s (ninja launchjob eccejobmaster "
                 "eccejobstore ecmd)" % (exe, build))
    for tool in (("mopac", "mosquitto", "perl") if local else
                 ("mopac", "apache2", "htpasswd", "mosquitto", "perl")):
        if not shutil.which(tool) and not (
                tool == "apache2" and os.access("/usr/sbin/apache2", os.X_OK)):
            skip("%s is not installed" % tool)
    if not os.path.exists(os.path.join(build, "siteconfig-local", "DataServers")):
        skip("%s/siteconfig-local is missing (run cmake)" % build)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--build", default=os.path.join(REPO, "build-native"))
    parser.add_argument("--legacy-transport", metavar="VALUE",
                        help="run the jobs with ECCE_TRANSPORT=VALUE, as an "
                        "8.x eccejobmaster exports it; there is only one "
                        "transport, so the job must run as usual")
    parser.add_argument("--machine", default="localhost",
                        help="machine to run on; anything but localhost is "
                        "registered as an ssh machine (default: localhost)")
    parser.add_argument("--remote-user", default="bashuser",
                        help="login name on --machine")
    parser.add_argument("--cwd", help="working directory of the launch "
                        "(default $ECCE_HOME/bin, as under the gateway)")
    parser.add_argument("--drop", action="store_true",
                        help="also run with the "
                        "monitor's ssh session killed mid-job")
    parser.add_argument("--kill", action="store_true",
                        help="#205/#206: kill the remote monitor process four "
                        "times, as a login node would; the restart count "
                        "must reset, and without the reset must run out")
    parser.add_argument("--hold", action="store_true",
                        help="an outside agent freezes the link mid-job (#204 "
                        "keepalive test): allow the monitor to be restarted")
    parser.add_argument("--expect-keepalive", action="store_true",
                        help="with --hold: the restart must come from the ssh "
                        "keepalive (ECCE_SSH_KEEPALIVE)")
    parser.add_argument("--nwchem-restart", action="store_true",
                        help="#202: only run NWChem, Reset for Restart, run again")
    parser.add_argument("--shared-connection", action="store_true",
                        help="the machine's ssh config shares one connection "
                        "(ControlMaster) that is the only way in: Launch must "
                        "choose the OpenSSH client and still run, monitor and "
                        "copy back the job")
    parser.add_argument("--job-comms", metavar="VALUE",
                        help="run the jobs with ECCE_JOB_COMMS=VALUE (socket "
                        "and socketlocal are read as stdio)")
    parser.add_argument("--local", action="store_true",
                        help="#216: keep the data in a local folder "
                        "(ECCE_LOCAL_DATA) instead of a data server")
    parser.add_argument("--folder", action="store_true",
                        help="create the calculation in a plain local folder, "
                        "as Builder > Save As to the Local Filesystem does")
    parser.add_argument("--keep", action="store_true",
                        help="leave the services running afterwards")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    build = os.path.abspath(args.build)
    prerequisites(build, args.local)
    if args.nwchem_restart and not shutil.which("nwchem"):
        skip("nwchem is not installed")

    state = isolate.resolveStateDir(
        os.environ.get("ECCE_TEST_STATE")
        or isolate.defaultStateDir() + "-launch")
    os.makedirs(state, exist_ok=True)
    note, _ = sweep(state)
    if note:
        say(note)

    shutil.rmtree(os.path.join(state, "jobs"), ignore_errors=True)
    #  TempStorage keeps the staging directory and monitor logs here, by
    #  default in /tmp/ecce_<user>, which a live session shares.
    shutil.rmtree(os.path.join(state, "tmp"), ignore_errors=True)
    os.makedirs(os.path.join(state, "tmp"))
    os.environ["ECCE_TMPDIR"] = os.path.join(state, "tmp")
    install = treeInstall(state, build)
    settings = isolate.apply(install, state)
    home = settings["ECCE_HOME"]
    shutil.rmtree(os.path.join(state, "folder"), ignore_errors=True)
    if args.local:
        #  Local mode must not need it; its absence proves it is not read.
        os.unlink(os.path.join(home, "siteconfig", "DataServers"))
        shutil.rmtree(os.path.join(state, "localdata"), ignore_errors=True)
    os.environ["ECCE_TEST_HOME"] = install
    say(isolate.describe(settings))

    import apps  # noqa: F401  (fixture reads apps.INSTALL)
    import fixture

    if args.machine == "localhost":
        with open(os.path.join(state, ".ECCE", "CONFIG.localhost"), "w") as handle:
            handle.write("MOPAC: %s\nNWChem: %s\n" % (shutil.which("mopac"),
                                                     shutil.which("nwchem") or ""))
    else:
        registerRemote(state, args.machine, args.remote_user)

    suite = Suite(args, build, state, home)
    say("binaries under test (ECCE_HOME=%s):" % home)
    for name in ("eccejobmaster", "eccejobstore", "ecmd", "launchjob"):
        path = os.path.realpath(os.path.join(home, "bin", name))
        suite.check(path == os.path.join(build, name),
                    "%s -> %s" % (name, path))
    for name in ("gensub", "eccejobmonitor"):
        path = os.path.realpath(os.path.join(home, "scripts", name))
        suite.check(path == os.path.join(REPO, "scripts", name),
                    "%s -> %s" % (name, path))
    found = shutil.which("gensub", path=suite.env()["PATH"])
    suite.check(found is not None and os.path.realpath(found)
                == os.path.join(REPO, "scripts", "gensub"),
                "gensub on PATH -> %s" % found)

    label = "ssh" if args.machine != "localhost" else "local"
    modes = [(label, False)]
    if args.drop and args.machine != "localhost":
        modes.append((label + "-drop", True))
    try:
        if not suite.services(True):
            suite.check(False, "services started")
        else:
            if not args.local:
                fixture.ensureRealUserAccount()
            if args.nwchem_restart:
                suite.nwchemRestart()
                modes = []
            for name, drop in modes:
                suite.one(name, drop)
            if args.kill and args.machine != "localhost":
                suite.one(label + "-kill", kills=4, env={
                    "ECCE_JOB_MAXCONNECTS": "2",
                    "ECCE_JOB_RESTARTRESET": "5",
                    "ECCE_JOB_MAXQUICKTIME": "1"})
                suite.one(label + "-kill-noreset", kills=4,
                          giveup=True, env={
                              "ECCE_JOB_MAXCONNECTS": "2",
                              "ECCE_JOB_RESTARTRESET": "100000",
                              "ECCE_JOB_MAXQUICKTIME": "1"})
        for name, path in sorted(suite.seen.items()):
            inBuild = os.path.dirname(path) == build
            suite.check(inBuild, "ran while the job was alive: %s" % path)
    finally:
        if suite.remote():
            suite.sshRun("pkill -9 -f '^perl eccejobmonitor'; "
                         "pkill -f mopac-slow; true")
        if not args.keep:
            suite.services(False)
            note, n = sweep(state)
            left = procsUnder(state)
            say("cleanup: %d extra process(es) stopped, %d left" % (n, len(left)))
            if left:
                suite.failures.append("processes left running: %r" % left)

    say("")
    if suite.failures:
        say("FAILED (%d): %s" % (len(suite.failures), "; ".join(suite.failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
