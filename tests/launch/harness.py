"""Shared plumbing for the suites that run jobs through the real Launch.

Builds a $ECCE_HOME made only of the build and source trees, starts an
isolated data server and broker (tests/apps/isolate.py), and drives the
`launchjob` test binary.  Never touches the caller's own ~/.ECCE.
"""

import os
import shutil
import signal
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import isolate  # noqa: E402

SKIP = 77
#  One session for the run (#233). DISPLAY is unset: these jobs need no X
#  server, and the session must not depend on one.
SESSION_ID = os.urandom(8).hex()
FINAL_STATES = ("completed", "loaded", "failed", "killed", "unsuccessful")
BINARIES = ("eccejobmaster", "eccejobstore", "ecmd", "launchjob")


def say(text):
    print(text, flush=True)


def skip(reason):
    say("SKIP: " + reason)
    sys.exit(SKIP)


def link(source, target):
    os.makedirs(os.path.dirname(target), exist_ok=True)
    if os.path.lexists(target):
        os.unlink(target)
    os.symlink(source, target)


def treeInstall(state, build):
    """An install directory made only of the build and source trees."""
    home = os.path.join(state, "tree-install")
    shutil.rmtree(home, ignore_errors=True)
    for exe in BINARIES:
        link(os.path.join(build, exe), os.path.join(home, "bin", exe))
    for sub in ("gateway", "dataserver"):
        scripts = os.path.join(REPO, "packaging", sub)
        for name in os.listdir(scripts):
            path = os.path.join(scripts, name)
            if name.startswith("ecce-") and (os.access(path, os.X_OK)
                                             or name.endswith("-lib.sh")):
                link(path, os.path.join(home, "bin", name))
    nwdir = os.path.join(REPO, "packaging", "nwchem", "ecce-nwchem-datadir")
    if os.path.exists(nwdir):
        link(nwdir, os.path.join(home, "bin", "ecce-nwchem-datadir"))
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


def _ancestors():
    pids, pid = set(), os.getpid()
    while pid > 1:
        pids.add(pid)
        try:
            with open("/proc/%d/stat" % pid) as handle:
                pid = int(handle.read().rsplit(")", 1)[1].split()[1])
        except (OSError, ValueError, IndexError):
            break
    return pids


def procsUnder(state):
    """PIDs whose command line or working directory lies in `state`.

    Never this process or its parents: a shell started with the state path
    on its command line is not a leftover service.
    """
    needle = os.fsencode(state.rstrip("/"))
    found = []
    skip = _ancestors()
    for entry in os.listdir("/proc"):
        if not entry.isdigit() or int(entry) in skip:
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
            if name.startswith("ecce") or name == "launchjob":
                found[name] = exe
    return found


def prerequisites(build, tools):
    for exe in BINARIES:
        if not os.access(os.path.join(build, exe), os.X_OK):
            skip("%s is not built in %s (ninja launchjob eccejobmaster "
                 "eccejobstore ecmd)" % (exe, build))
    for tool in ("apache2", "htpasswd", "mosquitto", "perl") + tuple(tools):
        if not shutil.which(tool) and not (
                tool == "apache2" and os.access("/usr/sbin/apache2", os.X_OK)):
            skip("%s is not installed" % tool)
    if not os.path.exists(os.path.join(build, "siteconfig-local", "DataServers")):
        skip("%s/siteconfig-local is missing (run cmake)" % build)


