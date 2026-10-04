#!/usr/bin/env python3
"""Does quitting the last app end an ECCE session? (#185)

With the Gateway window hidden (#93) there is no Quit button, so the
session has to end when its last app closes: `ecce` returns and the
gateway and this display's JMSDispatcher go. The broker and the data
server stay up; only an explicit Quit and Stop Server stops the broker.
This drives that through the real wrappers on a private Xvfb, closing
windows the way a window manager does (WM_DELETE_WINDOW):

  organizer   close the Organizer; `ecce` returns, the per-user broker
              is stopped (mode 1), the data server is still running
  builder     open a Builder first; closing the Organizer alone must not
              end the session, closing the Builder afterwards must
  jobstore    a stand-in for a running eccejobstore must not hold the
              session open, and must survive it
  stop        ecce-gateway-stop (Quit and Stop Server) ends the session
              and stops the broker
  quit-stop   the Organizer's Quit and Stop Server, clicked: all the
              teardown's output reaches `ecce`'s stream before it returns
  remote      #167's recipe (mode 2): with the server account marked
              (ecce-remote-setup --server), neither its own plain quit nor
              a -remote client's quit stops its broker
  remote-down `ecce -remote` with nothing listening on the central
              server's ports: one message naming them, a non-zero exit,
              no gateway left to abort
  displays    one user on two displays: the per-user broker outlives the
              first session and goes with the last
  shared      mode 3: a stand-in for ecce-broker.service, run exactly as
              the unit runs it from its own state directory, used by two
              "users" (two state directories) through siteconfig/
              SharedBroker; no quit, reap or Quit and Stop Server stops it,
              and no per-user broker is ever started
  markers     the reaper alone: a broker listening beyond loopback, or
              under a server marker, survives --if-idle; a loopback one
              does not
  window      ECCE_GATEWAY_WINDOW=1 keeps the Gateway window's behaviour
  bug         `ecce --bug`: a folder with session.log, service logs and
              ecce-diagnose output, and an archive, once the session ends

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
        if script.startswith("ecce-") and os.access(
                os.path.join(gwdir, script), os.X_OK):
            overrides[script] = os.path.join(gwdir, script)
    overrides["ecce-remote-setup"] = os.path.join(
        REPO, "packaging", "dataserver", "ecce-remote-setup")
    overrides["ecce-dataserver-start"] = os.path.join(
        REPO, "packaging", "dataserver", "ecce-dataserver-start")
    overrides["ecce-diagnose"] = os.path.join(REPO, "packaging",
                                              "ecce-diagnose")
    #  Scripts this change adds are not in the install at all yet.
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
                                 "remote", "remote-down", "displays",
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

state = isolate.resolveStateDir(
    os.environ.get("ECCE_TEST_STATE")
    or os.path.join(isolate.defaultStateDir() + "-session"))
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
import xdisplay  # noqa: E402

try:
    from Xlib import X, display as xlibdisplay, protocol
except ImportError:
    X = None


def say(text):
    print(text, flush=True)


# --- the process table ---------------------------------------------------

def procs(displayName):
    """{pid: exe} of this user's processes on displayName."""
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
        if ("DISPLAY=" + displayName).encode() in env:
            found[int(entry)] = exe
    return found


def named(displayName, name):
    return [pid for pid, exe in procs(displayName).items()
            if os.path.basename(exe).split(" ")[0] == name]


def sessionProcs(displayName, keep=("eccejobstore",)):
    """{pid: name} of the session's apps and its dispatcher still running."""
    found = {}
    for pid, exe in procs(displayName).items():
        exe = exe.replace(" (deleted)", "")
        name = os.path.basename(exe)
        link = os.path.join(install, "bin", name)
        if (name not in keep and os.path.exists(link)
                and os.path.realpath(link) == exe):
            found[pid] = name
        else:
            try:
                with open("/proc/%d/cmdline" % pid, "rb") as handle:
                    if b"JMSDispatcher" in handle.read():
                        found[pid] = "JMSDispatcher"
            except OSError:
                pass
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
    """Kill every ECCE process and relay on displayName; True once none is.

    Verified by re-reading the process table, not by sleeping: SIGTERM,
    then SIGKILL for whatever is left after 10s.
    """
    def victims():
        pids = dict(eccePids(displayName))
        for base in statedirs or [statedir()]:
            relay = pidfile(os.path.join(base, "jmsdispatcher_%s.pid"
                                         % displayName))
            if relay is not None and alive(relay):
                pids[relay] = "JMSDispatcher"
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
    for base in statedirs or [statedir()]:
        for name in ("jmsdispatcher_%s.pid" % displayName,):
            try:
                os.unlink(os.path.join(base, name))
            except OSError:
                pass
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


