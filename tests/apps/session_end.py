#!/usr/bin/env python3
"""Does quitting the last app end an ECCE session? (#185)

With the Gateway window hidden (#93) there is no Quit button, so the
session has to end when its last app closes: `ecce` returns and the
gateway goes. The broker and the data server stay up; only an explicit Quit and Stop Server stops the broker.
This drives that through the real wrappers on a private Xvfb, closing
windows the way a window manager does (WM_DELETE_WINDOW):

  organizer   close the Organizer; `ecce` returns, the per-user broker
              is stopped (mode 1), the data server is still running
  builder     open a Builder first; closing the Organizer alone must not
              end the session, closing the Builder afterwards must
  jobstore    a stand-in for a running eccejobstore must not hold the
              session open, and must survive it, keeping its session's
              broker and files until it ends (#233, decision 6)
  stop        ecce-gateway-stop (Quit and Stop Server) ends the session
              and stops the broker
  quit-stop   the Organizer's Quit and Stop Server, clicked: all the
              teardown's output reaches `ecce`'s stream before it returns
  remote      #167's recipe (mode 2): with the server account marked
              (ecce-remote-setup --server) its mosquitto listens on TCP;
              a -remote client logs in to it with its data server account,
              and neither the server's own plain quit nor the client's quit
              stops it
  remote-refused  a login the (shared) message broker does not accept is
              reported in a dialog naming the account
  remote-down `ecce -remote` with nothing listening on the central
              server's ports: one message naming them, a non-zero exit,
              no gateway left to abort
  displays    one user on two displays: the per-user broker outlives the
              first session and goes with the last
  same-display  two `ecce` on one display are two sessions (#233): each
              ends alone, and neither sweeps the other's files
  two-sessions  two sessions; the second's Quit and Stop Server is not
              offered and its scripts refuse, the first keeps browsing,
              opening a calculation and publishing (the alpha.6-rc2 report);
              -server and -shared: the same in modes 2 and 3
  services-killed  data server and broker killed under a running
              Organizer: it says so and keeps running
  join        an app started without a session id joins the newest live
              session; with none alive it is a session of its own
  display-changes  an app of the session started with another DISPLAY
              (:N.0 for :N, as after an ssh -X reconnect) still counts
  shared      mode 3: a stand-in for ecce-broker.service, run as the unit
              runs it (ecce-broker-run --shared) from its own state
              directory, with an account from ecce-broker-setup --user, used by two
              "users" (two state directories) through siteconfig/
              SharedBroker; no quit, reap or Quit and Stop Server stops it,
              and no per-user broker is ever started
  markers     the reaper alone: a broker under a server marker survives
              --if-idle; a per-user one does not
  window      ECCE_GATEWAY_WINDOW=1 keeps the Gateway window's behaviour
  bug         `ecce --bug`: a folder with session.log, service logs and
              ecce-diagnose output, and an archive, once the session ends

ECCE_SESSION_LIVENESS=lease (or =proc) in the environment runs every case
with that evidence of liveness alone (#233, SessionLease.H).

Different Unix users cannot be run without root, so a second "user" is
a second ECCE_REALUSERHOME of the same account; the scripts key all
their state and liveness by it.

Every process of this user on the test displays running an ECCE binary
is killed, and checked gone, between cases, so one case's leftover
window cannot hold the next one's session open.

Isolated exactly as run_tests.py is (isolate.py). `--tree <build-dir>`
tests an uninstalled change: bin/ becomes symlinks to the install with
gateway, organizer and builder taken from the build and the gateway and
broker scripts (and ecce-remote-setup) from packaging/.

    tests/apps/session_end.py [--tree build-cmake] [case ...]
"""

import argparse
import getpass
import os
import re
import shutil
import socket
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))


def treeHome(state, install, build):
    """An install whose gateway binary and gateway scripts are the tree's."""
    home = os.path.join(state, "tree-home")
    shutil.rmtree(home, ignore_errors=True)
    os.makedirs(os.path.join(home, "bin"))
    for entry in os.listdir(install):
        if entry != "bin":
            os.symlink(os.path.join(install, entry), os.path.join(home, entry))
    overrides = {"gateway": os.path.join(build, "gateway"),
                 "organizer": os.path.join(build, "organizer"),
                 "builder": os.path.join(build, "builder")}
    gwdir = os.path.join(REPO, "packaging", "gateway")
    for script in os.listdir(gwdir):
        if script.startswith("ecce-") and (script.endswith(".sh") or os.access(
                os.path.join(gwdir, script), os.X_OK)):
            overrides[script] = os.path.join(gwdir, script)
    overrides["ecce-remote-setup"] = os.path.join(
        REPO, "packaging", "dataserver", "ecce-remote-setup")
    overrides["ecce-dataserver-start"] = os.path.join(
        REPO, "packaging", "dataserver", "ecce-dataserver-start")
    overrides["ecce-dataserver-stop"] = os.path.join(
        REPO, "packaging", "dataserver", "ecce-dataserver-stop")
    overrides["ecce-dataserver-adduser"] = os.path.join(
        REPO, "packaging", "dataserver", "ecce-dataserver-adduser")
    overrides["ecce-diagnose"] = os.path.join(REPO, "packaging",
                                              "ecce-diagnose")
    #  The access rules the broker scripts read from server/ are installed;
    #  scripts this change adds are not in the install at all yet.
    entries = set(os.listdir(os.path.join(install, "bin"))) | set(overrides)
    for entry in entries:
        source = overrides.get(entry, os.path.join(install, "bin", entry))
        os.symlink(source, os.path.join(home, "bin", entry))
    return home


def parse():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("cases", nargs="*",
                        default=["organizer", "builder", "jobstore", "stop",
                                 "quit-stop",
                                 "remote", "remote-refused", "remote-down", "displays",
                                 "same-display", "two-sessions",
                                 "two-sessions-server", "two-sessions-shared",
                                 "services-killed",
                                 "display-changes", "join",
                                 "shared", "markers",
                                 "window", "bug"])
    parser.add_argument("--tree", help="build directory to take gateway from")
    parser.add_argument("--wrappers", help="directory holding the ecce "
                        "wrappers (default: <tree>/wrappers, else "
                        "ECCE_TEST_WRAPPERS or /usr/bin)")
    return parser.parse_args()


args = parse()
install = os.environ.get("ECCE_TEST_HOME", "/opt/ecce")
sys.path.insert(0, HERE)
import isolate  # noqa: E402

