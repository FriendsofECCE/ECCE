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
import shutil
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

    def __init__(self, number=None):
        self.binary = findXvfb()
        self.number = number
        self.proc = None

    def __enter__(self):
        if self.number is None:
            self.number = _freeDisplay()
        self.proc = subprocess.Popen(
            [self.binary, ":%d" % self.number, "-screen", "0", SCREEN,
             "-nolisten", "tcp"],
            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        deadline = time.time() + 15
        while time.time() < deadline:
            if self.proc.poll() is not None:
                error = self.proc.stderr.read().decode("utf-8", "replace")
                raise DisplayUnavailable("Xvfb exited: %s" % error.strip())
            if self._ready():
                return self
            time.sleep(0.2)
        self.__exit__(None, None, None)
        raise DisplayUnavailable("Xvfb did not become ready")

    def __exit__(self, *exc):
        if self.proc is not None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()
            self.proc = None
        return False

    @property
    def name(self):
        return ":%d" % self.number

    def env(self, base=None):
        environment = dict(base if base is not None else os.environ)
        environment["DISPLAY"] = self.name
        return environment

    def _ready(self):
        try:
            return subprocess.run(["xdpyinfo", "-display", self.name],
                                  stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL,
                                  timeout=15).returncode == 0
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
        reason that has nothing to do with that app.  That is how eleven
        failures got reported for one fault in #127, and the list read
        like eleven bugs while naming none of them.
        """
        try:
            return subprocess.run(["xdpyinfo", "-display", self.name],
                                  stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL,
                                  timeout=timeout).returncode == 0
        except (FileNotFoundError, OSError, subprocess.TimeoutExpired):
            return False

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


def _freeDisplay():
    for number in range(70, 100):
        if not os.path.exists("/tmp/.X11-unix/X%d" % number):
            return number
    raise DisplayUnavailable("no free X display number")