def dispatcher(displayName):
    return pidfile(os.path.join(statedir(),
                                "jmsdispatcher_%s.pid" % displayName))


def broker():
    return pidfile(os.path.join(statedir(), "activemq.pid"))


def portOpen(port):
    with socket.socket() as sock:
        sock.settimeout(0.5)
        return sock.connect_ex(("127.0.0.1", port)) == 0


def brokerPort():
    return int(os.environ["ECCE_BROKER_PORT"])


def brokerStopped(checks, amq, why):
    checks.check(amq is not None and not alive(amq)
                 and not portOpen(brokerPort()),
                 "the per-user broker %s was stopped: %s" % (amq, why))
    checks.check(broker() is None, "and its pidfile removed")


def brokersOf(statedirs):
    """Pids of brokers whose ActiveMQ base is under one of statedirs."""
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
            want = ("-Dactivemq.base=" + os.path.join(base, ".ECCE",
                                                      "activemq")).encode()
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
        self.proc = subprocess.Popen(
            list(prefix) + [os.path.join(wrappers, "ecce")] + list(argv),
            env=self.env(),
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE if pipe else self.log,
            stderr=subprocess.STDOUT, start_new_session=True)

    def env(self):
        env = self.display.env()
        env.update(self.extra)
        env["PATH"] = wrappers + os.pathsep + env.get("PATH", "")
        return env

    def organizer(self, title="Organizer"):
        # ensureRealUserAccount()'s password; the gateway asks for it first.
        deadline = time.time() + 90
        frame = None
        while time.time() < deadline and not frame:
            titles = self.display.windows()
            frame = next((w for w in titles
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


def gatewayIsTheTree(checks, d):
    pids = named(d, "gateway")
    if not checks.check(len(pids) == 1, "one gateway on %s (%s)" % (d, pids)):
        return None
    exe = os.readlink("/proc/%d/exe" % pids[0])
    if build:
        checks.check(os.path.samefile(exe, os.path.join(build, "gateway")),
                     "the gateway running is the tree's: %s" % exe)
    return pids[0]


def endsCleanly(checks, session, d, gw, disp, t0, apps=True):
    returned = session.ended(30)
    checks.check(returned, "`ecce` returned (%.1fs after the last close)"
                 % (time.time() - t0))
    if not returned:
        return
    time.sleep(1)
    checks.check(not alive(gw), "gateway %d gone" % gw)
    checks.check(disp is not None and not alive(disp),
                 "JMSDispatcher %s gone" % disp)
    left = sessionProcs(d)
    if not apps:
        #  The stop case ends the session from a script while the
        #  Organizer is still up; CalcMgr has Destroy()ed it by then.
        left = {p: n for p, n in left.items() if n != "organizer"}
    checks.check(not left, "no gateway, app or dispatcher left on %s %s"
                 % (d, left))


def caseOrganizer(checks, display, logdir):
    d = display.name
    session = Session(display, os.path.join(logdir, "organizer.log"))
    try:
        frame = session.organizer()
        if not checks.check(frame, "the Organizer opened"):
            return
        gw = gatewayIsTheTree(checks, d)
        disp, amq = dispatcher(d), broker()
        checks.check(disp and alive(disp), "dispatcher running (%s)" % disp)
        checks.check(amq and alive(amq), "broker running (%s)" % amq)
        t0 = time.time()
        quitVia(display, frame)
        endsCleanly(checks, session, d, gw, disp, t0)
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
        gw = gatewayIsTheTree(checks, d)
        disp = dispatcher(d)
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
        checks.check(alive(gw) and alive(disp),
                     "gateway and dispatcher still up while Builder is")
        t0 = time.time()
        quitVia(display, bframe)
        try:
            builder.wait(timeout=30)
        except subprocess.TimeoutExpired:
            pass
        endsCleanly(checks, session, d, gw, disp, t0)
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
        gw = gatewayIsTheTree(checks, d)
        job = subprocess.Popen(["nohup", fake, "600"], env=session.env(),
                               stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL,
                               start_new_session=True)
        time.sleep(1)
        t0 = time.time()
        quitVia(display, frame)
        returned = session.ended(30)
        checks.check(returned, "`ecce` returned with a job being monitored "
                     "(%.1fs)" % (time.time() - t0))
        checks.check(not alive(gw), "gateway gone")
        checks.check(job.poll() is None, "the job monitor survived")
        disp = dispatcher(d)
        checks.check(disp is not None and alive(disp),
                     "dispatcher kept for the job monitor (reaper's rule)")
    finally:
        if job is not None:
            job.kill()
            job.wait()
        session.kill()
        os.unlink(fake)
        os.symlink(original, fake)
        subprocess.run([os.path.join(install, "bin", "ecce-gateway-reap")],
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
        gw = gatewayIsTheTree(checks, d)
        disp, amq = dispatcher(d), broker()
        checks.check(amq and alive(amq), "broker running (%s)" % amq)
        t0 = time.time()
        for script in ("ecce-dataserver-stop", "ecce-gateway-stop"):
            subprocess.run([os.path.join(install, "bin", script)],
                           env=session.env(), stdout=session.log,
                           stderr=subprocess.STDOUT, timeout=120)
        endsCleanly(checks, session, d, gw, disp, t0, apps=False)
        checks.check(amq is not None and not alive(amq)
                     and not portOpen(int(os.environ["ECCE_BROKER_PORT"])),
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
        gw = gatewayIsTheTree(checks, d)
        disp, amq = dispatcher(d), broker()
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
        checks.check("stopped JMSDispatcher" in before
                     and "stopping ActiveMQ broker" in before,
                     "the teardown's messages were printed")
        if not checks.check(not late, "nothing printed after `ecce` "
                            "returned: %r" % late):
            for t, data in watch.chunks:
                say("    %+.2fs %r" % (t - watch.exited, data))
        checks.check(amq is not None and not alive(amq),
                     "the broker %s was stopped" % amq)
        checks.check(not portOpen(fixture.dataserverPort()),
                     "the data server was stopped")
        checks.check(not alive(gw or -1) and not (disp and alive(disp)),
                     "gateway and relay gone")
    finally:
        session.kill()


def caseRemote(checks, display, logdir):
    """#167's single-machine recipe: a client quitting under -remote.

    This run's own services are the central server; the client has a
    state directory, an ECCE_HOME configured by the real
    ecce-remote-setup, and a display of its own. The server's side of the
    session is a relay started by ecce-gateway-start, as starting ECCE
    there would.
    """
    serverEnv = display.env()
    subprocess.run([os.path.join(install, "bin", "ecce-gateway-start")],
                   env=serverEnv, stdout=subprocess.DEVNULL,
                   stderr=subprocess.STDOUT, timeout=180)
    amq, sdisp = broker(), dispatcher(display.name)
    dport = fixture.dataserverPort()
    bport = int(os.environ["ECCE_BROKER_PORT"])
    if not checks.check(amq and alive(amq) and sdisp and alive(sdisp),
                        "server broker %s and relay %s up" % (amq, sdisp)):
        return
    marker = os.path.join(statedir(), "activemq", "server")
    mark = run("ecce-remote-setup", serverEnv, "--server")
    if not checks.check(mark.returncode == 0 and os.path.exists(marker),
                        "the server account marked (ecce-remote-setup "
                        "--server)"):
        say(mark.stdout.decode())
        return
    client = os.path.join(state, "client")
    os.makedirs(os.path.join(client, ".ECCE"), exist_ok=True)
    chome = isolate.homeOverlay(apps.INSTALL, client, dport, bport)
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
         str(dport), str(bport)], env=dict(os.environ, **extra),
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
        checks.check(waitWindow(cdisplay, "ECCE Organizer on localhost", 10),
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
        cdisp = pidfile(os.path.join(client, ".ECCE",
                                     "jmsdispatcher_%s.pid" % cd))
        checks.check(cdisp and alive(cdisp), "client relay running (%s)"
                     % cdisp)
        checks.check(not os.path.exists(os.path.join(client, ".ECCE",
                                                     "activemq.pid")),
                     "no broker of the client's own")
        gw = gatewayIsTheTree(checks, cd)
        # The teacher's session ends first: its reaper runs, exactly as
        # the gateway wrapper's EXIT trap does it, and takes only the
        # teacher's relay.
        reap = subprocess.run(
            [os.path.join(install, "bin", "ecce-gateway-reap"), "--if-idle"],
            env=serverEnv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        said = reap.stdout.decode().strip().replace("\n", " / ")
        checks.check(not alive(sdisp), "the teacher's relay %d was reaped"
                     % sdisp)
        checks.check(alive(amq) and "broker" not in said,
                     "the marked server's own plain quit left the broker "
                     "the client uses (reaper: %s)" % said)
        t0 = time.time()
        quitVia(cdisplay, frame)
        endsCleanly(checks, session, cd, gw or -1, cdisp, t0)
        checks.check(alive(amq), "the client quitting left the server's "
                     "broker running")
        checks.check(portOpen(dport) and portOpen(bport),
                     "the server's data server and broker still answer")
    finally:
        if session is not None:
            session.kill()
        checks.check(clearDisplay(cdisplay.name,
                                  [os.path.join(client, ".ECCE")]),
                     "nothing left on the client's display %s"
                     % cdisplay.name)
        stopEnv = dict(cdisplay.env(), **extra)
        subprocess.run([os.path.join(install, "bin", "ecce-gateway-stop")],
                       env=stopEnv, stdout=subprocess.DEVNULL)
        cdisplay.__exit__(None, None, None)
        try:
            os.unlink(marker)
        except OSError:
            pass


def caseRemoteDown(checks, display, logdir):
    """`ecce -remote` with the central server down: a message naming the
    server and its ports, a non-zero exit, and no gateway left to abort
    on the relay's missing port file."""
    client = os.path.join(state, "client-down")
    shutil.rmtree(client, ignore_errors=True)
    os.makedirs(os.path.join(client, ".ECCE"))
    dport = isolate._pickPort("ECCE_TEST_DOWN_DATA_PORT", 8390)
    bport = isolate._pickPort("ECCE_TEST_DOWN_BROKER_PORT", dport + 10)
    chome = isolate.homeOverlay(apps.INSTALL, client, dport, bport)
    extra = {"ECCE_REALUSERHOME": client, "ECCE_HOME": chome}
    setup = subprocess.run(
        [os.path.join(install, "bin", "ecce-remote-setup"), "localhost",
         str(dport), str(bport)], env=dict(os.environ, **extra),
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
                     "no assertion, core dump or relay timeout")
        checks.check(not named(display.name, "gateway"),
                     "no gateway process left")
        relay = pidfile(os.path.join(client, ".ECCE",
                                     "jmsdispatcher_%s.pid" % display.name))
        checks.check(relay is None or not alive(relay),
                     "no relay left (%s)" % relay)
        if want not in said:
            say("    " + said.replace("\n", "\n    "))
    finally:
        session.kill()

    # Both ports answer, but the "broker" is a socket that never speaks,
    # so the relay cannot connect and never reports ready.
    silent = socket.socket()
    silent.bind(("127.0.0.1", 0))
    silent.listen(8)
    sport = silent.getsockname()[1]
    try:
        setup = subprocess.run(
            [os.path.join(install, "bin", "ecce-remote-setup"), "localhost",
             str(fixture.dataserverPort()), str(sport)],
            env=dict(os.environ, **extra), stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)
        if not checks.check(setup.returncode == 0, "ecce-remote-setup ran "
                            "against a silent broker port %d" % sport):
            say(setup.stdout.decode())
            return
        log = os.path.join(logdir, "remote-relay.log")
        t0 = time.time()
        session = Session(display, log, ["-remote"], extra)
        try:
            returned = session.ended(45)
            checks.check(returned and session.proc.returncode != 0,
                         "a relay that never becomes ready: `ecce` exits "
                         "non-zero (%s, %.1fs)"
                         % (session.proc.returncode, time.time() - t0))
            with open(log, errors="replace") as handle:
                said = handle.read()
            checks.check("did not report ready" in said
                         and "not started" in said
                         and "ASSERTION" not in said,
                         "with the relay's message, not the gateway's "
                         "assertion")
            checks.check(not named(display.name, "gateway"),
                         "no gateway process left")
            relay = pidfile(os.path.join(
                client, ".ECCE", "jmsdispatcher_%s.pid" % display.name))
            checks.check(relay is None, "the relay that never reported was "
                         "stopped and its pidfile removed (%s)" % relay)
        finally:
            session.kill()
    finally:
        silent.close()


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
        gw1 = gatewayIsTheTree(checks, display.name)
        second = Session(other, os.path.join(logdir, "displays-2.log"))
        frame2 = second.organizer()
        if not checks.check(frame2, "a second Organizer opened on %s"
                            % other.name):
            return
        gw2 = gatewayIsTheTree(checks, other.name)
        amq = broker()
        disp1, disp2 = dispatcher(display.name), dispatcher(other.name)
        checks.check(amq and alive(amq), "one broker for both (%s)" % amq)
        t0 = time.time()
        quitVia(display, frame1)
        endsCleanly(checks, first, display.name, gw1 or -1, disp1, t0)
        checks.check(amq is not None and alive(amq)
                     and portOpen(brokerPort()),
                     "the broker survives the first session: the user "
                     "is still on %s" % other.name)
        checks.check(disp2 and alive(disp2), "the other display's relay "
                     "is untouched (%s)" % disp2)
        t0 = time.time()
        quitVia(other, frame2)
        endsCleanly(checks, second, other.name, gw2 or -1, disp2, t0)
        brokerStopped(checks, amq, "the user's last session ended")
    finally:
        for session in (first, second):
            if session is not None:
                session.kill()
        checks.check(clearDisplay(other.name), "nothing left on %s"
                     % other.name)
        other.__exit__(None, None, None)


def caseShared(checks, display, logdir):
    """Mode 3: the site's shared broker, and two users of it.

    The service is ecce-broker-run --shared, the unit's ExecStart, from a
    state directory of its own (the unit's /var/lib/ecce-broker). The
    users are this run's state and a second one, each on its own display.
    """
    env = display.env()
    stopOwnBroker(env)
    bport = brokerPort()
    user2 = os.path.join(state, "user2")
    os.makedirs(os.path.join(user2, ".ECCE"), exist_ok=True)
    users = [os.environ["ECCE_REALUSERHOME"], user2]
    checks.check(not portOpen(bport) and not brokersOf(users),
                 "no per-user broker to begin with")
    sport = isolate._pickPort("ECCE_TEST_SHARED_BROKER_PORT", bport + 100)
    decl = os.path.join(os.environ["ECCE_HOME"], "siteconfig",
                        "SharedBroker")
    setup = run("ecce-broker-setup", env, "localhost:%d" % sport)
    if not checks.check(setup.returncode == 0 and os.path.exists(decl),
                        "ecce-broker-setup declared localhost:%d" % sport):
        say(setup.stdout.decode())
        return
    base = os.path.join(state, "service", "ecce-broker")
    serviceLog = open(os.path.join(logdir, "shared-service.log"), "w")
    service = subprocess.Popen(
        [os.path.join(install, "bin", "ecce-broker-run"), "--shared", base],
        env={"PATH": os.environ["PATH"],
             "ECCE_HOME": os.environ["ECCE_HOME"]},
        cwd="/", stdin=subprocess.DEVNULL, stdout=serviceLog,
        stderr=subprocess.STDOUT, start_new_session=True)
    other = xdisplay.Display().__enter__()
    first = second = None
    try:
        deadline = time.time() + 60
        while not portOpen(sport) and time.time() < deadline:
            time.sleep(0.5)
        if not checks.check(portOpen(sport) and service.poll() is None,
                            "the stand-in service %d answers on %d"
                            % (service.pid, sport)):
            return
        extra2 = {"ECCE_REALUSERHOME": user2, "ECCE_NO_DATASERVER": "1"}
        first = Session(display, os.path.join(logdir, "shared-1.log"))
        frame1 = first.organizer()
        if not checks.check(frame1, "user 1's Organizer opened"):
            return
        gw1 = gatewayIsTheTree(checks, display.name)
        second = Session(other, os.path.join(logdir, "shared-2.log"),
                         extra=extra2)
        frame2 = second.organizer()
        if not checks.check(frame2, "user 2's Organizer opened"):
            return
        gw2 = gatewayIsTheTree(checks, other.name)
        disp1 = dispatcher(display.name)
        disp2 = pidfile(os.path.join(user2, ".ECCE",
                                     "jmsdispatcher_%s.pid" % other.name))
        linked = connectedTo(sport)
        for who, home, relay in (("user 1", users[0], disp1),
                                 ("user 2", user2, disp2)):
            with open(os.path.join(home, ".ECCE", "siteconfig",
                                   "jndi.properties")) as handle:
                jndi = handle.read()
            checks.check("tcp://localhost:%d" % sport in jndi,
                         "%s's relay is pointed at the shared broker" % who)
            checks.check(relay in linked, "%s's relay %s is connected to "
                         "it (connected: %s)" % (who, relay, sorted(linked)))
        checks.check(not portOpen(bport) and not brokersOf(users),
                     "no per-user broker was started")

        t0 = time.time()
        quitVia(display, frame1)
        endsCleanly(checks, first, display.name, gw1 or -1, disp1, t0)
        checks.check(service.poll() is None and portOpen(sport),
                     "user 1's plain quit left the shared broker running")
        env2 = dict(other.env(), **extra2)
        t0 = time.time()
        stop = ""
        for script in ("ecce-dataserver-stop", "ecce-gateway-stop"):
            stop += run(script, env2).stdout.decode()
        endsCleanly(checks, second, other.name, gw2 or -1, disp2, t0,
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
        run("ecce-broker-setup", env, "--remove")
        service.terminate()
        try:
            service.wait(timeout=30)
        except subprocess.TimeoutExpired:
            service.kill()
            service.wait()
        serviceLog.close()
        checks.check(not os.path.exists(decl) and not portOpen(sport),
                     "declaration withdrawn and the stand-in service "
                     "stopped (exit %s)" % service.returncode)


def caseMarkers(checks, display, logdir):
    """The reaper alone, on the two things that make a broker a server's.

    The broker is a stand-in (sleep, under activemq.pid), so the
    listen-beyond-loopback case needs no real outward socket.
    """
    env = display.env()
    stopOwnBroker(env)
    base = os.path.join(statedir(), "activemq")
    conf = os.path.join(base, "conf", "activemq.xml")
    marker = os.path.join(base, "server")
    os.makedirs(os.path.dirname(conf), exist_ok=True)
    saved = open(conf).read() if os.path.exists(conf) else None
    uri = 'uri="tcp://%s:%d"'
    try:
        for host, marked, survives, what in (
                ("0.0.0.0", False, True, "listening beyond loopback"),
                ("localhost", True, True, "marked a server's"),
                ("localhost", False, False, "a loopback per-user broker")):
            with open(conf, "w") as handle:
                handle.write('<transportConnector name="openwire" %s/>\n'
                             % (uri % (host, brokerPort())))
            if marked:
                open(marker, "w").close()
            fake = subprocess.Popen(["sleep", "300"],
                                    start_new_session=True)
            with open(os.path.join(statedir(), "activemq.pid"), "w") as f:
                f.write("%d\n" % fake.pid)
            said = run("ecce-gateway-reap", env, "--if-idle")
            time.sleep(0.5)
            checks.check((fake.poll() is None) == survives,
                         "%s: --if-idle %s it (%s)"
                         % (what, "leaves" if survives else "stops",
                            said.stdout.decode().strip()))
            fake.kill()
            fake.wait()
            for path in (marker, os.path.join(statedir(), "activemq.pid")):
                try:
                    os.unlink(path)
                except OSError:
                    pass
    finally:
        if saved is None:
            os.unlink(conf)
        else:
            with open(conf, "w") as handle:
                handle.write(saved)


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
        gw = gatewayIsTheTree(checks, d)
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
        endsCleanly(checks, session, d, gw, dispatcher(d), t0)
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
        for want in ("session.log", "bug-mode.txt", "activemq.log",
                     "jmsdispatcher.log", "error_log", "services.txt"):
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
            "-L" + build] + ["-l" + l for l in libs] * 3 + ["-lxerces-c"])
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
            checks.check(os.path.isdir(os.path.join(first, "users", user)),
                         "the folder has users/%s" % user)
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
         "remote-down": caseRemoteDown,
         "quit-stop": caseQuitStop,
         "displays": caseDisplays, "shared": caseShared,
         "markers": caseMarkers,
         "organizer": caseOrganizer, "builder": caseBuilder,
         "jobstore": caseJobstore}


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
