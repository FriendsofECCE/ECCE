"""Keep a run of this suite out of a real, live ECCE session.

A test run brings up the same two per-user services a person's session uses
-- the per-user mosquitto broker, and the per-user Apache that is the data
server -- and those services are keyed by state on disk and by fixed
ports.  Overlapping with a developer's own running ECCE therefore does not
produce a tidy "port in use" error.  It produces two brokers contending for
one `~/.ECCE/mosquitto.sock`, a data server that early-exits
because "something is already listening" and then serves somebody else's
document root, and a pile of app failures that read exactly like application
bugs.  That is the class of false failure that makes a suite untrustworthy,
so this module exists to make the overlap impossible rather than unlikely.

Four things have to move together, and the reason this was got wrong before
is that they are in four different places:

  * **the state directory** -- `$ECCE_REALUSERHOME/.ECCE`, which holds the
    preferences, the broker files, the mosquitto socket and the
    data server's entire document root.  Both the C++ (`Ecce::realUserHome`)
    and every service script honour `ECCE_REALUSERHOME`.  `ECCE_TEST_STATE`
    was documented as the way to move it -- but nothing ever exported it as
    `ECCE_REALUSERHOME`, so it moved only `fixture.py`'s idea of where the
    state was, and not one of the services.
  * **the ports** -- `ECCE_DATASERVER_PORT`, read by the service scripts,
    and `ECCE_BROKER_PORT`, the port of a central broker under -remote (the
    per-user broker is on a Unix socket in the state directory).
  * **`siteconfig/DataServers`** -- which is where the *apps* learn the data
    server's URL, and it is written at package time with a hardcoded
    `http://localhost:8096/Ecce`.  Moving the port without moving this makes
    the apps talk to whatever is on 8096, i.e. to the real session, which is
    worse than not moving the port at all.  It lives under `$ECCE_HOME`,
    which is root-owned, so it is moved by giving the run its own `ECCE_HOME`
    -- a directory of symlinks to the installed tree with one real
    `siteconfig/` of its own.
  * **`ECCE_HELP`**, hardcoded to port 8096 in the app wrappers for the same
    reason.

Everything is applied to `os.environ`, so it reaches the service scripts,
`ecce-dataserver-adduser` and every app launched afterwards without any of
them needing to know about the suite.
"""

import os
import atexit
import re
import shutil
import signal
import socket
import tempfile
import time

#  Ports and state directories are per run, never fixed: two runs of a
#  suite (two worktrees, or ctest beside a manual run) must not share
#  either.  The OS hands out the ports (bind to 0); the state directory is
#  a fresh mkdtemp under ~/.cache (not /tmp, which is RAM).
class IsolationError(Exception):
    pass


_MARKER = ".ecce-test-run"
_handedOut = set()
_defaults = {}


def _cacheDir():
    return (os.environ.get("XDG_CACHE_HOME")
            or os.path.join(os.path.expanduser("~"), ".cache"))


def _startTime(pid):
    try:
        with open("/proc/%d/stat" % pid) as handle:
            return handle.read().rsplit(")", 1)[1].split()[19]
    except (OSError, IndexError):
        return None


def _ownerAlive(state):
    try:
        with open(os.path.join(state, _MARKER)) as handle:
            pid, started = handle.read().split()
        return _startTime(int(pid)) == started
    except (OSError, ValueError):
        return True     # not recognisably ours or unreadable: leave alone


def reapDead(prefix):
    """Remove the run directories of killed runs, and what they left running.

    A run that is killed (ctest timeout, SIGKILL) never reaches its
    cleanup.  Only directories carrying this module's own marker, whose
    recorded owner process is gone, are touched.
    """
    cache = _cacheDir()
    try:
        names = os.listdir(cache)
    except OSError:
        return
    for name in names:
        path = os.path.join(cache, name)
        if (name.startswith(prefix) and os.path.isfile(
                os.path.join(path, _MARKER)) and not _ownerAlive(path)):
            killLeftovers(path)
            _killXvfbOf(path)
            shutil.rmtree(path, ignore_errors=True)


def _killXvfbOf(state):
    try:
        import xdisplay
        xdisplay.killStaleXvfb(os.path.join(state, "xvfb.pid"))
    except Exception:
        pass


