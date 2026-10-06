"""
A headless X server with working OpenGL, for the C++ GUI apps.

`tests/dialogs` gets away with `broadwayd`, GTK3's displayless backend,
because the codereg dialogs are plain widgets.  The C++ apps are not: every
one of them links GLX (`libGLX`, `libwx_gtk3u_gl`) because the molecular
viewer is Open Inventor, and broadway offers no GLX at all.  So this needs a
real X server -- Xvfb, whose GLX is served by Mesa's llvmpipe software
rasteriser.  Confirmed working here: `direct rendering: Yes`, renderer
`llvmpipe (LLVM 19.1.7)`.

Xvfb is not installed by default on a Debian desktop (it is not pulled in by
any desktop metapackage), so:

  * `apt install xvfb` is the normal answer;
  * `ECCE_XVFB=/path/to/Xvfb` is honoured, which also covers the
    no-root case -- `apt-get download xvfb && dpkg -x xvfb_*.deb ./root`
    produces a working binary at `root/usr/bin/Xvfb` with no installation;
  * with neither, the suite SKIPS rather than fails.

Deliberately NOT falling back to $DISPLAY: these apps open full-size windows
and start background services, which is not something to do on somebody's
live desktop by accident.
"""

import os
import select
import shutil
import signal
import subprocess
import time

SCREEN = "1280x1024x24"


class DisplayUnavailable(Exception):
    pass


def findXvfb():
    explicit = os.environ.get("ECCE_XVFB")
    if explicit:
        if not os.access(explicit, os.X_OK):
            raise DisplayUnavailable("ECCE_XVFB=%s is not executable"
                                     % explicit)
        return explicit
    found = shutil.which("Xvfb")
    if found:
        return found
    raise DisplayUnavailable(
        "Xvfb not found.  Install it (apt install xvfb), or point ECCE_XVFB "
        "at one -- `apt-get download xvfb && dpkg -x xvfb_*.deb ./root` gives "
        "a working binary with no root needed.")


