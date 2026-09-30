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

    tests/launch/run_tests.py [--build build-native] [--transport unset|direct|both]
                              [--keep] [-v]
    tests/launch/run_tests.py --machine sshtest --remote-user bashuser \
                              --transport unset|ssh|both

With --machine the job runs on a machine reached over ssh instead of on
localhost.  The test registers it (MyMachines, Queues, CONFIG.<machine>) for
the isolated user and leaves ~/.ssh alone, so the caller has to have made
`--machine` and its host resolvable by ssh non-interactively; see
tests/launch/remote_test.sh, which does that inside a container.

Exit status 77 (CTest SKIP) when a prerequisite is missing.
"""

import argparse
import os
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
DISPLAY_KEY = ":83"          # only a key for per-session state, no X server
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
            if name.startswith("ecce-") and os.access(path, os.X_OK):
                link(path, os.path.join(home, "bin", name))
    link(os.path.join(REPO, "packaging", "nwchem", "ecce-nwchem-datadir"),
         os.path.join(home, "bin", "ecce-nwchem-datadir"))
    link(os.path.join(REPO, "scripts"), os.path.join(home, "scripts"))
    link(os.path.join(REPO, "java", "lib"), os.path.join(home, "java", "lib"))
    link(os.path.join(REPO, "data", "admin"), os.path.join(home, "data", "admin"))
    link(os.path.join(REPO, "data", "client"), os.path.join(home, "data", "client"))
    link(os.path.join(REPO, "packaging", "gateway", "activemq.xml.ecce"),
         os.path.join(home, "server", "activemq-conf", "activemq.xml"))
    link(os.path.join(REPO, "packaging", "dataserver", "httpd.conf.ecce"),
         os.path.join(home, "server", "httpd-conf", "httpd.conf.ecce"))

    #  siteconfig is generated at install time for these three files.
    site = os.path.join(home, "siteconfig")
    shutil.copytree(os.path.join(REPO, "siteconfig"), site)
    for name in ("jndi.properties", "DataServers", "site_runtime"):
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
            return os.readlink("/proc/%s/fd/0" % entry)
        except OSError:
            continue
    return None


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
            "DISPLAY": DISPLAY_KEY,
            "ECCE_REALUSER": os.environ.get("USER") or subprocess.check_output(
                ["id", "-un"]).decode().strip(),
            "PATH": "%s/scripts:%s/scripts/parsers:%s" % (
                self.home, self.home, os.environ["PATH"]),
        })
        env.pop("ECCE_NO_REAP", None)
        env.pop("ECCE_TRANSPORT", None)
        if self.remote():
            env["ECCE_RCOM_LOGMODE"] = "1"
        env.update(extra or {})
        return env

    def remote(self):
        return self.args.machine != "localhost"

    def sshRun(self, command):
        """Run `command` on the remote machine, outside ECCE."""
        result = subprocess.run(
            ["ssh", "-o", "BatchMode=yes", "%s@%s" % (self.args.remote_user,
                                                      self.args.machine),
             command], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            timeout=60)
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

    def services(self, start):
        if start:
            for script in ("ecce-dataserver-start", "ecce-gateway-start"):
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

    def driver(self, *argv, transport=None):
        """One launchjob invocation, with credentials piped in."""
        pipe = self.authFile(os.path.join(self.state, "auth.pipe"))
        extra = {"ECCE_TRANSPORT": transport} if transport else None
        #  The gateway starts every app with `cd $ECCE_HOME/bin && ./app`,
        #  and eccejobmaster runs "./eccejobstore" relative to that, so the
        #  default mirrors it.  --cwd . shows what happens otherwise.
        return self.run([os.path.join(self.home, "bin", "launchjob"),
                         "-pipe", pipe] + list(argv), extra=extra,
                        cwd=self.args.cwd or os.path.join(self.home, "bin"))

    def user(self):
        return self.env()["ECCE_REALUSER"]

    def userUrl(self):
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

    def one(self, label, transport):
        say("--- %s (ECCE_TRANSPORT=%s)" % (label, transport or "unset"))
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

        rc, out = self.driver("launch", url, transport=transport)
        say("\n".join("  | " + line for line in out.strip().splitlines()))
        if not self.check(rc == 0, "Launch ran to the end"):
            return
        if self.remote():
            self.checkTransport(out, transport)
            ran = [l.split(":", 1)[1].strip() for l in out.splitlines()
                   if l.startswith("run directory:")]
            rundir = ran[-1] if ran else rundir

        #  Watch for the jobmaster/jobstore binaries while the job is alive:
        #  a process's exe is the only proof of which build was started.
        state = ""
        stdin = None
        self.monitorLog = ""
        deadline = time.time() + WAIT_SECONDS
        while time.time() < deadline:
            self.seen.update(seenBinaries(self.home))
            self.readMonitorLog(name)
            for _ in range(20):
                stdin = stdin or monitorStdin()
                time.sleep(0.05)
            rc, out = self.driver("state", url)
            state = out.strip().splitlines()[-1] if out.strip() else ""
            if state in ("completed", "loaded", "failed", "killed",
                         "unsuccessful", "system_failure"):
                break
        say("  eccejobmonitor stdin: %s" % stdin)
        if self.remote():
            pass        # the monitor runs on the remote machine
        elif stdin is not None:
            self.check(stdin.startswith("pipe:") == (transport == "direct"),
                       "monitor stdin is %s under %s"
                       % ("a pipe" if transport == "direct" else "a tty",
                          label))
        elif transport == "direct":
            self.check(False, "monitor seen while the job ran")
        self.check(state == "completed",
                   "run state reached completed within %ds (last: %s)"
                   % (WAIT_SECONDS, state or "none"))
        if state != "completed":
            self.diagnose(url)
            return

        rc, out = self.driver("props", url)
        props = out.split()
        say("  properties: " + " ".join(props))
        for prop in REQUIRED_PROPS:
            self.check(prop in props, "%s present in Props/" % prop)
        if self.remote():
            self.checkRemoteRun(rundir, name)

    def readMonitorLog(self, name):
        import glob
        for log in glob.glob(os.path.join(self.state, "tmp", "*", "jobs",
                                          name + "__*", "eccejobstore.log")):
            try:
                with open(log, errors="replace") as handle:
                    self.monitorLog = handle.read() or self.monitorLog
            except OSError:
                pass

    def checkTransport(self, launchOut, transport):
        """The connection Launch made must be the one this mode asks for."""
        viaSsh = "ssh transport: commands run over libssh" in launchOut
        self.check(viaSsh == (transport == "ssh"),
                   "Launch connected over %s"
                   % ("libssh" if viaSsh else "the pty ssh path"))

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
        if text:
            for mark in ("Connecting to compute server", "Started job monitor",
                         "Authenticated to", "ssh transport"):
                for line in text.splitlines():
                    if mark in line:
                        say("  monitor connection: " + line.strip()[:160])
                        break
            self.check("Connecting to compute server" in text
                       and "ssh transport" not in text,
                       "the remote monitor connection is still the pty ssh "
                       "(expected until the monitor moves to a channel)")
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


def registerRemote(state, machine):
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
        handle.write("MOPAC: /usr/bin/mopac\nperlPath: /usr/bin\n")


def prerequisites(build):
    for exe in ("launchjob", "eccejobstore", "eccejobmaster", "ecmd"):
        if not os.access(os.path.join(build, exe), os.X_OK):
            skip("%s is not built in %s (ninja launchjob eccejobmaster "
                 "eccejobstore ecmd)" % (exe, build))
    for tool in ("mopac", "csh", "apache2", "htpasswd", "java", "perl"):
        if not shutil.which(tool) and not (
                tool == "apache2" and os.access("/usr/sbin/apache2", os.X_OK)):
            skip("%s is not installed" % tool)
    if not os.path.exists(os.path.join(build, "siteconfig-local", "DataServers")):
        skip("%s/siteconfig-local is missing (run cmake)" % build)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--build", default=os.path.join(REPO, "build-native"))
    parser.add_argument("--transport", default="both",
                        choices=("unset", "direct", "ssh", "both"))
    parser.add_argument("--machine", default="localhost",
                        help="machine to run on; anything but localhost is "
                        "registered as an ssh machine (default: localhost)")
    parser.add_argument("--remote-user", default="bashuser",
                        help="login name on --machine")
    parser.add_argument("--cwd", help="working directory of the launch "
                        "(default $ECCE_HOME/bin, as under the gateway)")
    parser.add_argument("--keep", action="store_true",
                        help="leave the services running afterwards")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    build = os.path.abspath(args.build)
    prerequisites(build)

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
    os.environ["ECCE_TEST_HOME"] = install
    say(isolate.describe(settings))

    import apps  # noqa: F401  (fixture reads apps.INSTALL)
    import fixture

    if args.machine == "localhost":
        with open(os.path.join(state, ".ECCE", "CONFIG.localhost"), "w") as handle:
            handle.write("MOPAC: %s\n" % shutil.which("mopac"))
    else:
        registerRemote(state, args.machine)

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

    second = ("ssh", "ssh") if args.machine != "localhost" else ("direct", "direct")
    if args.machine != "localhost" and args.transport == "direct":
        skip("--transport direct is for localhost")
    modes = {"unset": [("pty", None)], "direct": [("direct", "direct")],
             "ssh": [("ssh", "ssh")],
             "both": [("pty", None), second]}[args.transport]
    try:
        if not suite.services(True):
            suite.check(False, "services started")
        else:
            fixture.ensureRealUserAccount()
            for label, transport in modes:
                suite.one(label, transport)
        for name, path in sorted(suite.seen.items()):
            inBuild = os.path.dirname(path) == build
            suite.check(inBuild, "ran while the job was alive: %s" % path)
    finally:
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