state = isolate.resolveStateDir(isolate.runState("apps-session"))
os.makedirs(state, exist_ok=True)
if args.tree:
    build = os.path.abspath(args.tree)
    install = treeHome(state, install, build)
    # The build writes its wrappers without the execute bit (install()
    # adds it), so run executable copies of them.
    wrappers = os.path.join(state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(args.wrappers or os.path.join(build, "wrappers"),
                    wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
else:
    build = None
    wrappers = (args.wrappers or os.environ.get("ECCE_TEST_WRAPPERS")
                or "/usr/bin")
os.environ["ECCE_TEST_HOME"] = install
os.environ.pop("ECCE_NO_REAP", None)     # the reaper is under test

import apps      # noqa: E402  (reads ECCE_TEST_HOME at import)
import fixture   # noqa: E402
import sessionkey  # noqa: E402
import xdisplay  # noqa: E402

try:
    from Xlib import X, display as xlibdisplay, protocol
except ImportError:
    X = None


def say(text):
    print(text, flush=True)


# --- the process table ---------------------------------------------------

SESSION_ID = re.compile(r"^[0-9a-f]{16}$")


def procs(where):
    """{pid: exe} of this user's processes in a session or on a display.

    where is a session id (16 lower-case hex characters, #233), which is
    what a session's liveness is decided by, or a display name, which
    is what clears a test display of everything on it.
    """
    var = "ECCE_SESSION_ID=" if SESSION_ID.match(where) else "DISPLAY="
    found = {}
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        try:
            if os.stat("/proc/" + entry).st_uid != os.getuid():
                continue
            exe = os.readlink("/proc/%s/exe" % entry)
            with open("/proc/%s/environ" % entry, "rb") as handle:
                env = handle.read().split(b"\0")
        except OSError:
            continue
        if (var + where).encode() in env:
            found[int(entry)] = exe
    return found


def ancestry(pid):
    """pid and its ancestors, up to init."""
    chain = []
    while pid > 1:
        chain.append(pid)
        try:
            with open("/proc/%d/stat" % pid) as handle:
                pid = int(handle.read().rsplit(")", 1)[1].split()[1])
        except (OSError, ValueError, IndexError):
            break
    return chain


def environOf(pid):
    try:
        with open("/proc/%d/environ" % pid, "rb") as handle:
            pairs = handle.read().split(b"\0")
    except OSError:
        return {}
    return dict(p.decode(errors="replace").split("=", 1)
                for p in pairs if b"=" in p)


def named(displayName, name):
    return [pid for pid, exe in procs(displayName).items()
            if os.path.basename(exe).split(" ")[0] == name]


def sessionProcs(displayName, keep=("eccejobstore",)):
    """{pid: name} of the session's apps still running."""
    found = {}
    for pid, exe in procs(displayName).items():
        exe = exe.replace(" (deleted)", "")
        name = os.path.basename(exe)
        link = os.path.join(install, "bin", name)
        if (name not in keep and os.path.exists(link)
                and os.path.realpath(link) == exe):
            found[pid] = name
    return found


def eccePids(displayName):
    """{pid: exe} of this user's ECCE binaries on displayName.

    An exe counts if $ECCE_HOME/bin/<name> resolves to it, which also
    catches apps the gateway or the dispatcher spawned outside the
    `ecce` process group -- the ones killpg cannot reach.
    """
    bindir = os.path.join(os.environ.get("ECCE_HOME", install), "bin")
    found = {}
    for pid, exe in procs(displayName).items():
        exe = exe.replace(" (deleted)", "")
        link = os.path.join(bindir, os.path.basename(exe))
        if os.path.lexists(link) and os.path.realpath(link) == exe:
            found[pid] = exe
    return found


def clearDisplay(displayName, statedirs=None):
    """Kill every ECCE process on displayName; True once none is.

    Verified by re-reading the process table, not by sleeping: SIGTERM,
    then SIGKILL for whatever is left after 10s.
    """
    def victims():
        pids = dict(eccePids(displayName))
        for pid, name in sessionProcs(displayName, keep=()).items():
            pids.setdefault(pid, name)
        return {pid: exe for pid, exe in pids.items() if alive(pid)}

    for sig, wait in ((15, 10), (9, 10)):
        for pid in victims():
            try:
                os.kill(pid, sig)
            except OSError:
                pass
        deadline = time.time() + wait
        while victims() and time.time() < deadline:
            time.sleep(0.2)
        if not victims():
            break
    left = victims()
    if left:
        say("    (still running on %s: %s)" % (displayName, left))
        return False
    return True


def alive(pid):
    try:
        os.kill(pid, 0)
        with open("/proc/%d/stat" % pid) as handle:
            return handle.read().rsplit(")", 1)[1].split()[0] != "Z"
    except OSError:
        return False


def pidfile(path):
    try:
        with open(path) as handle:
            return int(handle.read().strip())
    except (OSError, ValueError):
        return None


def statedir():
    return os.path.join(os.environ["ECCE_REALUSERHOME"], ".ECCE")


def broker():
    """Pid of this user's mosquitto, or None."""
    return pidfile(os.path.join(statedir(), "mosquitto.pid"))


def brokerSocket():
    return os.path.join(statedir(), "mosquitto.sock")


def portOpen(port):
    with socket.socket() as sock:
        sock.settimeout(0.5)
        return sock.connect_ex(("127.0.0.1", port)) == 0


def brokerPort():
    return int(os.environ["ECCE_BROKER_PORT"])


def brokerStopped(checks, amq, why):
    checks.check(amq is not None and not alive(amq)
                 and not os.path.exists(brokerSocket()),
                 "the per-user broker %s was stopped: %s" % (amq, why))
    checks.check(broker() is None, "and its pidfile removed")


def brokersOf(statedirs):
    """Pids of mosquitto brokers started from one of statedirs."""
    found = []
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        try:
            with open("/proc/%s/cmdline" % entry, "rb") as handle:
                argv = handle.read().split(b"\0")
        except OSError:
            continue
        for base in statedirs:
            want = os.path.join(base, ".ECCE", "mosquitto.conf").encode()
            if want in argv:
                found.append(int(entry))
    return found


def connectedTo(port):
    """Pids with an established TCP connection to localhost:port."""
    out = subprocess.run(["ss", "-Htnp", "state", "established",
                          "( dport = :%d )" % port],
                         stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    return set(int(p) for p in re.findall(r"pid=(\d+)",
                                          out.stdout.decode()))


def run(script, env, *argv):
    return subprocess.run([os.path.join(install, "bin", script)] + list(argv),
                          env=env, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, timeout=180)


def stopOwnBroker(env):
    """Stop this state's per-user broker, as Quit and Stop Server would."""
    run("ecce-gateway-reap", env, "--stop")


# --- windows ---------------------------------------------------------------

def waitWindow(display, pattern, timeout=60, gone=False):
    deadline = time.time() + timeout
    while time.time() < deadline:
        match = [w for w in display.windows() if pattern in w[1]]
        if bool(match) != gone:
            return match[0] if match else True
        time.sleep(0.5)
    return None


def closeWindow(display, wid):
    """What a window manager's close button does: WM_DELETE_WINDOW."""
    conn = xlibdisplay.Display(display.name)
    window = conn.create_resource_object("window", int(wid, 16))
    protocols = conn.intern_atom("WM_PROTOCOLS")
    delete = conn.intern_atom("WM_DELETE_WINDOW")
    event = protocol.event.ClientMessage(
        window=window, client_type=protocols,
        data=(32, [delete, X.CurrentTime, 0, 0, 0]))
    window.send_event(event, event_mask=X.NoEventMask)
    conn.flush()
    conn.close()


def windowPid(display, wid):
    """The _NET_WM_PID GTK sets on a top-level window, or None."""
    try:
        conn = xlibdisplay.Display(display.name)
    except Exception:
        return None
    try:
        window = conn.create_resource_object("window", int(wid, 16))
        prop = window.get_full_property(conn.intern_atom("_NET_WM_PID"),
                                        X.AnyPropertyType)
        return int(prop.value[0]) if prop is not None and len(prop.value) \
            else None
    except Exception:
        return None
    finally:
        conn.close()


def pressReturn(display, wid):
    env = display.env()
    subprocess.run(["xdotool", "windowfocus", str(int(wid, 16))],
                   env=env, timeout=10, stderr=subprocess.DEVNULL)
    time.sleep(0.3)
    subprocess.run(["xdotool", "key", "Return"], env=env, timeout=10)


# Modal dialogs a user would answer on the way out: while one is up, wx
# ignores the frame's close. The Builder's layout notice appears whenever
# wxbuilder.ini was last written by another ECCE version, and is never
# written back if the Builder is killed, so it recurs until answered.
ACCEPT = ("Quit ECCE", "Reset Default Tools/Toolbars")


def quitVia(display, frame):
    """Close a frame and accept its 'Quit ECCE' confirmation, if any.

    Retried: without a window manager a dialog can be listed before it
    is viewable, and a focus request on it then fails silently.
    """
    time.sleep(3)          # nobody closes a window the instant it maps
    deadline = time.time() + 40
    lastClose = 0
    while time.time() < deadline:
        windows = display.windows()
        if frame[0] not in [w for w, _ in windows]:
            return True
        dialog = next((w for w in windows if w[1] in ACCEPT), None)
        if dialog:
            time.sleep(0.5)
            pressReturn(display, dialog[0])
        elif time.time() - lastClose > 8:
            closeWindow(display, frame[0])
            lastClose = time.time()
        time.sleep(1)
    say("    (%s did not close; windows: %s)"
        % (frame[1], [t for _, t in display.windows() if t]))
    for pid, exe in procs(display.name).items():
        try:
            with open("/proc/%d/wchan" % pid) as handle:
                wchan = handle.read()
            with open("/proc/%d/stat" % pid) as handle:
                st = handle.read().rsplit(")", 1)[1].split()[0]
        except OSError:
            continue
        say("    (  %d %s %s %s)" % (pid, os.path.basename(exe), st, wchan))
    return False


# --- one session -------------------------------------------------------

class Session(object):
    def __init__(self, display, log, argv=(), extra=None, pipe=False,
                 prefix=()):
        self.display = display
        self.extra = extra or {}
        self.log = open(log, "w")
        self._sid = None
        self.proc = subprocess.Popen(
            list(prefix) + [os.path.join(wrappers, "ecce")] + list(argv),
            env=self.env(),
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE if pipe else self.log,
            stderr=subprocess.STDOUT, start_new_session=True)

    def gateway(self):
        """Pid of the gateway this `ecce` started, or None."""
        for pid, exe in procs(self._sid or self.display.name).items():
            if (os.path.basename(exe).split(" ")[0] == "gateway"
                    and self.proc.pid in ancestry(pid)):
                return pid
        return None

    def sid(self, timeout=60):
        """The session id `ecce` made (#233), read from its gateway.

        `ecce` always makes a fresh one, so the id passed in by the
        display's environment is not it.
        """
        deadline = time.time() + timeout
        while self._sid is None:
            gw = self.gateway()
            sid = environOf(gw).get("ECCE_SESSION_ID") if gw else None
            if sid and SESSION_ID.match(sid):
                self._sid = sid
            elif time.time() >= deadline:
                break
            else:
                time.sleep(0.3)
        return self._sid

    def mine(self, windows):
        """Of (wid, title) pairs, those of this session's processes, so two
        sessions on one display are told apart. Empty until the session
        id is known."""
        sid = self.sid(timeout=0)
        if not sid:
            return []
        found = []
        for w in windows:
            pid = windowPid(self.display, w[0])
            if pid is None or environOf(pid).get("ECCE_SESSION_ID") == sid:
                found.append(w)
        return found

    def env(self):
        """The session's environment, as an app the gateway starts has it."""
        env = self.display.env()
        env.update(self.extra)
        env["PATH"] = wrappers + os.pathsep + env.get("PATH", "")
        if self._sid:
            env["ECCE_SESSION_ID"] = self._sid
        return env

    def organizer(self, title="Organizer"):
        # ensureRealUserAccount()'s password; the gateway asks for it first.
        deadline = time.time() + 90
        frame = None
        while time.time() < deadline and not frame:
            titles = self.display.windows()
            frame = next((w for w in self.mine(titles)
                          if (title(w[1]) if callable(title)
                              else title in w[1])), None)
            auth = next((w for w in titles
                         if w[1] == "ECCE Authentication"), None)
            if auth and not frame:
                env = self.display.env()
                subprocess.run(["xdotool", "windowfocus",
                                str(int(auth[0], 16))], env=env, timeout=10,
                               stderr=subprocess.DEVNULL)
                time.sleep(0.3)
                subprocess.run(["xdotool", "type", "--delay", "50", "ecce"],
                               env=env, timeout=10)
                subprocess.run(["xdotool", "key", "Return"], env=env,
                               timeout=10)
                time.sleep(3)
            time.sleep(0.5)
        if not frame:
            say("    windows: %s" % [t for _, t in self.display.windows()
                                     if t])
            ps = subprocess.run(["ps", "-o", "pid,stat,etime,args", "-g",
                                 str(self.proc.pid)], stdout=subprocess.PIPE)
            say("    " + ps.stdout.decode().replace("\n", "\n    "))
        return frame

    def ended(self, timeout):
        try:
            self.proc.wait(timeout=timeout)
            return True
        except subprocess.TimeoutExpired:
            return False

    def kill(self):
        if self.proc.poll() is None:
            try:
                os.killpg(self.proc.pid, 15)
            except OSError:
                pass
            self.proc.wait(timeout=20)


class Checks(object):
    def __init__(self):
        self.failed = []

    def check(self, ok, what):
        say("    %s  %s" % ("ok  " if ok else "FAIL", what))
        if not ok:
            self.failed.append(what)
        return ok


def gatewayIsTheTree(checks, session):
    sid = session.sid()
    pids = named(sid, "gateway") if sid else []
    if not checks.check(len(pids) == 1, "one gateway in session %s on %s (%s)"
                        % (sid, session.display.name, pids)):
        return None
    exe = os.readlink("/proc/%d/exe" % pids[0])
    if build:
        checks.check(os.path.samefile(exe, os.path.join(build, "gateway")),
                     "the gateway running is the tree's: %s" % exe)
    return pids[0]


def endsCleanly(checks, session, d, gw, t0, apps=True):
    returned = session.ended(30)
    checks.check(returned, "`ecce` returned (%.1fs after the last close)"
                 % (time.time() - t0))
    if not returned:
        return
    time.sleep(1)
    checks.check(not alive(gw), "gateway %d gone" % gw)
    left = sessionProcs(session.sid(timeout=0) or d)
    if not apps:
        #  The stop case ends the session from a script while the
        #  Organizer is still up; CalcMgr has Destroy()ed it by then.
        left = {p: n for p, n in left.items() if n != "organizer"}
    checks.check(not left, "no gateway or app left on %s %s"
                 % (d, left))


def caseOrganizer(checks, display, logdir):
    d = display.name
    session = Session(display, os.path.join(logdir, "organizer.log"))
    try:
        frame = session.organizer()
        if not checks.check(frame, "the Organizer opened"):
            return
        gw = gatewayIsTheTree(checks, session)
        amq = broker()
        checks.check(amq and alive(amq), "broker running (%s)" % amq)
        checks.check(os.path.exists(brokerSocket()), "on its socket")
        t0 = time.time()
        quitVia(display, frame)
        endsCleanly(checks, session, d, gw, t0)
        brokerStopped(checks, amq, "the user's last session ended "
                      "(mode 1, #191)")
        checks.check(portOpen(fixture.dataserverPort()),
                     "data server left running (a plain quit, #97)")
    finally:
        session.kill()


def caseBuilder(checks, display, logdir):
    d = display.name
    session = Session(display, os.path.join(logdir, "builder.log"))
    builder = None
    try:
        frame = session.organizer()
        if not checks.check(frame, "the Organizer opened"):
            return
        gw = gatewayIsTheTree(checks, session)
        # Through the gateway, as File > New Structure does.
        builder = subprocess.Popen(
            [os.path.join(wrappers, "ecce-builder")], env=session.env(),
            stdout=session.log, stderr=subprocess.STDOUT,
            start_new_session=True)
        bframe = waitWindow(display, "Builder", timeout=90)
        if not checks.check(bframe, "a Builder opened"):
            return
        quitVia(display, frame)
        waitWindow(display, "Organizer", timeout=15, gone=True)
        checks.check(not session.ended(8),
                     "closing the Organizer alone did not end the session")
        checks.check(alive(gw), "gateway still up while Builder is")
        t0 = time.time()
        quitVia(display, bframe)
        try:
            builder.wait(timeout=30)
        except subprocess.TimeoutExpired:
            pass
        endsCleanly(checks, session, d, gw, t0)
    finally:
        if builder is not None and builder.poll() is None:
            os.killpg(builder.pid, 15)
            builder.wait(timeout=20)
        session.kill()


def caseJobstore(checks, display, logdir):
    """A running job monitor must neither hold the session nor die with it.

    The stand-in is `sleep` under the name eccejobstore in bin/, started
    the way Launch.C starts the real one: nohup'd, from the session.
    """
    d = display.name
    fake = os.path.join(install, "bin", "eccejobstore")
    if not args.tree:
        say("    skip  needs --tree (it replaces bin/eccejobstore)")
        return
    original = os.readlink(fake)
    os.unlink(fake)
    shutil.copy("/bin/sleep", fake)
    session = Session(display, os.path.join(logdir, "jobstore.log"))
    job = None
    try:
        frame = session.organizer()
        if not checks.check(frame, "the Organizer opened"):
            return
        gw = gatewayIsTheTree(checks, session)
        #  `sleep` takes no session lease (SessionLease.H) as the real job
        #  store does, so flock holds one for it.
        lease = os.path.join(statedir(), "leases",
                             sessionkey.key(session.sid()))
        os.makedirs(lease, exist_ok=True)
        job = subprocess.Popen(["nohup", "flock", os.path.join(
                                   lease, "eccejobstore.standin"),
                                fake, "600"], env=session.env(),
                               stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL,
                               start_new_session=True)
        time.sleep(1)
        sid = session.sid()
        bfile = sessionkey.brokerFile(statedir(), sid)
        afile = sessionkey.authFile(statedir(), sid)
        hadAuth = os.path.exists(afile)
        t0 = time.time()
        quitVia(display, frame)
        returned = session.ended(30)
        checks.check(returned, "`ecce` returned with a job being monitored "
                     "(%.1fs)" % (time.time() - t0))
        checks.check(not alive(gw), "gateway gone")
        checks.check(job.poll() is None, "the job monitor survived")
        amq = broker()
        checks.check(amq is not None and alive(amq),
                     "broker kept for the job monitor (reaper's rule)")
        #  Decision 6 of #233: the job store keeps its session's id, so
        #  its session's broker file and credential stay while it runs.
        run("ecce-gateway-reap", display.env())
        checks.check(os.path.exists(bfile),
                     "its session's broker file kept after a sweep (%s)"
                     % os.path.basename(bfile))
        checks.check(not hadAuth or os.path.exists(afile),
                     "its session's credential kept after a sweep%s"
                     % ("" if hadAuth else " (none was stored)"))
        os.killpg(job.pid, 9)
        job.wait()
        job = None
        said = run("ecce-gateway-reap", display.env(),
                   "--if-idle").stdout.decode().strip()
        checks.check(not alive(amq) and not os.path.exists(bfile)
                     and not os.path.exists(afile),
                     "the job monitor ending stops the broker and removes "
                     "its session's files (%s)" % said.replace("\n", " / "))
    finally:
        if job is not None:
            os.killpg(job.pid, 9)
            job.wait()
        session.kill()
        os.unlink(fake)
        os.symlink(original, fake)
        subprocess.run([os.path.join(install, "bin", "ecce-gateway-reap"),
                        "--stop"],
                       env=display.env(), stdout=subprocess.DEVNULL)


def caseStop(checks, display, logdir):
    """Quit and Stop Server: what CalcMgr::confirmAndQuit() runs, in its
    order, from inside the session. The broker must go this time."""
    d = display.name
    session = Session(display, os.path.join(logdir, "stop.log"))
    try:
        frame = session.organizer()
        if not checks.check(frame, "the Organizer opened"):
            return
        gw = gatewayIsTheTree(checks, session)
        amq = broker()
        checks.check(amq and alive(amq), "broker running (%s)" % amq)
        t0 = time.time()
        for script in ("ecce-dataserver-stop", "ecce-gateway-stop"):
            subprocess.run([os.path.join(install, "bin", script)],
                           env=session.env(), stdout=session.log,
                           stderr=subprocess.STDOUT, timeout=120)
        endsCleanly(checks, session, d, gw, t0, apps=False)
        checks.check(amq is not None and not alive(amq)
                     and not os.path.exists(brokerSocket()),
                     "the broker %s was stopped, as asked" % amq)
        checks.check(broker() is None, "and its pidfile removed")
        checks.check(not portOpen(fixture.dataserverPort()),
                     "the data server was stopped, as asked")
    finally:
        session.kill()


class StreamWatch(object):
    """What arrives on a session's output, and when, until every writer
    has closed it -- as a terminal would see it."""

    def __init__(self, session):
        import threading
        self.chunks = []
        self.exited = None
        self.eof = None
        self.session = session

        def read():
            fd = session.proc.stdout.fileno()
            while True:
                data = os.read(fd, 4096)
                if not data:
                    break
                self.chunks.append((time.time(), data))
                session.log.write(data.decode(errors="replace"))
                session.log.flush()
            self.eof = time.time()

        def wait():
            session.proc.wait()
            self.exited = time.time()

        self.reader = threading.Thread(target=read, daemon=True)
        self.waiter = threading.Thread(target=wait, daemon=True)
        self.reader.start()
        self.waiter.start()

    def text(self, after=None):
        return b"".join(d for t, d in self.chunks
                        if after is None or t > after).decode(errors="replace")


def clickLastButton(display, wid):
    """Click a message dialog's right-most button (ewxMessageDialog lays
    its buttons out right-aligned, in the order they were added)."""
    env = display.env()
    geo = subprocess.run(["xdotool", "getwindowgeometry", "--shell",
                          str(int(wid, 16))], env=env, timeout=10,
                         stdout=subprocess.PIPE).stdout.decode()
    size = dict(l.split("=") for l in geo.split() if "=" in l)
    x, y = int(size["WIDTH"]) - 30, int(size["HEIGHT"]) - 22
    subprocess.run(["xdotool", "mousemove", "--window", str(int(wid, 16)),
                    str(x), str(y), "click", "1"], env=env, timeout=10)


def caseQuitStop(checks, display, logdir):
    """The Organizer's Quit and Stop Server, clicked: everything the
    teardown prints reaches `ecce`'s output before `ecce` returns, so the
    shell prompt is not followed by stray lines."""
    d = display.name
    session = Session(display, os.path.join(logdir, "quit-stop.log"),
                      pipe=True)
    watch = StreamWatch(session)
    try:
        frame = session.organizer()
        if not checks.check(frame, "the Organizer opened"):
            return
        gw = gatewayIsTheTree(checks, session)
        amq = broker()
        time.sleep(3)
        dialog = None
        deadline = time.time() + 40
        while not dialog and time.time() < deadline:
            closeWindow(display, frame[0])
            dialog = waitWindow(display, "Quit ECCE", timeout=8)
        if not checks.check(dialog, "the Quit ECCE dialog opened"):
            return
        time.sleep(1)
        clickLastButton(display, dialog[0])
        watch.waiter.join(60)
        if not checks.check(watch.exited is not None,
                            "`ecce` returned after Quit and Stop Server"):
            return
        watch.reader.join(30)
        checks.check(watch.eof is not None, "its output was closed %.1fs "
                     "after it returned" % ((watch.eof or time.time())
                                            - watch.exited))
        before = watch.text()
        late = watch.text(after=watch.exited + 0.1)
        checks.check("stopping mosquitto broker" in before,
                     "the teardown's messages were printed")
        if not checks.check(not late, "nothing printed after `ecce` "
                            "returned: %r" % late):
            for t, data in watch.chunks:
                say("    %+.2fs %r" % (t - watch.exited, data))
        checks.check(amq is not None and not alive(amq),
                     "the broker %s was stopped" % amq)
        checks.check(not portOpen(fixture.dataserverPort()),
                     "the data server was stopped")
        checks.check(not alive(gw or -1), "gateway gone")
    finally:
        session.kill()


def caseRemote(checks, display, logdir):
    """#167's single-machine recipe: a client quitting under -remote.

    This run's own services are the central server; the client has a
    state directory, an ECCE_HOME configured by the real
    ecce-remote-setup, and a display of its own. The server's side is
    what `ecce-remote-setup --server` and starting ECCE there give: its
    mosquitto on TCP, with the accounts of ecce-dataserver-adduser.
    """
    serverEnv = display.env()
    stopOwnBroker(serverEnv)
    marker = os.path.join(statedir(), "mosquitto.server")
    mark = run("ecce-remote-setup", serverEnv, "--server")
    if not checks.check(mark.returncode == 0 and os.path.exists(marker),
                        "the server account marked (ecce-remote-setup "
                        "--server)"):
        say(mark.stdout.decode())
        return
    start = run("ecce-gateway-start", serverEnv)
    amq = broker()
    dport = fixture.dataserverPort()
    bport = int(os.environ["ECCE_BROKER_PORT"])
    if not checks.check(amq and alive(amq) and portOpen(bport),
                        "the server's broker %s answers on TCP port %d"
                        % (amq, bport)):
        say(start.stdout.decode())
        return
    try:
        _remoteClient(checks, display, logdir, serverEnv, amq, dport, bport,
                      marker)
    finally:
        try:
            os.unlink(marker)
        except OSError:
            pass
        stopOwnBroker(serverEnv)


def _remoteClient(checks, display, logdir, serverEnv, amq, dport, bport,
                  marker):
    client = os.path.join(state, "client")
    os.makedirs(os.path.join(client, ".ECCE"), exist_ok=True)
    chome = isolate.homeOverlay(apps.INSTALL, client, dport)
    extra = {"ECCE_REALUSERHOME": client, "ECCE_HOME": chome}
    # homeOverlay copies (not symlinks) siteconfig, so this is safe to
    # edit: make the client's machine list differ from the server's, so a
    # successful copy by ecce-remote-setup (#188) is visible below rather
    # than the two starting out identical by construction.
    clientMachines = os.path.join(chome, "siteconfig", "Machines")
    with open(clientMachines, "w"):
        pass
    # An install that was itself once made a client carries its backup,
    # which ecce-remote-setup keeps rather than overwrites.
    shutil.rmtree(os.path.join(chome, "siteconfig", "local-machines.orig"),
                  ignore_errors=True)
    setup = subprocess.run(
        [os.path.join(install, "bin", "ecce-remote-setup"), "localhost",
         str(dport)], env=dict(os.environ, **extra),
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if not checks.check(setup.returncode == 0, "ecce-remote-setup ran"):
        say(setup.stdout.decode())
        return
    setupOut = setup.stdout.decode()
    servedMachines = os.path.join(statedir(), "dataserver", "htdocs", "Ecce",
                                  "system", "siteconfig", "Machines")
    try:
        with open(servedMachines) as f:
            servedContent = f.read()
    except OSError:
        servedContent = None
    with open(clientMachines) as f:
        clientContent = f.read()
    checks.check(servedContent is not None and clientContent == servedContent,
                 "ecce-remote-setup copied the server's Machines (#188)")
    backupMachines = os.path.join(chome, "siteconfig", "local-machines.orig",
                                  "Machines")
    try:
        with open(backupMachines) as f:
            backupContent = f.read()
    except OSError:
        backupContent = None
    checks.check(backupContent == "",
                 "the client's original (truncated) Machines was backed up "
                 "to local-machines.orig")
    checks.check("Copied the server's machine list" in setupOut,
                 "ecce-remote-setup reported the copy")
    # A new student account: nothing registered, no Queues file (#188).
    myMachines = os.path.join(client, ".ECCE", "MyMachines")
    for leftover in ("MyMachines", "Queues"):
        try:
            os.unlink(os.path.join(client, ".ECCE", leftover))
        except OSError:
            pass
    accessLog = os.path.join(statedir(), "dataserver", "logs", "access_log")
    logStart = os.path.getsize(accessLog) if os.path.exists(accessLog) else 0
    cdisplay = xdisplay.Display().__enter__()
    session = None
    try:
        session = Session(cdisplay, os.path.join(logdir, "remote.log"),
                          ["-remote"], extra)
        frame = session.organizer()
        if not checks.check(frame, "the client's Organizer opened"):
            return
        checks.check(waitWindow(cdisplay, " on localhost", 10),
                     "the client's Organizer names its server in the title")
        with open(accessLog, errors="replace") as f:
            f.seek(logStart)
            served = [l for l in f if "PROPFIND" in l and " 207 " in l]
        checks.check(served, "the central data server answered the client "
                     "(%d PROPFIND 207 in its access log)" % len(served))
        host = socket.gethostname().split(".")[0]
        try:
            with open(myMachines) as f:
                registered = any(l.split("\t")[0] in (host, socket.getfqdn())
                                 for l in f)
        except OSError:
            registered = False
        checks.check(registered, "the client's own machine %s was registered "
                     "in its MyMachines" % host)
        cd = cdisplay.name
        checks.check(not os.path.exists(os.path.join(client, ".ECCE",
                                                     "mosquitto.pid")),
                     "no broker of the client's own")
        gw = gatewayIsTheTree(checks, session)
        # The gateway is logged in to the server's broker: anonymous
        # clients are refused on TCP, so a connection means its account.
        checks.check(gw in connectedTo(bport),
                     "the client's gateway is connected to the server's "
                     "broker with its data server login (connected: %s)"
                     % sorted(connectedTo(bport)))
        # The teacher's session ends: its reaper runs, exactly as the
        # gateway wrapper's EXIT trap does it, and leaves the marked broker.
        reap = subprocess.run(
            [os.path.join(install, "bin", "ecce-gateway-reap"), "--if-idle"],
            env=serverEnv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        said = reap.stdout.decode().strip().replace("\n", " / ")
        checks.check(alive(amq) and "broker" not in said,
                     "the marked server's own plain quit left the broker "
                     "the client uses (reaper: %s)" % said)
        t0 = time.time()
        quitVia(cdisplay, frame)
        endsCleanly(checks, session, cd, gw or -1, t0)
        checks.check(alive(amq), "the client quitting left the server's "
                     "broker running")
        checks.check(portOpen(dport) and portOpen(bport),
                     "the server's data server and broker still answer")
        #  The client never offers Quit and Stop Server (#190); its stop
        #  scripts, run anyway, reach only its own state directory.
        for script in (("ecce-dataserver-stop", "--if-unused"),
                       ("ecce-gateway-stop",)):
            run(script[0], session.env(), *script[1:])
        checks.check(alive(amq) and portOpen(dport) and portOpen(bport),
                     "the client's stop scripts left the server's data "
                     "server and broker running")
    finally:
        if session is not None:
            session.kill()
        checks.check(clearDisplay(cdisplay.name,
                                  [os.path.join(client, ".ECCE")]),
                     "nothing left on the client's display %s"
                     % cdisplay.name)
        stopEnv = session.env() if session is not None else dict(cdisplay.env(), **extra)
        subprocess.run([os.path.join(install, "bin", "ecce-gateway-stop")],
                       env=stopEnv, stdout=subprocess.DEVNULL)
        cdisplay.__exit__(None, None, None)


def caseRemoteRefused(checks, display, logdir):
    """A login the message broker does not accept: the user gets a dialog
    naming the account, not silence on a stderr nobody reads.

    A central server's broker takes the data server's own users file, so
    it cannot disagree with the data server; the shared broker (mode 3)
    has an account list of its own, and here it lacks this account.
    """
    env = display.env()
    stopOwnBroker(env)
    sport = isolate._pickPort("ECCE_TEST_SHARED_BROKER_PORT",
                              brokerPort() + 100)
    decl = os.path.join(os.environ["ECCE_HOME"], "siteconfig",
                        "SharedBroker")
    accounts = os.path.join(os.environ["ECCE_HOME"], "siteconfig",
                            "SharedBroker.passwd")
    setup = run("ecce-broker-setup", env, "localhost:%d" % sport)
    if not checks.check(setup.returncode == 0 and os.path.exists(decl),
                        "ecce-broker-setup declared localhost:%d" % sport):
        return
    subprocess.run(
        [os.path.join(install, "bin", "ecce-broker-setup"), "--user",
         "someoneelse"], env=env, input=b"pw\n",
        stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    serviceLog = open(os.path.join(logdir, "refused-service.log"), "w")
    service = subprocess.Popen(
        [os.path.join(install, "bin", "ecce-broker-run"), "--shared",
         os.path.join(state, "service", "ecce-broker")],
        env={"PATH": os.environ["PATH"],
             "ECCE_HOME": os.environ["ECCE_HOME"]},
        cwd="/", stdin=subprocess.DEVNULL, stdout=serviceLog,
        stderr=subprocess.STDOUT, start_new_session=True)
    session = None
    account = fixture.realUser()
    try:
        deadline = time.time() + 60
        while not portOpen(sport) and time.time() < deadline:
            time.sleep(0.5)
        if not checks.check(portOpen(sport) and service.poll() is None,
                            "the stand-in service answers on %d" % sport):
            return
        log = os.path.join(logdir, "remote-refused.log")
        session = Session(display, log)
        session.organizer()    # answers the login dialog on the way
        deadline = time.time() + 90
        said = ""
        dialog = None
        while time.time() < deadline and not dialog:
            with open(log, errors="replace") as handle:
                said = handle.read()
            dialog = next((w for w in display.windows()
                           if w[1] == "Message broker refused the login"),
                          None)
            time.sleep(0.5)
        checks.check(dialog is not None,
                     "a dialog says the message broker refused the login")
        checks.check("refused the connection" in said
                     and "'%s'" % account in said,
                     "and stderr names the account too")
    finally:
        if session is not None:
            session.kill()
        checks.check(clearDisplay(display.name, [os.path.join(
            os.environ["ECCE_REALUSERHOME"], ".ECCE")]),
            "nothing left on %s" % display.name)
        run("ecce-broker-setup", env, "--remove")
        service.terminate()
        try:
            service.wait(timeout=30)
        except subprocess.TimeoutExpired:
            service.kill()
            service.wait()
        serviceLog.close()
        try:
            os.unlink(accounts)
        except OSError:
            pass


def caseRemoteDown(checks, display, logdir):
    """`ecce -remote` with the central server down: a message naming the
    server and its ports, a non-zero exit, and no gateway left to abort
    on a missing broker."""
    client = os.path.join(state, "client-down")
    shutil.rmtree(client, ignore_errors=True)
    os.makedirs(os.path.join(client, ".ECCE"))
    dport = isolate._pickPort("ECCE_TEST_DOWN_DATA_PORT", 8390)
    bport = isolate._pickPort("ECCE_TEST_DOWN_BROKER_PORT", dport + 10)
    chome = isolate.homeOverlay(apps.INSTALL, client, dport)
    extra = {"ECCE_REALUSERHOME": client, "ECCE_HOME": chome,
             "ECCE_BROKER_PORT": str(bport)}
    setup = subprocess.run(
        [os.path.join(install, "bin", "ecce-remote-setup"), "localhost",
         str(dport)], env=dict(os.environ, **extra),
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if not checks.check(setup.returncode == 0, "ecce-remote-setup ran "
                        "against ports %d/%d with nothing on them"
                        % (dport, bport)):
        say(setup.stdout.decode())
        return
    log = os.path.join(logdir, "remote-down.log")
    t0 = time.time()
    session = Session(display, log, ["-remote"], extra)
    try:
        returned = session.ended(30)
        checks.check(returned, "`ecce -remote` returned (%.1fs)"
                     % (time.time() - t0))
        checks.check(returned and session.proc.returncode != 0,
                     "with a non-zero exit (%s)" % session.proc.returncode)
        with open(log, errors="replace") as handle:
            said = handle.read()
        want = ("the central ECCE server localhost is not answering on "
                "localhost:%d (broker) and localhost:%d (data server)"
                % (bport, dport))
        checks.check(want in said and "ecce-dataserver-status" in said,
                     "it named the server, both ports and what to check")
        checks.check("ASSERTION" not in said and "core dumped" not in said
                     and "did not report ready" not in said,
                     "no assertion or core dump")
        checks.check(not named(display.name, "gateway"),
                     "no gateway process left")
        if want not in said:
            say("    " + said.replace("\n", "\n    "))
    finally:
        session.kill()


def caseDisplays(checks, display, logdir):
    """Mode 1, one user on two displays: the per-user broker is the
    user's, so it outlives the first session and goes with the last."""
    other = xdisplay.Display().__enter__()
    first = second = None
    try:
        first = Session(display, os.path.join(logdir, "displays-1.log"))
        frame1 = first.organizer()
        if not checks.check(frame1, "the Organizer opened on %s"
                            % display.name):
            return
        gw1 = gatewayIsTheTree(checks, first)
        second = Session(other, os.path.join(logdir, "displays-2.log"))
        frame2 = second.organizer()
        if not checks.check(frame2, "a second Organizer opened on %s"
                            % other.name):
            return
        gw2 = gatewayIsTheTree(checks, second)
        amq = broker()
        checks.check(amq and alive(amq), "one broker for both (%s)" % amq)
        t0 = time.time()
        quitVia(display, frame1)
        endsCleanly(checks, first, display.name, gw1 or -1, t0)
        checks.check(amq is not None and alive(amq)
                     and os.path.exists(brokerSocket()),
                     "the broker survives the first session: the user "
                     "is still on %s" % other.name)
        t0 = time.time()
        quitVia(other, frame2)
        endsCleanly(checks, second, other.name, gw2 or -1, t0)
        brokerStopped(checks, amq, "the user's last session ended")
    finally:
        for session in (first, second):
            if session is not None:
                session.kill()
        checks.check(clearDisplay(other.name), "nothing left on %s"
                     % other.name)
        other.__exit__(None, None, None)


def caseSameDisplay(checks, display, logdir):
    """Two `ecce` on one display are two sessions (#233, decision 1):
    each has its own id and files, and either ends without the other."""
    first = second = None
    try:
        first = Session(display, os.path.join(logdir, "same-display-1.log"))
        frame1 = first.organizer()
        if not checks.check(frame1, "the first Organizer opened"):
            return
        gw1 = gatewayIsTheTree(checks, first)
        second = Session(display, os.path.join(logdir, "same-display-2.log"))
        frame2 = second.organizer()
        if not checks.check(frame2 and frame2[0] != frame1[0],
                            "a second Organizer opened on the same display"):
            return
        gw2 = gatewayIsTheTree(checks, second)
        s1, s2 = first.sid(), second.sid()
        checks.check(s1 and s2 and s1 != s2, "two session ids (%s, %s)"
                     % (s1, s2))
        files = {}
        for sid in (s1, s2):
            files[sid] = (sessionkey.brokerFile(statedir(), sid),
                          sessionkey.authFile(statedir(), sid))
            checks.check(os.path.exists(files[sid][0]),
                         "session %s has its own broker file" % sid)
        amq = broker()
        checks.check(amq and alive(amq), "one broker for both (%s)" % amq)
        t0 = time.time()
        quitVia(display, frame1)
        endsCleanly(checks, first, display.name, gw1 or -1, t0)
        checks.check(alive(gw2 or -1) and not second.ended(3),
                     "the second session is still up")
        checks.check(display.windows() and waitWindow(
            display, "Organizer", timeout=5), "its Organizer still open")
        hadAuth2 = os.path.exists(files[s2][1])
        run("ecce-gateway-reap", display.env())
        checks.check(os.path.exists(files[s2][0])
                     and (not hadAuth2 or os.path.exists(files[s2][1])),
                     "the first session's end and a sweep left the second's "
                     "files")
        checks.check(not os.path.exists(files[s1][1]),
                     "the first session's credential is gone")
        checks.check(amq is not None and alive(amq),
                     "the broker survives the first session")
        t0 = time.time()
        quitVia(display, frame2)
        endsCleanly(checks, second, display.name, gw2 or -1, t0)
        brokerStopped(checks, amq, "the user's last session ended")
    finally:
        for session in (first, second):
            if session is not None:
                session.kill()


HOOK = "ECCE_TEST_ORGANIZER"
CRASH = re.compile(r"Segmentation fault|ASSERTION|ended by SIG|core dumped")


class Probe(object):
    """Commands to an Organizer through its ECCE_TEST_ORGANIZER hook
    (CalcMgr::runTestCommand); the answers arrive in the session's log."""

    def __init__(self, name):
        self.path = os.path.join(state, "%s.commands" % name)
        open(self.path, "w").close()
        self.extra = {HOOK: self.path}

    def ask(self, session, command, timeout=90, pid=None):
        pattern = re.compile(r"%s: %s: (.*)" % (HOOK, re.escape(command)))
        before = len(pattern.findall(self.logText(session)))
        with open(self.path, "a") as handle:
            handle.write(command + "\n")
        deadline = time.time() + timeout
        while time.time() < deadline:
            found = pattern.findall(self.logText(session))
            if len(found) > before:
                return found[-1].strip()
            if pid is not None and not alive(pid):
                return None
            time.sleep(0.5)
        return None

    @staticmethod
    def logText(session):
        session.log.flush()
        with open(session.log.name, errors="replace") as handle:
            return handle.read()


def organizerPid(session):
    sid = session.sid(timeout=0)
    pids = named(sid, "organizer") if sid else []
    return pids[0] if pids else None


def twoSessions(checks, display, logdir, tag, brokerUp, stopSays):
    """The report on 9.0.0-alpha.6-rc2 (#233): two `ecce` on one display;
    the second ends with Quit and Stop Server; the first, still browsing,
    must keep its broker and data server and must not crash.

    brokerUp() says whether the broker the sessions use is up; stopSays is
    what the second session's stop must print about the broker."""
    p1, p2 = Probe(tag + "-1"), Probe(tag + "-2")
    first = second = None
    try:
        first = Session(display, os.path.join(logdir, tag + "-1.log"),
                        extra=p1.extra)
        frame1 = first.organizer()
        if not checks.check(frame1, "the first Organizer opened"):
            return
        gw1 = gatewayIsTheTree(checks, first)
        org1 = organizerPid(first)
        checks.check(p1.ask(first, "browse") == "ok",
                     "the first Organizer browses the data server")
        second = Session(display, os.path.join(logdir, tag + "-2.log"),
                         extra=p2.extra)
        frame2 = second.organizer()
        if not checks.check(frame2 and frame2[0] != frame1[0],
                            "a second, independent Organizer opened"):
            return
        gw2 = gatewayIsTheTree(checks, second)
        offer = p2.ask(second, "quit-offer")
        checks.check(offer == "stop withheld, services in use",
                     "the second Organizer's Quit does not offer Quit and "
                     "Stop Server while the first session runs (%s)" % offer)
        amq = broker()
        mark1 = len(Probe.logText(first))
        #  What the button ran, in CalcMgr::confirmAndQuit's order, in case
        #  it was offered before the first session started.
        t0 = time.time()
        said = ""
        for script in (("ecce-dataserver-stop", "--if-unused"),
                       ("ecce-gateway-stop",)):
            said += run(script[0], second.env(), *script[1:]).stdout.decode()
        endsCleanly(checks, second, display.name, gw2 or -1, t0, apps=False)
        quitVia(display, frame2)
        lines = " / ".join(l for l in said.splitlines() if l.strip())
        checks.check("data server left running" in said,
                     "its Quit and Stop Server left the data server, saying "
                     "why (%s)" % lines)
        checks.check(stopSays in said, "and said why the broker stays "
                     "(%r expected)" % stopSays)
        checks.check(brokerUp(), "the broker is still up")
        checks.check(amq is None or alive(amq),
                     "the per-user broker %s was not restarted" % amq)
        checks.check(portOpen(fixture.dataserverPort()),
                     "the data server still answers")
        for command, want in (("browse", "ok"), ("open", "ok, state "),
                              ("publish", "sent")):
            answer = p1.ask(first, command, pid=org1)
            checks.check(answer is not None and answer.startswith(want),
                         "the first Organizer still works: %s -> %s"
                         % (command, answer))
        checks.check(org1 and alive(org1) and alive(gw1 or -1),
                     "the first session's Organizer and gateway are alive")
        log1 = Probe.logText(first)[mark1:]
        checks.check(not CRASH.search(log1), "no crash or assertion in the "
                     "first session's output")
        checks.check("starting data server" not in log1
                     and "starting mosquitto broker" not in log1,
                     "nothing in the first session had to restart a service")
        t0 = time.time()
        quitVia(display, frame1)
        endsCleanly(checks, first, display.name, gw1 or -1, t0)
    finally:
        for session in (first, second):
            if session is not None:
                session.kill()


def caseTwoSessionsStop(checks, display, logdir):
    """Mode 1, the per-user broker and data server."""
    twoSessions(checks, display, logdir, "two-sessions",
                lambda: broker() is not None and alive(broker()),
                "broker left running")


def caseTwoSessionsStopServer(checks, display, logdir):
    """Mode 2, the sessions are the central server's own account's."""
    serverEnv = display.env()
    stopOwnBroker(serverEnv)
    marker = os.path.join(statedir(), "mosquitto.server")
    mark = run("ecce-remote-setup", serverEnv, "--server")
    if not checks.check(mark.returncode == 0 and os.path.exists(marker),
                        "the server account marked (ecce-remote-setup "
                        "--server)"):
        say(mark.stdout.decode())
        return
    bport = int(os.environ["ECCE_BROKER_PORT"])
    try:
        twoSessions(checks, display, logdir, "two-sessions-server",
                    lambda: portOpen(bport), "broker left running")
    finally:
        try:
            os.unlink(marker)
        except OSError:
            pass
        stopOwnBroker(serverEnv)


def caseTwoSessionsStopShared(checks, display, logdir):
    """Mode 3, the site's shared broker: no session stops it anyway; the
    data server is the account's own."""
    env = display.env()
    stopOwnBroker(env)
    shared = startSharedService(checks, env, logdir, "two-sessions-shared")
    if shared is None:
        return
    try:
        twoSessions(checks, display, logdir, "two-sessions-shared",
                    lambda: shared.poll() is None
                    and portOpen(shared.port), "shared service")
    finally:
        stopSharedService(checks, env, shared)


def caseServicesKilled(checks, display, logdir):
    """The data server and the broker killed under a running Organizer:
    it reports the outage and keeps running; nothing crashes."""
    #  Calculations with results, made by an earlier session, so this
    #  Organizer has only listed them, not loaded them, when the services
    #  go -- as after a fresh start.
    prep = Probe("services-killed-prep")
    earlier = Session(display, os.path.join(logdir,
                                            "services-killed-prep.log"),
                      extra=prep.extra)
    try:
        if not checks.check(earlier.organizer(), "an earlier Organizer "
                            "opened, to make calculations"):
            return
        checks.check(prep.ask(earlier, "open", 120).startswith("ok"),
                     "it made a calculation")
        for name in ("nwchem/h2o_opt_stdout.out", "orca/h2o_sym.out"):
            prep.ask(earlier, "import " + os.path.join(
                REPO, "tests", "parsers", "fixtures", name), 180)
    finally:
        earlier.kill()
        clearDisplay(display.name)
    probe = Probe("services-killed")
    session = Session(display, os.path.join(logdir, "services-killed.log"),
                      extra=probe.extra)
    try:
        frame = session.organizer()
        if not checks.check(frame, "the Organizer opened"):
            return
        gatewayIsTheTree(checks, session)
        org = organizerPid(session)
        checks.check(probe.ask(session, "browse", 120) == "ok",
                     "it lists the project")
        #  A crash is caught with its backtrace: gdb waits on the
        #  Organizer and prints one if it stops on a signal.
        gdbLog = os.path.join(logdir, "services-killed-gdb.log")
        gdb = subprocess.Popen(
            ["gdb", "-q", "-batch", "-p", str(org), "-ex", "continue",
             "-ex", "thread apply all bt 25"],
            stdin=subprocess.DEVNULL, stdout=open(gdbLog, "w"),
            stderr=subprocess.STDOUT) if org and shutil.which("gdb") else None
        time.sleep(3)
        amq = broker()
        run("ecce-dataserver-stop", display.env())
        if amq:
            os.kill(amq, 9)
        #  What the 9.0.0-alpha.6-rc2 reaper's stop removed with it.
        for name in os.listdir(statedir()):
            if name.startswith("broker_") or name in ("mosquitto.sock",
                                                      "mosquitto.pid"):
                os.unlink(os.path.join(statedir(), name))
        deadline = time.time() + 30
        while portOpen(fixture.dataserverPort()) and time.time() < deadline:
            time.sleep(0.5)
        checks.check(not portOpen(fixture.dataserverPort())
                     and (amq is None or not alive(amq)),
                     "data server and broker killed (broker %s)" % amq)
        mark = len(Probe.logText(session))
        for command in ("walk", "refresh", "walk", "open calc2", "browse",
                        "publish", "publish"):
            answer = probe.ask(session, command, 180, pid=org)
            checks.check(answer is not None, "%s answered without the "
                         "services: %s" % (command, answer))
        checks.check(org and alive(org), "the Organizer is still running")
        say("    windows now: %s" % [t for _, t in display.windows() if t])
        text = Probe.logText(session)[mark:]
        checks.check(not CRASH.search(text), "no crash or assertion")
        checks.check("no connection to the message broker" in text,
                     "the failed publish was reported")
        checks.check("starting data server" not in text
                     and "starting mosquitto broker" not in text,
                     "nothing restarted the services behind its back")
        if gdb is not None:
            os.kill(gdb.pid, 2)
            try:
                gdb.wait(timeout=20)
            except subprocess.TimeoutExpired:
                gdb.kill()
                gdb.wait()
            with open(gdbLog, errors="replace") as handle:
                trace = handle.read()
            checks.check("received signal SIGSEGV" not in trace
                         and "SIGABRT" not in trace,
                         "gdb saw no crash (%s)" % gdbLog)
    finally:
        session.kill()
        subprocess.run([os.path.join(install, "bin", "ecce-gateway-reap"),
                        "--stop"], env=display.env(),
                       stdout=subprocess.DEVNULL)


def caseDisplayChanges(checks, display, logdir):
    """One session, an app whose DISPLAY differs (as after an ssh -X
    reconnect, here :N.0 for :N): it is still the session's app, so the
    session ends with it and not before (#233)."""
    session = Session(display, os.path.join(logdir, "display-changes.log"))
    builder = None
    try:
        frame = session.organizer()
        if not checks.check(frame, "the Organizer opened"):
            return
        gw = gatewayIsTheTree(checks, session)
        env = session.env()
        env["DISPLAY"] = display.name + ".0"
        builder = subprocess.Popen(
            [os.path.join(wrappers, "ecce-builder")], env=env,
            stdout=session.log, stderr=subprocess.STDOUT,
            start_new_session=True)
        bframe = waitWindow(display, "Builder", timeout=90)
        if not checks.check(bframe, "a Builder opened with DISPLAY=%s"
                            % env["DISPLAY"]):
            return
        bpid = next(iter(named(session.sid(), "builder")), None)
        checks.check(bpid is not None, "the Builder is in the session")
        quitVia(display, frame)
        waitWindow(display, "Organizer", timeout=15, gone=True)
        checks.check(not session.ended(8) and alive(gw),
                     "closing the Organizer alone did not end the session")
        t0 = time.time()
        quitVia(display, bframe)
        try:
            builder.wait(timeout=30)
        except subprocess.TimeoutExpired:
            pass
        endsCleanly(checks, session, display.name, gw, t0)
    finally:
        if builder is not None and builder.poll() is None:
            os.killpg(builder.pid, 15)
            builder.wait(timeout=20)
        session.kill()


def waitAppWindow(display, pattern, timeout=90):
    """A window titled pattern, answering the login dialog on the way (a
    session of its own has no stored data server login yet)."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        windows = display.windows()
        match = [w for w in windows if pattern in w[1]]
        if match:
            return match[0]
        auth = next((w for w in windows if w[1] == "ECCE Authentication"),
                    None)
        if auth:
            env = display.env()
            subprocess.run(["xdotool", "windowfocus", str(int(auth[0], 16))],
                           env=env, timeout=10, stderr=subprocess.DEVNULL)
            time.sleep(0.3)
            subprocess.run(["xdotool", "type", "--delay", "50", "ecce"],
                           env=env, timeout=10)
            subprocess.run(["xdotool", "key", "Return"], env=env, timeout=10)
            time.sleep(3)
        time.sleep(0.5)
    return None


def caseJoin(checks, display, logdir):
    """An app started outside the session, with no ECCE_SESSION_ID (from a
    file manager, say), joins the newest live session of the account; with
    no session alive it is a session of its own (#233, decision 2)."""
    session = Session(display, os.path.join(logdir, "join.log"))
    builder = lone = None
    try:
        frame = session.organizer()
        if not checks.check(frame, "the Organizer opened"):
            return
        gw = gatewayIsTheTree(checks, session)
        env = session.env()
        env.pop("ECCE_SESSION_ID", None)
        builder = subprocess.Popen(
            [os.path.join(wrappers, "ecce-builder")], env=env,
            stdout=session.log, stderr=subprocess.STDOUT,
            start_new_session=True)
        bframe = waitAppWindow(display, "Builder")
        if not checks.check(bframe, "a Builder opened, started without an id"):
            return
        checks.check(named(session.sid(), "builder"),
                     "it joined the session %s" % session.sid())
        quitVia(display, frame)
        waitWindow(display, "Organizer", timeout=15, gone=True)
        checks.check(not session.ended(8) and alive(gw),
                     "closing the Organizer alone did not end the session")
        t0 = time.time()
        quitVia(display, bframe)
        try:
            builder.wait(timeout=30)
        except subprocess.TimeoutExpired:
            pass
        endsCleanly(checks, session, display.name, gw, t0)

        lone = subprocess.Popen(
            [os.path.join(wrappers, "ecce-builder")], env=env,
            stdout=session.log, stderr=subprocess.STDOUT,
            start_new_session=True)
        lframe = waitAppWindow(display, "Builder")
        if not checks.check(lframe, "with no session alive, a Builder "
                            "started without an id opened"):
            return
        pids = named(display.name, "builder")
        sid = environOf(pids[0]).get("ECCE_SESSION_ID") if pids else None
        checks.check(sid and SESSION_ID.match(sid) and sid != session.sid(),
                     "in a session of its own (%s)" % sid)
        checks.check(sid and os.path.exists(
            sessionkey.brokerFile(statedir(), sid)),
            "with its own broker file")
        quitVia(display, lframe)
        try:
            lone.wait(timeout=30)
        except subprocess.TimeoutExpired:
            pass
        checks.check(lone.poll() is not None, "and it ended")
    finally:
        for proc in (builder, lone):
            if proc is not None and proc.poll() is None:
                os.killpg(proc.pid, 15)
                proc.wait(timeout=20)
        session.kill()


def caseShared(checks, display, logdir):
    """Mode 3: the site's shared broker, and two users of it.

    The service is ecce-broker-run --shared, the unit's ExecStart, from a
    state directory of its own (the unit's /var/lib/ecce-broker). The
    users are this run's state and a second one, each on its own display.
    Its accounts are the ones ecce-broker-setup --user adds.
    """
    env = display.env()
    stopOwnBroker(env)
    bport = brokerPort()
    user2 = os.path.join(state, "user2")
    os.makedirs(os.path.join(user2, ".ECCE"), exist_ok=True)
    users = [os.environ["ECCE_REALUSERHOME"], user2]
    checks.check(not portOpen(bport) and not brokersOf(users),
                 "no per-user broker to begin with")
    service = startSharedService(checks, env, logdir, "shared")
    if service is None:
        return
    sport = service.port
    other = xdisplay.Display().__enter__()
    first = second = None
    try:
        extra2 = {"ECCE_REALUSERHOME": user2, "ECCE_NO_DATASERVER": "1"}
        first = Session(display, os.path.join(logdir, "shared-1.log"))
        frame1 = first.organizer()
        if not checks.check(frame1, "user 1's Organizer opened"):
            return
        gw1 = gatewayIsTheTree(checks, first)
        second = Session(other, os.path.join(logdir, "shared-2.log"),
                         extra=extra2)
        frame2 = second.organizer()
        if not checks.check(frame2, "user 2's Organizer opened"):
            return
        gw2 = gatewayIsTheTree(checks, second)
        linked = connectedTo(sport)
        for who, home, gw, session in (("user 1", users[0], gw1, first),
                                       ("user 2", user2, gw2, second)):
            bfile = sessionkey.brokerFile(os.path.join(home, ".ECCE"),
                                          session.sid())
            try:
                with open(bfile) as handle:
                    said = handle.read()
            except OSError:
                said = ""
            checks.check("host=localhost" in said
                         and "port=%d" % sport in said,
                         "%s's processes are pointed at the shared broker"
                         % who)
            checks.check(gw in linked, "%s's gateway %s is logged in to it "
                         "(connected: %s)" % (who, gw, sorted(linked)))
        checks.check(not portOpen(bport) and not brokersOf(users),
                     "no per-user broker was started")

        t0 = time.time()
        quitVia(display, frame1)
        endsCleanly(checks, first, display.name, gw1 or -1, t0)
        checks.check(service.poll() is None and portOpen(sport),
                     "user 1's plain quit left the shared broker running")
        env2 = second.env()     # what its Quit and Stop Server runs with
        t0 = time.time()
        stop = ""
        for script in ("ecce-dataserver-stop", "ecce-gateway-stop"):
            stop += run(script, env2).stdout.decode()
        endsCleanly(checks, second, other.name, gw2 or -1, t0,
                    apps=False)
        checks.check(service.poll() is None and portOpen(sport),
                     "user 2's Quit and Stop Server left it running")
        checks.check("shared service" in stop,
                     "and said why (%s)" % " / ".join(
                         l for l in stop.splitlines() if "broker" in l))
        said = run("ecce-gateway-reap", env, "--stop").stdout.decode()
        checks.check(service.poll() is None and portOpen(sport),
                     "user 1's reaper --stop left it running (%s)"
                     % said.strip().replace("\n", " / "))
        checks.check(portOpen(fixture.dataserverPort()),
                     "user 2's Quit and Stop Server left user 1's data "
                     "server alone")
        checks.check(not portOpen(bport) and not brokersOf(users)
                     and broker() is None,
                     "still no per-user broker, for either user")
    finally:
        for session in (first, second):
            if session is not None:
                session.kill()
        checks.check(clearDisplay(other.name,
                                  [os.path.join(user2, ".ECCE")]),
                     "nothing left on %s" % other.name)
        other.__exit__(None, None, None)
        stopSharedService(checks, env, service)


def startSharedService(checks, env, logdir, tag):
    """A stand-in for ecce-broker.service, declared in siteconfig/
    SharedBroker, with this run's account; the Popen, with .port, or None."""
    bport = brokerPort()
    sport = isolate._pickPort("ECCE_TEST_SHARED_BROKER_PORT", bport + 100)
    decl = os.path.join(os.environ["ECCE_HOME"], "siteconfig",
                        "SharedBroker")
    setup = run("ecce-broker-setup", env, "localhost:%d" % sport)
    if not checks.check(setup.returncode == 0 and os.path.exists(decl),
                        "ecce-broker-setup declared localhost:%d" % sport):
        say(setup.stdout.decode())
        return None
    account = fixture.realUser()
    adduser = subprocess.run(
        [os.path.join(install, "bin", "ecce-broker-setup"), "--user", account],
        env=env, input=(fixture.passwordFor(account) + "\n").encode(),
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    accounts = os.path.join(os.environ["ECCE_HOME"], "siteconfig",
                            "SharedBroker.passwd")
    if not checks.check(adduser.returncode == 0 and os.path.exists(accounts)
                        and oct(os.stat(accounts).st_mode & 0o777) == "0o600",
                        "ecce-broker-setup --user %s wrote a private account "
                        "list" % account):
        say(adduser.stdout.decode())
        return None
    with open(accounts) as handle:
        listed = handle.read()
    checks.check(fixture.passwordFor(account) not in listed
                 and listed.startswith(account + ":$7$"),
                 "the list holds a hash, not the password")
    base = os.path.join(state, "service", "ecce-broker")
    serviceLog = open(os.path.join(logdir, tag + "-service.log"), "w")
    service = subprocess.Popen(
        [os.path.join(install, "bin", "ecce-broker-run"), "--shared", base],
        env={"PATH": os.environ["PATH"],
             "ECCE_HOME": os.environ["ECCE_HOME"]},
        cwd="/", stdin=subprocess.DEVNULL, stdout=serviceLog,
        stderr=subprocess.STDOUT, start_new_session=True)
    service.port, service.decl, service.log = sport, decl, serviceLog
    deadline = time.time() + 60
    while not portOpen(sport) and time.time() < deadline:
        time.sleep(0.5)
    if not checks.check(portOpen(sport) and service.poll() is None,
                        "the stand-in service %d answers on %d"
                        % (service.pid, sport)):
        stopSharedService(checks, env, service)
        return None
    return service


def stopSharedService(checks, env, service):
    run("ecce-broker-setup", env, "--remove")
    service.terminate()
    try:
        service.wait(timeout=30)
    except subprocess.TimeoutExpired:
        service.kill()
        service.wait()
    service.log.close()
    checks.check(not os.path.exists(service.decl)
                 and not portOpen(service.port),
                 "declaration withdrawn and the stand-in service "
                 "stopped (exit %s)" % service.returncode)


def caseMarkers(checks, display, logdir):
    """The reaper alone, on what makes a broker a server's.

    The broker is a stand-in (sleep, run as "mosquitto" under
    mosquitto.pid), so no real socket is involved.
    """
    env = display.env()
    stopOwnBroker(env)
    marker = os.path.join(statedir(), "mosquitto.server")
    standin = os.path.join(state, "standin")
    os.makedirs(standin, exist_ok=True)
    fakeBin = os.path.join(standin, "mosquitto")
    try:
        os.unlink(fakeBin)
    except OSError:
        pass
    os.symlink(shutil.which("sleep"), fakeBin)
    try:
        for marked, survives, what in (
                (True, True, "marked a server's (ecce-remote-setup "
                 "--server)"),
                (False, False, "a per-user broker")):
            if marked:
                open(marker, "w").close()
            fake = subprocess.Popen([fakeBin, "300"],
                                    start_new_session=True)
            with open(os.path.join(statedir(), "mosquitto.pid"), "w") as f:
                f.write("%d\n" % fake.pid)
            said = run("ecce-gateway-reap", env, "--if-idle")
            time.sleep(0.5)
            checks.check((fake.poll() is None) == survives,
                         "%s: --if-idle %s it (%s)"
                         % (what, "leaves" if survives else "stops",
                            said.stdout.decode().strip()))
            fake.kill()
            fake.wait()
            for path in (marker, os.path.join(statedir(), "mosquitto.pid")):
                try:
                    os.unlink(path)
                except OSError:
                    pass
    finally:
        try:
            os.unlink(fakeBin)
        except OSError:
            pass


def caseWindow(checks, display, logdir):
    """ECCE_GATEWAY_WINDOW=1 keeps today's behaviour: the Gateway window
    is the session, an app closing does not end it, its Quit does."""
    d = display.name
    extra = {"ECCE_GATEWAY_WINDOW": "1"}
    session = Session(display, os.path.join(logdir, "window.log"),
                      extra=extra)
    other = None
    try:
        gframe = session.organizer(
            title=lambda t: re.match(r"ECCE \d", t) is not None)
        if not checks.check(gframe, "the Gateway window is shown (%s)"
                            % (gframe and gframe[1])):
            return
        gw = gatewayIsTheTree(checks, session)
        other = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")], env=session.env(),
            stdout=session.log, stderr=subprocess.STDOUT,
            start_new_session=True)
        frame = waitWindow(display, "Organizer", timeout=90)
        if not checks.check(frame, "an Organizer opened"):
            return
        quitVia(display, frame)
        checks.check(not session.ended(8) and alive(gw),
                     "closing the Organizer left the session up")
        t0 = time.time()
        quitVia(display, gframe)
        endsCleanly(checks, session, d, gw, t0)
    finally:
        if other is not None and other.poll() is None:
            os.killpg(other.pid, 15)
        session.kill()


def caseBug(checks, display, logdir):
    import glob
    home = os.environ["ECCE_REALUSERHOME"]
    pattern = os.path.join(home, "ecce-bug-*")
    for old in glob.glob(pattern):
        if os.path.isdir(old):
            shutil.rmtree(old)
        else:
            os.unlink(old)
    log = os.path.join(logdir, "bug.log")
    session = Session(display, log, argv=["--bug"])
    try:
        frame = session.organizer()
        if not checks.check(frame, "the Organizer opened"):
            return
        folders = [f for f in glob.glob(pattern) if os.path.isdir(f)]
        if not checks.check(len(folders) == 1,
                            "one ecce-bug-<time> folder made (%s)" % folders):
            return
        folder = folders[0]
        time.sleep(3)
        checks.check(os.path.exists(os.path.join(folder, "session.log")),
                     "session.log exists while the session runs")
        quitVia(display, frame)
        #  ecce-diagnose runs after the session ends and takes ~30s.
        checks.check(session.ended(180), "`ecce --bug` returned")
        found = sorted(os.listdir(folder))
        for want in ("session.log", "bug-mode.txt", "mosquitto.log",
                     "error_log", "services.txt"):
            checks.check(want in found, "the folder holds %s" % want)
        checks.check(any(f.startswith("ecce-diagnostics-") for f in found),
                     "the folder holds the ecce-diagnose output")
        with open(os.path.join(folder, "bug-mode.txt")) as handle:
            checks.check("ECCE_RCOM_LOGMODE=1" in handle.read(),
                         "bug-mode.txt lists the logging switched on")
        archives = glob.glob(folder + ".zip") + glob.glob(folder + ".tar.gz")
        if checks.check(len(archives) == 1, "one archive made: %s" % archives):
            listing = subprocess.run(
                ["unzip", "-l", archives[0]] if archives[0].endswith(".zip")
                else ["tar", "tzf", archives[0]],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
            checks.check(b"session.log" in listing and b"ecce-diagnostics-"
                         in listing, "the archive holds both")
        with open(log) as handle:
            said = handle.read()
        checks.check("ECCE bug report: attach" in said,
                     "the terminal was told which file to attach")
        say("    session.log: %d bytes; folder: %s"
            % (os.path.getsize(os.path.join(folder, "session.log")), found))
    finally:
        session.kill()


def localHome(base):
    """An $ECCE_HOME with no siteconfig/DataServers, as a local install has."""
    home = os.path.join(state, "ecce-home-local")
    shutil.rmtree(home, ignore_errors=True)
    os.makedirs(os.path.join(home, "siteconfig"))
    for entry in os.listdir(base):
        if entry != "siteconfig":
            os.symlink(os.path.join(base, entry), os.path.join(home, entry))
    for entry in os.listdir(os.path.join(base, "siteconfig")):
        if entry != "DataServers":
            os.symlink(os.path.join(base, "siteconfig", entry),
                       os.path.join(home, "siteconfig", entry))
    return home


def apacheProcs():
    """Pids of this run's own Apache: its command line names the state."""
    found = []
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        try:
            with open("/proc/%s/cmdline" % entry, "rb") as handle:
                argv = handle.read()
        except OSError:
            continue
        if state.encode() in argv and (b"apache2" in argv or b"httpd" in argv):
            found.append(int(entry))
    return found


def makeLocalCalculation(env, home, data, mode="create"):
    """A project and an NWChem calculation made through the real classes
    (Resource::createChild, as the Organizer's New menu does), by
    tests/filedsi/resourceTest, into the user's folder of the local data.
    mode "mopac" makes proj-mopac/ch4 instead, a MOPAC calculation holding
    a molecule and nothing else."""
    build = os.environ.get("ECCE_TEST_BUILD", os.path.join(REPO, "build-cmake"))
    libs = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
            "eccecipc", "ecceutil", "eccecomm", "eccercmd"]
    driver = os.path.join(state, "resourceTest")
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(REPO, "include"), "-o",
            driver, os.path.join(HERE, "..", "filedsi", "resourceTest.C"),
            "-L" + build] + ["-l" + l for l in libs] * 3 + ["-lxerces-c", "-lmosquitto"])
    built = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if built.returncode != 0:
        return built.stdout.decode()[-1500:]
    user = os.path.join(data, "users", getpass.getuser())
    run_env = dict(env, ECCE_HOME=home, ECCE_LOCAL_DATA=data,
                   ECCE_REALUSER=getpass.getuser(),
                   ECCE_NO_MESSAGING="1")     # no session to tell
    done = subprocess.run([driver, mode, user], env=run_env,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return None if done.returncode == 0 else done.stdout.decode()[-1500:]


def straceCmd(path):
    """strace of the file-system calls, following children, or () without it."""
    if not shutil.which("strace"):
        return ()
    return ("strace", "-f", "-o", path, "-s", "300", "-e",
            "trace=openat,getdents64,newfstatat,statx")


def straceSaw(path, pattern, failed=False):
    """Lines of an strace log matching pattern that did not fail (ENOENT),
    or all of them with failed=True."""
    found = []
    with open(path, errors="replace") as handle:
        for line in handle:
            if re.search(pattern, line) and (failed or "ENOENT" not in line):
                found.append(line.strip())
    return found


def launchApp(checks, display, session, name, argv, logdir, titleHint,
              trace=None):
    """Start ecce-<name> through its wrapper; it must open a window and live."""
    before = set(w for w, _ in display.windows())
    log = open(os.path.join(logdir, "local-%s%s.log"
                            % (name, "-ctx" if argv else "")), "w")
    proc = subprocess.Popen(list(trace or ()) +
                            [os.path.join(wrappers, "ecce-" + name)] + argv,
                            env=session.env(), stdout=log,
                            stderr=subprocess.STDOUT, start_new_session=True)
    deadline = time.time() + 90
    new = []
    while time.time() < deadline and not new and proc.poll() is None:
        # A frame, not the app's hidden helper window named after the binary.
        new = [w for w in display.windows()
               if w[0] not in before and w[1] and w[1] != name]
        time.sleep(0.5)
    ok = checks.check(bool(new), "%s %s: a window opened %s"
                      % (name, " ".join(argv[-1:]) if argv else "(bare)",
                         [t for _, t in new]))
    time.sleep(8)
    if shutil.which("import"):
        subprocess.run(["import", "-window", "root", os.path.join(
            logdir, "local-%s%s.png" % (name, "-ctx" if argv else ""))],
            env=session.env(), timeout=30, stderr=subprocess.DEVNULL)
    up = proc.poll() is None
    checks.check(up, "%s still running 8s later%s"
                 % (name, "" if up else " (exit %s)" % proc.returncode))
    log.close()
    with open(log.name, errors="replace") as handle:
        text = handle.read()
    marker = re.search(r"ASSERT|Assertion|Segmentation|terminate called|Fatal|"
                       r"Error|FAILURE", text)
    if marker or not ok or not up:
        say("    %s log tail:\n      %s" % (name, "\n      ".join(
            text.strip().splitlines()[-12:])))
    checks.check(not marker, "%s output has no assertion or error marker" % name)
    if proc.poll() is None:
        try:
            os.killpg(proc.pid, 15)
            proc.wait(timeout=20)
        except (OSError, subprocess.TimeoutExpired):
            pass
    return ok and up


def caseLocal(checks, display, logdir):
    """#216: local mode, no data server, no siteconfig/DataServers."""
    d = display.name
    if not checks.check(d != ":1", "own Xvfb %s, never :1" % d):
        return
    data = os.path.join(state, "localdata")
    shutil.rmtree(data, ignore_errors=True)
    home = localHome(os.environ["ECCE_HOME"])
    checks.check(not os.path.exists(os.path.join(home, "siteconfig",
                                                 "DataServers")),
                 "no siteconfig/DataServers in this run's ECCE_HOME")
    checks.check(not portOpen(fixture.dataserverPort()) and not apacheProcs(),
                 "no data server running before the start")
    user = os.path.join(data, "users", getpass.getuser())
    os.makedirs(user)
    base_env = display.env()
    problem = makeLocalCalculation(base_env, home, data)
    if not checks.check(problem is None, "project and calculation created "
                        "through Resource::createChild"):
        say("    " + (problem or ""))
        return
    say("    user folder holds: %s; proj/water holds: %s"
        % (sorted(os.listdir(user)),
           sorted(os.listdir(os.path.join(user, "proj", "water")))))
    traced = bool(shutil.which("strace"))
    if not traced:
        say("    skip  no strace: the data is not checked as read")
    organizerTrace = os.path.join(logdir, "local-organizer.strace")
    session = Session(display, os.path.join(logdir, "local.log"),
                      extra={"ECCE_LOCAL_DATA": data, "ECCE_HOME": home,
                             "ECCE_ORGANIZER_OPEN":
                             "file://" + os.path.join(user, "proj", "water") + "/"},
                      prefix=straceCmd(organizerTrace))
    try:
        frame = session.organizer()
        if not checks.check(frame, "the Organizer opened"):
            return
        say("    window: %r" % (frame[1],))
        orgs = named(d, "organizer")
        exe = os.readlink("/proc/%d/exe" % orgs[0]) if orgs else ""
        want = os.path.realpath(os.path.join(install, "bin", "organizer"))
        checks.check(orgs and os.path.realpath(exe) == want
                     and not exe.startswith("/opt/ecce"),
                     "the Organizer running is %s, not /opt/ecce" % exe)
        time.sleep(10)
        checks.check(frame[0] in [w for w, _ in display.windows()]
                     and session.proc.poll() is None,
                     "still up, window still there, 10s later")
        checks.check(not apacheProcs() and not portOpen(fixture.dataserverPort()),
                     "no Apache was started")
        checks.check(os.path.isdir(data), "the local data directory was made: %s" % data)
        say("    windows now: %s" % [t for _, t in display.windows() if t])
        env = display.env()
        # The Organizer opened the tree down to the calculation itself
        # (ECCE_ORGANIZER_OPEN, set in the session's environment).
        time.sleep(5)
        time.sleep(3)
        if shutil.which("import"):
            subprocess.run(["import", "-window", "root",
                            os.path.join(logdir, "local-organizer.png")],
                           env=env, timeout=30, stderr=subprocess.DEVNULL)
        if traced:
            q = re.escape(user)
            saw = straceSaw(organizerTrace, q + r"/\.ecce-meta\"", failed=True)
            checks.check(saw, "the Organizer looked for the user folder's "
                         ".ecce-meta (it has none: no properties yet)")
            saw = straceSaw(organizerTrace, q + r"/proj/\.ecce-meta\"")
            checks.check(saw, "the Organizer read the project's own .ecce-meta")
            saw = straceSaw(organizerTrace, q + r"/proj/water/\.ecce-meta\"")
            checks.check(saw, "the Organizer read the calculation's record in "
                         "proj/.ecce-meta and its own (State, Application)")
            saw = straceSaw(organizerTrace, r"getdents64|openat.*" + q + r"/proj\"")
            checks.check(saw, "the Organizer listed the project directory")

        calc = "file://" + os.path.join(user, "proj", "water") + "/"
        calcdir = os.path.join(user, "proj", "water")
        installed = os.path.realpath(os.path.join(install, "data"))
        launchApp(checks, display, session, "builder", [], logdir, "Builder")
        bt = os.path.join(logdir, "local-builder.strace")
        launchApp(checks, display, session, "builder", ["-context", calc],
                  logdir, "Builder", trace=straceCmd(bt))
        ct = os.path.join(logdir, "local-calced.strace")
        launchApp(checks, display, session, "calced", ["-context", calc],
                  logdir, "Calculation", trace=straceCmd(ct))
        if traced:
            for app, log in (("Builder", bt), ("CalcEd", ct)):
                saw = straceSaw(log, re.escape(calcdir) + r"/\.ecce-meta\"")
                checks.check(saw, "%s opened the calculation's .ecce-meta" % app)
                saw = straceSaw(log, re.escape(calcdir) + r"/[^/\"]+\"")
                say("      %s opened %d file(s) under the calculation: %s"
                    % (app, len(saw), sorted(set(
                        re.findall(r'/water/([^/"]+)"', " ".join(saw))))))
        t0 = time.time()
        quitVia(display, frame)
        returned = session.ended(30)
        checks.check(returned, "`ecce` returned after the Organizer closed")
    finally:
        session.kill()


def caseLocalSave(checks, display, logdir):
    """#216: CalcEd sets up and saves a calculation in local mode.

    A MOPAC calculation holding only a molecule needs nothing but Save to
    become launchable: the theory and runtype are the defaults and MOPAC
    needs no basis set.  Ctrl+S goes to CalcEd's own window on this run's
    Xvfb.
    """
    d = display.name
    if not checks.check(d != ":1", "own Xvfb %s, never :1" % d):
        return
    data = os.path.join(state, "localdata-save")
    shutil.rmtree(data, ignore_errors=True)
    home = localHome(os.environ["ECCE_HOME"])
    user = os.path.join(data, "users", getpass.getuser())
    os.makedirs(user)
    problem = makeLocalCalculation(display.env(), home, data, "mopac")
    if not checks.check(problem is None, "MOPAC calculation with a methane "
                        "molecule created"):
        say("    " + (problem or ""))
        return
    calcdir = os.path.join(user, "proj-mopac", "ch4")
    deck = os.path.join(calcdir, "Inputs", "mopac.mop")
    checks.check(not os.path.exists(deck), "no input file before the save")
    session = Session(display, os.path.join(logdir, "local-save.log"),
                      extra={"ECCE_LOCAL_DATA": data, "ECCE_HOME": home})
    try:
        frame = session.organizer()
        if not checks.check(frame, "the Organizer opened"):
            return
        before = set(w for w, _ in display.windows())
        trace = os.path.join(logdir, "local-save-calced.strace")
        log = open(os.path.join(logdir, "local-save-calced.log"), "w")
        proc = subprocess.Popen(
            list(straceCmd(trace)) + [os.path.join(wrappers, "ecce-calced"),
                                      "-context", "file://" + calcdir + "/"],
            env=session.env(), stdout=log, stderr=subprocess.STDOUT,
            start_new_session=True)
        try:
            deadline = time.time() + 90
            win = []
            while time.time() < deadline and not win and proc.poll() is None:
                win = [w for w in display.windows() if w[0] not in before
                       and "MOPAC" in w[1]]
                time.sleep(0.5)
            if not checks.check(win, "CalcEd opened on the calculation %s"
                                % [t for _, t in win]):
                return
            pids = named(d, "calced")
            exe = os.readlink("/proc/%d/exe" % pids[0]) if pids else ""
            checks.check(pids and os.path.realpath(exe) == os.path.realpath(
                os.path.join(install, "bin", "calced")),
                "the CalcEd running is %s" % exe)
            #  It runs the details dialogs once to collect their defaults.
            time.sleep(8)
            env = display.env()
            subprocess.run(["xdotool", "windowfocus", str(int(win[0][0], 16))],
                           env=env, timeout=10, stderr=subprocess.DEVNULL)
            time.sleep(0.5)
            subprocess.run(["xdotool", "key", "ctrl+s"], env=env, timeout=10)
            deadline = time.time() + 60
            while time.time() < deadline and not os.path.exists(deck):
                time.sleep(0.5)
            #  The input checker reads the deck back after the save.
            time.sleep(5)
            if shutil.which("import"):
                subprocess.run(["import", "-window", "root", os.path.join(
                    logdir, "local-save.png")], env=env, timeout=30,
                    stderr=subprocess.DEVNULL)
            up = proc.poll() is None
            checks.check(up, "CalcEd still running after the save")
        finally:
            if proc.poll() is None:
                try:
                    os.killpg(proc.pid, 15)
                    proc.wait(timeout=20)
                except (OSError, subprocess.TimeoutExpired):
                    pass
            log.close()
        with open(log.name, errors="replace") as handle:
            text = handle.read()
        marker = re.search(r"ASSERT|Assertion|Segmentation|terminate called|"
                           r"Fatal|FAILURE|Unhandled|ended by SIG", text)
        if marker:
            say("    calced log tail:\n      " + "\n      ".join(
                text.strip().splitlines()[-15:]))
        checks.check(not marker, "CalcEd output has no assertion or error marker")
        if not checks.check(os.path.exists(deck), "Save wrote Inputs/mopac.mop"):
            return
        with open(deck, errors="replace") as handle:
            text = handle.read()
        say("    deck:\n      " + "\n      ".join(text.strip().splitlines()))
        checks.check(len(re.findall(r"^\s*H\s", text, re.M)) == 4
                     and re.search(r"^\s*C\s", text, re.M),
                     "the deck holds methane's five atoms")
        meta = {}
        with open(os.path.join(calcdir, ".ecce-meta"), errors="replace") as handle:
            for line in handle:
                f = line.rstrip("\n").split("\t")
                if len(f) == 4 and f[0] == ".":
                    meta[f[1].rsplit(":", 1)[-1]] = f[3]
        say("    state %r, theory %r, runtype %r" % (
            meta.get("state"), meta.get("theory"), meta.get("runtype")))
        checks.check(meta.get("state") == "Ready", "the calculation is Ready")
        checks.check(meta.get("theory") and meta.get("runtype"),
                     "theory and runtype were stored")
        checks.check("mopac.mop" in meta.get("hasinputs", "")
                     and "CDATA" not in meta.get("hasinputs", ""),
                     "the input list names mopac.mop, as parsed XML")
        params = os.listdir(os.path.join(calcdir, "Parameters"))
        say("    Parameters/: %s" % sorted(params))
        checks.check(any(p.startswith("GUIValues") or "Setup" in p
                         for p in params), "the dialog settings were stored")
        if straceCmd(trace):
            saw = [l for l in straceSaw(trace, re.escape(deck) + r'"')
                   if "O_RDONLY" in l]
            checks.check(saw, "the deck was read back after it was written "
                         "(the input checker)")
        quitVia(display, frame)
        checks.check(session.ended(30), "`ecce` returned after the Organizer "
                     "closed")
    finally:
        session.kill()

def procEnv(pid):
    try:
        with open("/proc/%d/environ" % pid, "rb") as handle:
            raw = handle.read().split(b"\0")
    except OSError:
        return {}
    return dict(e.decode(errors="replace").split("=", 1)
                for e in raw if b"=" in e)


def preferencesShot(display, frame, logdir):
    """Edit > Preferences, Data folder tab, as a screenshot to look at."""
    env = display.env()
    wid = str(int(frame[0], 16))
    subprocess.run(["xdotool", "windowfocus", wid], env=env, timeout=10,
                   stderr=subprocess.DEVNULL)
    time.sleep(0.5)
    subprocess.run(["xdotool", "key", "alt+e"], env=env, timeout=10)
    time.sleep(1)
    subprocess.run(["xdotool", "key", "n"], env=env, timeout=10)
    deadline = time.time() + 20
    prefs = None
    while time.time() < deadline and not prefs:
        prefs = next((w for w in display.windows()
                      if w[1] == "ECCE Preferences"), None)
        time.sleep(0.5)
    if not prefs:
        say("    (no Preferences window to photograph)")
        return
    time.sleep(2)
    geo = subprocess.run(["xdotool", "getwindowgeometry", "--shell",
                          str(int(prefs[0], 16))], env=env, timeout=10,
                         stdout=subprocess.PIPE).stdout.decode()
    size = dict(l.split("=") for l in geo.split() if "=" in l)
    # Tabs: General, External programs, Data folder; the third is right of
    # the other two, about 230 px in.
    subprocess.run(["xdotool", "mousemove", "--window", str(int(prefs[0], 16)),
                    "250", "18", "click", "1"], env=env, timeout=10)
    time.sleep(2)
    subprocess.run(["import", "-window", "root", os.path.join(
        logdir, "local-pref-preferences.png")], env=env, timeout=30,
        stderr=subprocess.DEVNULL)
    closeWindow(display, prefs[0])
    time.sleep(1)


def caseLocalPref(checks, display, logdir):
    """#216: local mode chosen by the preference alone, with no
    ECCE_LOCAL_DATA; then a folder move asked for in Preferences and
    accepted at the next start."""
    d = display.name
    if not checks.check(d != ":1", "own Xvfb %s, never :1" % d):
        return
    prefs = os.path.join(os.environ["ECCE_REALUSERHOME"], ".ECCE", "EcceGlobal")
    os.makedirs(os.path.dirname(prefs), exist_ok=True)
    saved = open(prefs).read() if os.path.exists(prefs) else None
    first = os.path.join(state, "prefdata")
    second = os.path.join(state, "prefdata-moved")
    for p in (first, second):
        shutil.rmtree(p, ignore_errors=True)
    home = localHome(os.environ["ECCE_HOME"])
    user = getpass.getuser()

    def writePrefs(extra):
        with open(prefs, "w") as handle:
            handle.write((saved or "") + extra)

    def start(tag):
        session = Session(display, os.path.join(logdir, "local-pref-%s.log" % tag),
                          extra={"ECCE_HOME": home})
        session.extra.pop("ECCE_LOCAL_DATA", None)
        return session

    try:
        writePrefs("LOCALDATA:\ttrue\nLOCALDATA.FOLDER:\t%s\n" % first)
        env = display.env()
        checks.check("ECCE_LOCAL_DATA" not in env,
                     "the session starts with no ECCE_LOCAL_DATA")
        session = start("first")
        try:
            frame = session.organizer()
            if not checks.check(frame, "the Organizer opened"):
                return
            orgs = named(d, "organizer")
            got = procEnv(orgs[0]).get("ECCE_LOCAL_DATA") if orgs else None
            checks.check(got == first, "the wrapper exported the preference's "
                         "folder to the Organizer: %r" % got)
            checks.check(not apacheProcs() and
                         not portOpen(fixture.dataserverPort()),
                         "no Apache was started")
            checks.check(os.path.isdir(os.path.join(first, "users", "local")),
                         "the folder has users/local")
            if shutil.which("import"):
                preferencesShot(display, frame, logdir)
            quitVia(display, frame)
            checks.check(session.ended(30), "`ecce` returned")
        finally:
            session.kill()

        # Preferences asked for a new folder: offered and moved at start.
        with open(os.path.join(first, "users", user, "marker"), "w") as handle:
            handle.write("calculations")
        writePrefs("LOCALDATA:\ttrue\nLOCALDATA.FOLDER:\t%s\n"
                   "LOCALDATA.MOVETO:\t%s\n" % (first, second))
        # The first session's apps may outlive `ecce` by a moment.
        inuse = os.path.join(home, "bin", "ecce-localdata")
        deadline = time.time() + 30
        while time.time() < deadline and subprocess.run(
                [inuse, "in-use", first], env=display.env()).returncode == 0:
            time.sleep(1)
        checks.check(time.time() < deadline,
                     "no process of the first session holds the folder")
        session = start("move")
        try:
            deadline = time.time() + 60
            dialog = None
            while time.time() < deadline and not dialog:
                dialog = next((w for w in display.windows()
                               if w[1] == "ECCE data folder"), None)
                time.sleep(0.5)
            if not checks.check(dialog, "the move is offered before the "
                                "Organizer opens"):
                return
            # Listed is not yet mapped; a click before that goes nowhere.
            deadline = time.time() + 30
            while time.time() < deadline and b"IsViewable" not in subprocess.run(
                    ["xwininfo", "-id", dialog[0]], env=env,
                    stdout=subprocess.PIPE, stderr=subprocess.DEVNULL).stdout:
                time.sleep(0.5)
            time.sleep(2)
            if shutil.which("import"):
                subprocess.run(["import", "-window", "root", os.path.join(
                    logdir, "local-pref-move-dialog.png")], env=env,
                    timeout=30, stderr=subprocess.DEVNULL)
            clickLastButton(display, dialog[0])        # "Move", right-most
            frame = session.organizer()
            if not checks.check(frame, "the Organizer opened after the move"):
                return
            checks.check(not os.path.exists(first) and os.path.exists(
                os.path.join(second, "users", user, "marker")),
                "the folder moved, its contents with it")
            orgs = named(d, "organizer")
            got = procEnv(orgs[0]).get("ECCE_LOCAL_DATA") if orgs else None
            checks.check(got == second, "the session uses the new folder: %r"
                         % got)
            text = open(prefs).read()
            checks.check("LOCALDATA.FOLDER:\t%s" % second in text and
                         "MOVETO" not in text,
                         "the preference names the new folder, no move pending")
            quitVia(display, frame)
            checks.check(session.ended(30), "`ecce` returned")
        finally:
            session.kill()
    finally:
        if saved is None:
            os.unlink(prefs)
        else:
            with open(prefs, "w") as handle:
                handle.write(saved)


CASES = {"local": caseLocal, "local-pref": caseLocalPref, "local-save": caseLocalSave, "bug": caseBug, "window": caseWindow, "stop": caseStop, "remote": caseRemote,
         "remote-down": caseRemoteDown, "remote-refused": caseRemoteRefused,
         "quit-stop": caseQuitStop,
         "displays": caseDisplays, "same-display": caseSameDisplay,
         "display-changes": caseDisplayChanges, "join": caseJoin,
         "shared": caseShared,
         "markers": caseMarkers,
         "organizer": caseOrganizer, "builder": caseBuilder,
         "jobstore": caseJobstore,
         "two-sessions": caseTwoSessionsStop,
         "two-sessions-server": caseTwoSessionsStopServer,
         "two-sessions-shared": caseTwoSessionsStopShared,
         "services-killed": caseServicesKilled}


def main():
    if X is None:
        say("SKIP: python3-xlib is needed to close a window as a WM would")
        return 0
    settings = isolate.apply(apps.INSTALL, state)
    say(isolate.describe(settings))
    note = isolate.killLeftovers(state)
    if note:
        say("  " + note)
    logdir = os.path.join(state, "logs")
    os.makedirs(logdir, exist_ok=True)
    checks = Checks()
    try:
        display = xdisplay.Display().__enter__()
    except xdisplay.DisplayUnavailable as exc:
        say("SKIP: %s" % exc)
        return 0
    try:
        for name in args.cases:
            say("%s:" % name)
            if not checks.check(clearDisplay(display.name),
                                "nothing of an earlier case left on %s"
                                % display.name):
                continue
            #  Cases share one broker and data server, as sessions do;
            #  the stop case takes both down, the next session restarts
            #  the broker and this restarts the data server.
            if name in ("local", "local-save", "local-pref"):   # no data server
                subprocess.run([os.path.join(install, "bin",
                                             "ecce-dataserver-stop")],
                               env=display.env(), stdout=subprocess.DEVNULL,
                               stderr=subprocess.STDOUT, timeout=120)
                CASES[name](checks, display, logdir)
                continue
            subprocess.run([os.path.join(install, "bin",
                                         "ecce-dataserver-start")],
                           env=display.env(), stdout=subprocess.DEVNULL,
                           stderr=subprocess.STDOUT, timeout=120)
            fixture.ensureRealUserAccount()
            CASES[name](checks, display, logdir)
        checks.check(clearDisplay(display.name),
                     "nothing of the last case left on %s" % display.name)
    finally:
        apps.stopServices(display)
        display.__exit__(None, None, None)
    checks.check(broker() is None and not portOpen(
                     int(os.environ["ECCE_BROKER_PORT"])),
                 "the run's broker is stopped at the end")
    say("%d failure(s); logs in %s" % (len(checks.failed), logdir))
    return 1 if checks.failed else 0


if __name__ == "__main__":
    sys.exit(main())