class Display(object):
    """An Xvfb instance, and the X queries the suite needs against it."""

    def __init__(self, number=None, pidfile=None):
        self.binary = findXvfb()
        self.number = number
        self.proc = None
        #  Where this instance's own pid is recorded, so a NEXT run of the
        #  suite -- one that starts because this one was killed rather than
        #  torn down cleanly -- can find and stop it.  Only ever a path
        #  under the suite's own isolated state directory (see
        #  killStaleXvfb / isolate.py); left None for --use-real-state,
        #  which does not get this leak-recovery behaviour at all.
        self.pidfile = pidfile

    def __enter__(self):
        argv = [self.binary]
        passFds = ()
        reader = None
        if self.number is None and not _displayRange():
            #  The server picks and locks a free number itself and reports
            #  it, so two runs cannot race for one.
            reader, writer = os.pipe()
            argv += ["-displayfd", str(writer)]
            passFds = (writer,)
        elif self.number is None:
            self.number = _freeDisplay()
        if self.number is not None:
            argv.append(":%d" % self.number)
        argv += ["-screen", "0", SCREEN, "-nolisten", "tcp"]
        self.proc = subprocess.Popen(
            argv, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
            pass_fds=passFds)
        if reader is not None:
            os.close(writer)
            try:
                self.number = self._readDisplayNumber(reader)
            finally:
                os.close(reader)
        if self.pidfile:
            #  Best-effort: a failure to write this only means a killed run
            #  cannot be swept up next time, not that this run cannot work.
            try:
                with open(self.pidfile, "w") as handle:
                    handle.write(str(self.proc.pid))
            except OSError:
                pass
        deadline = time.time() + _scaled(15)
        while time.time() < deadline:
            if self.proc.poll() is not None:
                error = self.proc.stderr.read().decode("utf-8", "replace")
                raise DisplayUnavailable("Xvfb exited: %s" % error.strip())
            if self._ready():
                return self
            time.sleep(0.2)
        self.__exit__(None, None, None)
        raise DisplayUnavailable("Xvfb did not become ready")

    def _readDisplayNumber(self, fd):
        deadline = time.time() + _scaled(30)
        data = b""
        while b"\n" not in data and time.time() < deadline:
            ready, _, _ = select.select([fd], [], [], 0.5)
            if ready:
                chunk = os.read(fd, 64)
                if not chunk:
                    break
                data += chunk
            elif self.proc.poll() is not None:
                break
        if b"\n" not in data:
            error = ""
            if self.proc.poll() is not None:
                error = self.proc.stderr.read().decode("utf-8", "replace")
            else:
                self.proc.kill()
            self.proc = None
            raise DisplayUnavailable(
                "Xvfb did not report a display number: %s" % error.strip())
        return int(data.split()[0])

    def __exit__(self, *exc):
        if self.proc is not None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()
            self.proc = None
        if self.pidfile:
            try:
                os.remove(self.pidfile)
            except OSError:
                pass
        return False

    @property
    def name(self):
        return ":%d" % self.number

    def env(self, base=None):
        environment = dict(base if base is not None else os.environ)
        environment["DISPLAY"] = self.name
        #  One ECCE session per test display (#233), unless a caller
        #  replaces it: what an app launched by the gateway would inherit.
        if not hasattr(self, "sessionId"):
            self.sessionId = os.urandom(8).hex()
        environment["ECCE_SESSION_ID"] = self.sessionId
        #  No accessibility bus on a CI runner: every GTK app there logs
        #  "AT-SPI: Error retrieving accessibility bus address" and can
        #  stall on it at exit -- msgdialog, which returns from OnInit at
        #  once, sat for ~70 s until the suite killed it, and the display
        #  was unresponsive afterwards (#127).  Nothing here needs AT-SPI.
        environment.setdefault("NO_AT_BRIDGE", "1")
        environment.setdefault("GTK_A11Y", "none")
        return environment

    def _ready(self):
        try:
            return subprocess.run(["xdpyinfo", "-display", self.name],
                                  stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL,
                                  timeout=_scaled(15)).returncode == 0
        except subprocess.TimeoutExpired:
            return False

    def windows(self):
        """Top-level windows as [(id, title), ...].

        Read with xwininfo rather than a Python X binding so the suite keeps
        its "python3 and nothing else" property; x11-utils is already a
        dependency of any desktop that can run ECCE at all.
        """
        #  TIMEOUT IS LOAD-BEARING.  This is polled twice a second while
        #  waiting for an app's window, and it talks to the X server.  An
        #  app that takes an X grab -- a modal, a menu, a drag -- and then
        #  wedges leaves xwininfo blocking on that connection forever, so
        #  an unbounded call here hangs the whole suite rather than the
        #  one app.  That is what held a CI runner for 40 minutes per run:
        #  every bound inside run() was respected, because the suite never
        #  got back into run().
        #
        #  Returning nothing on a timeout is the right answer for the
        #  caller: "no windows visible", which lets that app's own
        #  window-timeout expire and be reported as a failure.
        try:
            result = subprocess.run(
                ["xwininfo", "-display", self.name, "-root", "-tree"],
                stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                timeout=20)
        except subprocess.TimeoutExpired:
            return []
        found = []
        for line in result.stdout.decode("utf-8", "replace").splitlines():
            line = line.strip()
            if not line.startswith("0x"):
                continue
            parts = line.split(None, 1)
            if len(parts) < 2 or not parts[1].startswith('"'):
                continue
            title = parts[1].split('"')[1]
            found.append((parts[0], title))
        return found

    def responsive(self, timeout=10):
        """Is the X server still answering at all?

        Asked after every app, because a display that has stopped
        answering makes every app after it fail identically and for a
        reason that has nothing to do with that app (#127).  A loaded
        machine answers slowly rather than not at all (#220), so the
        timeout grows with the load and a miss is retried before the
        server is called dead.  `probeNote` says how long it waited.
        """
        waited = 0.0
        for attempt in range(3):
            limit = _scaled(timeout) * (attempt + 1)
            started = time.time()
            try:
                if subprocess.run(["xdpyinfo", "-display", self.name],
                                  stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL,
                                  timeout=limit).returncode == 0:
                    return True
            except (FileNotFoundError, OSError):
                return False
            except subprocess.TimeoutExpired:
                pass
            waited += time.time() - started
            if self.proc is not None and self.proc.poll() is not None:
                break           # the process is gone: no point waiting
            time.sleep(1)
        self.probeNote = "waited %.0fs over %d probe(s), load average %.1f" % (
            waited, attempt + 1, _load())
        return False

    probeNote = ""

    def serverState(self):
        """Whether the X server process itself is still alive.

        The first question to ask when the display stops answering, and
        the one with the shortest answer: an Xvfb that died and an Xvfb
        that is wedged look identical from the client side and want
        completely different investigations.
        """
        if self.proc is None:
            return "no Xvfb of ours"
        code = self.proc.poll()
        return "Xvfb running" if code is None else "Xvfb exited (%s)" % code

    def clients(self):
        """Whatever can still be said about who is holding the server.

        Diagnostic only, and best-effort: xlsclients is in x11-utils with
        the rest, but a missing tool must never be what decides whether a
        fault gets reported.
        """
        try:
            result = subprocess.run(["xlsclients", "-display", self.name],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.DEVNULL, timeout=10)
        except (FileNotFoundError, OSError, subprocess.TimeoutExpired):
            return ""
        return result.stdout.decode("utf-8", "replace").strip()

    def hasGL(self):
        """True when GLX is answering. Unknown counts as "probably fine".

        glxinfo lives in mesa-utils, which is not installed everywhere, and
        this is a diagnostic rather than a requirement -- the apps
        themselves will fail loudly if they really cannot get a context. A
        missing tool must not take the whole suite down, which is exactly
        what it did on its first CI run.
        """
        try:
            result = subprocess.run(["glxinfo", "-display", self.name, "-B"],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.DEVNULL,
                                    timeout=30)
        except (FileNotFoundError, OSError, subprocess.TimeoutExpired):
            return None
        return b"direct rendering: Yes" in result.stdout