def keepState():
    return bool(os.environ.get("ECCE_TEST_KEEP_STATE"))


def runState(tag, keep=False):
    """A state directory of this run's own, removed at exit unless kept.

    `ECCE_TEST_STATE` still wins, and is never removed: it is the caller's.
    Repeated calls with one tag in a process return the same directory.
    """
    explicit = os.environ.get("ECCE_TEST_STATE")
    if explicit:
        return os.path.abspath(os.path.expanduser(explicit))
    if tag in _defaults:
        return _defaults[tag]
    cache = _cacheDir()
    os.makedirs(cache, exist_ok=True)
    reapDead("ecce-%s-" % tag)
    state = tempfile.mkdtemp(prefix="ecce-%s-" % tag, dir=cache)
    with open(os.path.join(state, _MARKER), "w") as handle:
        handle.write("%d %s\n" % (os.getpid(), _startTime(os.getpid())))
    _defaults[tag] = state

    def cleanup():
        if keep or keepState():
            print("state kept: %s" % state)
            return
        killLeftovers(state)
        _killXvfbOf(state)
        shutil.rmtree(state, ignore_errors=True)
    atexit.register(cleanup)
    return state


def defaultStateDir(tag="apps"):
    return runState(tag)


def freePort():
    """A port the OS has just offered, not yet handed out in this process."""
    for _ in range(50):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.bind(("127.0.0.1", 0))
            port = sock.getsockname()[1]
        if port not in _handedOut:
            _handedOut.add(port)
            return port
    raise IsolationError("the OS offered no unused port")