class Session(object):
    """An isolated ECCE (services, $ECCE_HOME, user account) for one run."""

    def __init__(self, build, tag, codes, ports=None, keep=False, local=False):
        self.build = os.path.abspath(build)
        self.keep = keep
        #  #216: data in a folder (ECCE_LOCAL_DATA); no data server.
        self.local = local
        self.failures = []
        self.seen = {}
        self._authLock = threading.Lock()
        self._authCount = 0

        #  State directory and ports are this run's own (isolate.runState,
        #  isolate.apply), so concurrent runs cannot meet; `ports` is
        #  ignored, kept for old callers.  ECCE_TEST_STATE and the port
        #  variables still override.
        self.state = isolate.resolveStateDir(isolate.runState(
            "%s" % tag, keep=keep))
        os.makedirs(self.state, exist_ok=True)
        note, _ = sweep(self.state)
        if note:
            say(note)
        shutil.rmtree(os.path.join(self.state, "jobs"), ignore_errors=True)
        #  TempStorage keeps the staging directory and monitor logs here, by
        #  default in /tmp/ecce_<user>, which a live session shares.
        shutil.rmtree(os.path.join(self.state, "tmp"), ignore_errors=True)
        os.makedirs(os.path.join(self.state, "tmp"))
        os.environ["ECCE_TMPDIR"] = os.path.join(self.state, "tmp")
        install = treeInstall(self.state, self.build)
        settings = isolate.apply(install, self.state)
        self.home = settings["ECCE_HOME"]
        if self.local:
            #  Local mode must not need it; its absence proves it is not read.
            os.unlink(os.path.join(self.home, "siteconfig", "DataServers"))
            shutil.rmtree(self.localData(), ignore_errors=True)
        os.environ["ECCE_TEST_HOME"] = install
        say(isolate.describe(settings))

        import apps  # noqa: F401  (fixture reads apps.INSTALL)
        import fixture
        self.fixture = fixture

        #  Machine "localhost" under the Shell queue manager runs each
        #  code from the path registered here.
        with open(os.path.join(self.state, ".ECCE", "CONFIG.localhost"), "w") as h:
            h.write("".join("%s: %s\n" % (k, v) for k, v in codes.items()))

    def check(self, ok, what):
        say("  %s %s" % ("ok  " if ok else "FAIL", what))
        if not ok:
            self.failures.append(what)
        return ok

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
        env.pop("ECCE_TRANSPORT", None)
        env.pop("ECCE_LOCAL_DATA", None)
        if self.local:
            env["ECCE_LOCAL_DATA"] = self.localData()
        env.update(extra or {})
        return env

    def localData(self):
        return os.path.join(self.state, "localdata")

    def run(self, argv, extra=None, timeout=180, cwd=None):
        result = subprocess.run(argv, env=self.env(extra), cwd=cwd,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=timeout)
        return result.returncode, result.stdout.decode("utf-8", "replace")

    def services(self, start):
        if start:
            servers = ["ecce-gateway-start"]
            if not self.local:
                servers.insert(0, "ecce-dataserver-start")
            for script in servers:
                rc, out = self.run([os.path.join(self.home, "bin", script)],
                                   timeout=240)
                say("  %s: rc=%d %s" % (script, rc,
                                        out.strip().replace("\n", " | ")[:300]))
                if rc != 0:
                    return False
            if not self.local:
                self.fixture.ensureRealUserAccount()
            return True
        for script in ("ecce-gateway-stop", "ecce-dataserver-stop"):
            try:
                self.run([os.path.join(self.home, "bin", script)], timeout=120)
            except subprocess.TimeoutExpired:
                pass
        return True

    def user(self):
        return self.env()["ECCE_REALUSER"]

    def userUrl(self):
        if self.local:
            # Local mode's home is always users/local (#216).
            return "file://%s/users/local" % self.localData()
        return "%s/users/%s" % (
            self.fixture.base().rsplit("/users/", 1)[0], self.user())

    def authFile(self):
        """Credentials for the calling user's own account, as AuthCache reads them.

        AuthCache deletes the file once read, so every call gets its own.
        """
        with self._authLock:
            self._authCount += 1
            path = os.path.join(self.state, "auth.%d.pipe" % self._authCount)
        root = os.path.join(self.fixture.stateDir(), "htdocs", "Ecce",
                            "users", self.user())
        realm = self.fixture.realm(root)
        prefix = "http://localhost:%s/" % os.environ["ECCE_DATASERVER_PORT"]
        with open(path, "w") as handle:
            handle.write("2\n%s%s|%s|ecce\n%s|%s|ecce\n"
                         % (prefix, realm, self.user(), prefix, self.user()))
        os.chmod(path, 0o600)
        return path

    def driver(self, *argv, timeout=180):
        """One launchjob invocation, with credentials piped in.

        The gateway starts every app with `cd $ECCE_HOME/bin && ./app`, and
        eccejobmaster runs "./eccejobstore" relative to that, so mirror it.
        """
        pipe = os.devnull if self.local else self.authFile()
        return self.run([os.path.join(self.home, "bin", "launchjob"),
                         "-pipe", pipe] + list(argv),
                        cwd=os.path.join(self.home, "bin"), timeout=timeout)

    def registerMachine(self, name, manager, codes, queues=("normal", "debug"),
                        limits=None, config=None):
        """Register `name` (this host, reached as localhost) under a queue manager.

        Writes what Machine Registration and the Queues preference would:
        MyMachines, Queues, <name>.Q and CONFIG.<name> in the user's ~/.ECCE.
        The capability column enables the launcher's queue, wall time,
        memory and account controls, which decide what Launch passes on.
        """
        prefs = os.path.join(self.state, ".ECCE")
        self.machines = getattr(self, "machines", {})
        self.machines[name] = manager
        with open(os.path.join(prefs, "MyMachines"), "w") as h:
            for m in self.machines:
                h.write("%s\tlocalhost\tGeneric\tqueue test machine\tUnspecified\t"
                        "1:16\tssh\t:%s\tMN:RD:SD:Q:TL:MM:SS:AA\n"
                        % (m, ":".join(sorted(codes))))
        with open(os.path.join(prefs, "Queues"), "w") as h:
            h.write("Queues: %s\n" % " ".join(self.machines))
            for m, mgr in self.machines.items():
                h.write("%s|queueMgrName:   %s\n%s|prefFile:       %s.Q\n\n"
                        % (m, mgr, m, m))
        with open(os.path.join(prefs, name + ".Q"), "w") as h:
            h.write("Queues: %s\n" % " ".join(queues))
            for q in queues:
                lim = (limits or {}).get(q, (1, 16, 30 if q == "debug" else 1440))
                h.write("%s|minProcessors: %d\n%s|maxProcessors: %d\n"
                        "%s|runLimit: %d\n%s|memLimit: 0\n%s|memUnits: MB\n"
                        % (q, lim[0], q, lim[1], q, lim[2], q, q))
        with open(os.path.join(prefs, "CONFIG." + name), "w") as h:
            h.write("".join("%s: %s\n" % kv for kv in codes.items()))
            h.write("perlPath: /usr/bin\n")
            h.write("".join("%s: %s\n" % kv for kv in (config or {}).items()))

    def create(self, name, resourceType, deck, deckName, rundir,
               machine="localhost", extra=()):
        rc, out = self.driver("create", self.userUrl(), name, resourceType,
                              deck, deckName, machine, rundir, self.user(),
                              *extra)
        if rc != 0:
            return None, out
        return out.strip().splitlines()[-1], out

    def launch(self, url):
        return self.driver("launch", url)

    def state_of(self, url):
        rc, out = self.driver("state", url)
        return out.strip().splitlines()[-1] if out.strip() else ""

    def waitState(self, url, seconds=180, want=FINAL_STATES):
        state = ""
        deadline = time.time() + seconds
        while time.time() < deadline:
            self.seen.update(seenBinaries(self.home))
            state = self.state_of(url)
            if state in want or state == "system_failure":
                #  system_failure (a job that vanished) may still be followed
                #  by a later state from a restarted store; give it a moment.
                if state != "system_failure":
                    break
                time.sleep(2)
                state = self.state_of(url)
                if state in want:
                    break
                break
            time.sleep(1)
        return state

    def props(self, url):
        rc, out = self.driver("props", url)
        return sorted(out.split())

    def propsDir(self, project, name):
        """Where the data server keeps a calculation's Props/ files."""
        return os.path.join(self.fixture.stateDir(), "htdocs", "Ecce", "users",
                            self.user(), project, name, "Props")

    def stop(self):
        if self.keep:
            return []
        self.services(False)
        sweep(self.state)
        return procsUnder(self.state)