def killStaleXvfb(pidfile):
    """Stop an Xvfb THIS SUITE started on a previous, killed run.

    Xvfb's own command line never mentions the suite's state directory --
    unlike the broker/dataserver, which killLeftovers() (isolate.py) finds
    that way -- so it needs its own record: the pid `Display.__enter__`
    wrote to `pidfile` last time it started one.  Before believing that
    pid is still an Xvfb worth killing, re-check `/proc/<pid>/cmdline` --
    pids get recycled, and killing whatever some other process has become
    would not be a leak fix, it would be a new bug.  Returns the pid killed,
    or None if there was nothing to do.
    """
    try:
        with open(pidfile) as handle:
            pid = int(handle.read().strip())
    except (OSError, ValueError):
        return None
    try:
        with open("/proc/%d/cmdline" % pid, "rb") as handle:
            cmdline = handle.read()
    except OSError:
        _removeQuietly(pidfile)
        return None
    if b"Xvfb" not in cmdline:
        _removeQuietly(pidfile)
        return None

    try:
        os.kill(pid, signal.SIGTERM)
    except ProcessLookupError:
        _removeQuietly(pidfile)
        return None

    deadline = time.time() + 5
    while time.time() < deadline and os.path.exists("/proc/%d" % pid):
        time.sleep(0.2)
    if os.path.exists("/proc/%d" % pid):
        try:
            os.kill(pid, signal.SIGKILL)
        except ProcessLookupError:
            pass

    _removeQuietly(pidfile)
    return pid


def _removeQuietly(path):
    try:
        os.remove(path)
    except OSError:
        pass


def _load():
    try:
        return os.getloadavg()[0]
    except OSError:
        return 0.0


def _scaled(seconds):
    """`seconds`, stretched when the machine is busier than it has CPUs."""
    return seconds * max(1.0, _load() / (os.cpu_count() or 1))


def _displayRange():
    span = os.environ.get("ECCE_TEST_XDISPLAYS", "")
    if "-" in span:
        return tuple(int(n) for n in span.split("-", 1))
    return None


def _freeDisplay():
    #  Only with ECCE_TEST_XDISPLAYS=160-169, to stay off other sessions'
    #  numbers; otherwise Xvfb -displayfd chooses.
    first, last = _displayRange() or (70, 99)
    for number in range(first, last + 1):
        if not os.path.exists("/tmp/.X11-unix/X%d" % number):
            return number
    raise DisplayUnavailable("no free X display number")