def _portFree(port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(0.5)
        return sock.connect_ex(("127.0.0.1", port)) != 0


def _pickPort(name, preferred=None):
    """The port named by the environment variable `name`, else a free one.

    An explicitly requested port is used as given: if it is busy, say so
    rather than quietly serving on a different one than the caller arranged
    the rest of their environment for.  `preferred` is unused, kept for
    callers written when ports were fixed.
    """
    explicit = os.environ.get(name)
    if explicit:
        port = int(explicit)
        if not _portFree(port):
            raise IsolationError(
                "%s=%d, but something is already listening there.  Stop it, "
                "or leave %s unset and a free port will be chosen."
                % (name, port, name))
        _handedOut.add(port)
        return port
    return freePort()


def _link(source, target):
    #  A state directory outlives the install it was made for, so an existing
    #  link to a different install must be replaced, or the run silently
    #  tests whatever the old link points at.
    if os.path.islink(target):
        if os.readlink(target) == source:
            return
        os.unlink(target)
    elif os.path.lexists(target):
        return
    os.symlink(source, target)


def homeOverlay(install, state, dataserverPort):
    """An `$ECCE_HOME` that differs from the installed one only in siteconfig.

    `siteconfig/DataServers` names the data server's port, and it lives in
    a root-owned install that a test cannot edit.  Symlinking everything else and owning a real
    `siteconfig/` costs a 128K copy and makes the whole of `$ECCE_HOME`
    honest about which instance this run is talking to.
    """
    home = os.path.join(state, "ecce-home")
    siteconfig = os.path.join(home, "siteconfig")
    if os.path.isdir(siteconfig):
        shutil.rmtree(siteconfig)
    os.makedirs(home, exist_ok=True)
    for entry in sorted(os.listdir(install)):
        if entry == "siteconfig":
            continue
        _link(os.path.join(install, entry), os.path.join(home, entry))
    shutil.copytree(os.path.join(install, "siteconfig"), siteconfig,
                    symlinks=True)

    _rewrite(os.path.join(siteconfig, "DataServers"),
             (r"(<(?:Url|BasisSet)>\s*http://[^:<\s]+):\d+",
              r"\1:%d" % dataserverPort),
             expect=":%d" % dataserverPort)
    return home


def _rewrite(path, substitution, expect):
    """Apply one regex substitution to a config file and CHECK it took.

    A silent no-op here is the worst possible outcome: the run would look
    isolated and the apps would go on talking to port 8096, which is the
    real session.  So the result is verified rather than assumed.
    """
    if not os.path.exists(path):
        raise IsolationError("%s is missing from the installed siteconfig"
                             % path)
    pattern, replacement = substitution
    with open(path) as handle:
        original = handle.read()
    rewritten, count = re.subn(pattern, replacement, original)
    if not count or expect not in rewritten:
        raise IsolationError(
            "could not repoint %s at this run's own ports -- its contents do "
            "not match %r, so the apps would still be talking to the real "
            "session" % (path, pattern))
    with open(path, "w") as handle:
        handle.write(rewritten)


def resolveStateDir(state=None):
    """The state directory this run will use (made on first use).

    Split out of apply() so a caller can find it before the run's own
    services exist.
    """
    state = state or runState("apps")
    return os.path.abspath(os.path.expanduser(state))


def killLeftovers(state):
    """Stop processes still holding THIS run's own state directory.

    A run that is killed (a ctest timeout, ^C, SIGTERM) never reaches its
    `finally:` block, so the mosquitto brokers, the per-user apache2 and
    whatever else the services scripts started are left running -- and
    because they are per-user services keyed by state on disk, the next
    run collides with them: "httpd already running", "broker did not come
    up within 30s", every app then reporting no window.

    Matched by STATE DIRECTORY PATH in `/proc/<pid>/cmdline`, deliberately
    never by process name -- "mosquitto"/"apache2" also names a real user's
    live session on their own, real `~/.ECCE`, and this must never be able
    to touch that.  A process whose command line does not mention this
    exact, isolated state directory is left alone, unconditionally.
    """
    #  The path must end where the match does: a plain substring test
    #  lets the default state directory match a sibling that extends its
    #  name (ecce-apps-suite vs session_end.py's ecce-apps-suite-session),
    #  and a run started beside another then kills the other's services.
    needle = re.compile(re.escape(os.fsencode(state.rstrip("/")))
                        + rb"(?=/|\0|$)")
    self_pid = os.getpid()
    victims = []
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        pid = int(entry)
        if pid == self_pid:
            continue
        try:
            with open("/proc/%s/cmdline" % entry, "rb") as handle:
                cmdline = handle.read()
        except OSError:
            continue        # gone already, or not ours to read
        if needle.search(cmdline):
            victims.append(pid)

    if not victims:
        return None

    for pid in victims:
        try:
            os.kill(pid, signal.SIGTERM)
        except ProcessLookupError:
            pass

    deadline = time.time() + 5
    remaining = set(victims)
    while remaining and time.time() < deadline:
        remaining = set(pid for pid in remaining
                        if os.path.exists("/proc/%d" % pid))
        if remaining:
            time.sleep(0.2)

    for pid in remaining:
        try:
            os.kill(pid, signal.SIGKILL)
        except ProcessLookupError:
            pass

    return ("stopped %d leftover process(es) under %s left by a previous "
            "run that did not shut down cleanly"
            % (len(victims), state))


def apply(install, state=None):
    """Redirect this process's environment at a private ECCE instance.

    Returns a dict of what was set, for the run's own log.  Every value goes
    into `os.environ`, so every subprocess inherits it.
    """
    state = resolveStateDir(state)

    real = os.path.realpath(os.path.expanduser("~"))
    if os.path.realpath(state) == real:
        raise IsolationError(
            "the isolated state directory is the real home directory (%s); "
            "that is the collision this is meant to prevent" % state)

    dataserverPort = _pickPort("ECCE_DATASERVER_PORT")
    brokerPort = _pickPort("ECCE_BROKER_PORT")

    os.makedirs(os.path.join(state, ".ECCE"), exist_ok=True)
    home = homeOverlay(install, state, dataserverPort)

    settings = {
        "ECCE_REALUSERHOME": state,
        "ECCE_TEST_STATE": state,
        "ECCE_DATASERVER_PORT": str(dataserverPort),
        "ECCE_BROKER_PORT": str(brokerPort),
        "ECCE_HOME": home,
        "ECCE_HELP": "http://localhost:%d/" % dataserverPort,
        # The first-start question comes at every start (#240); only the
        # cases about it clear this.
        "ECCE_NO_FIRST_START": "1",
    }
    os.environ.update(settings)
    return settings


def describe(settings):
    return ("isolated: state %s, data server :%s, broker :%s"
            % (settings["ECCE_REALUSERHOME"],
               settings["ECCE_DATASERVER_PORT"],
               settings["ECCE_BROKER_PORT"]))
