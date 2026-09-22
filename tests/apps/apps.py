"""
Launch one installed ECCE GUI app headlessly and report what happened.

This is the tier neither existing suite reaches.  `tests/parsers` stops at a
parser script's stdout and `tests/dialogs` at the codereg dialogs; the
wx3.2/GTK3 bugs that have cost this port the most time live in the C++ apps
and are invisible to both:

  * #78 -- `wxGrid::CreateGrid()` synchronously firing
    `wxEVT_GRID_SELECT_CELL` during construction, into a handler whose
    dependencies did not exist yet.  Segfaulted `builder` on opening any job
    with vibrational data.
  * the layout reentrancy class documented in CLAUDE.md, where
    `DoSetSize` -> `wxEVT_SIZE` -> `Layout()` recurses, sometimes crashing
    *after* `Show()` has already returned.

Both are "the app dies on startup or shortly after", which is exactly what
can be checked without a human: start it, wait for a window, then keep
watching for a while, because the asynchronous variant does not crash until
after the window is up.

Apps are launched through their `/usr/bin/ecce-<name>` wrapper rather than
the binary in `$ECCE_HOME/bin`, because the wrapper is what sets ECCE_HOME,
ECCE_REALUSER and the GDK_BACKEND workaround, and what starts the background
services.  Running the binary directly is explicitly not supported.
"""

import os
import signal
import subprocess
import time

WRAPPERS = "/usr/bin"
INSTALL = "/opt/ecce"


class Result(object):
    def __init__(self, name):
        self.name = name
        self.windows = []
        self.log = ""
        self.returncode = None
        self.crashed = False
        self.sawWindow = False
        self.secondsToWindow = None
        self.note = None

    @property
    def signalName(self):
        if self.returncode is None or self.returncode >= 0:
            return None
        try:
            return signal.Signals(-self.returncode).name
        except ValueError:
            return "signal %d" % -self.returncode


def installed(name):
    return os.access(os.path.join(WRAPPERS, "ecce-" + name), os.X_OK)


def guiBinaries():
    """Installed binaries that link wxWidgets, i.e. the GUI apps.

    Discovered rather than listed so a newly added app is covered the moment
    it is packaged -- the same principle as tests/dialogs reading the .edml
    files instead of keeping its own code table.
    """
    found = []
    binDir = os.path.join(INSTALL, "bin")
    if not os.path.isdir(binDir):
        return found
    for entry in sorted(os.listdir(binDir)):
        path = os.path.join(binDir, entry)
        if not os.access(path, os.X_OK) or os.path.isdir(path):
            continue
        if not installed(entry):
            continue
        result = subprocess.run(["ldd", path], stdout=subprocess.PIPE,
                                stderr=subprocess.DEVNULL)
        if b"libwx_gtk3" in result.stdout:
            found.append(entry)
    return found


def startServices(display, log=None):
    """Bring up the gateway and data server once, before any app runs.

    Every wrapper does this itself on first launch, but doing it here keeps
    the ~10s of broker startup out of the first app's window timeout, where
    it would look like that app being slow.
    """
    for script in ("ecce-dataserver-start", "ecce-gateway-start"):
        # ECCE_GATEWAY_START / ECCE_DATASERVER_START point at an alternative
        # copy of the script.  This exists because /opt/ecce is root-owned:
        # a fix to a service script cannot be tried out without a packaging
        # round-trip otherwise, and the JMSDispatcher per-display bug this
        # suite found lives in exactly those scripts.  Normal runs leave
        # these unset and use what is installed.
        override = os.environ.get(script.upper().replace("-", "_"))
        path = override or os.path.join(INSTALL, "bin", script)
        if not os.access(path, os.X_OK):
            continue
        result = subprocess.run([path], env=display.env(),
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=180)
        if log is not None:
            log.append("%s: %s" % (script, result.stdout.decode(
                "utf-8", "replace").strip().replace("\n", " | ")))


def serviceState():
    states = {}
    for script, key in (("ecce-gateway-status", "gateway"),
                        ("ecce-dataserver-status", "dataserver")):
        path = os.path.join(INSTALL, "bin", script)
        if not os.access(path, os.X_OK):
            continue
        result = subprocess.run([path], stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        text = result.stdout.decode("utf-8", "replace")
        states[key] = "not running" not in text
    return states


def stopServices():
    for script in ("ecce-gateway-stop", "ecce-dataserver-stop"):
        path = os.path.join(INSTALL, "bin", script)
        if os.access(path, os.X_OK):
            subprocess.run([path], stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL, timeout=120)


def run(display, name, args=(), windowTimeout=40, settle=8):
    """Start ecce-<name>, wait for a window, watch it, then shut it down."""
    result = Result(name)
    before = set(wid for wid, _ in display.windows())

    command = [os.path.join(WRAPPERS, "ecce-" + name)] + list(args)
    logFile = subprocess.PIPE
    proc = subprocess.Popen(command, env=display.env(), stdout=logFile,
                            stderr=subprocess.STDOUT,
                            start_new_session=True)

    started = time.time()
    deadline = started + windowTimeout
    while time.time() < deadline:
        if proc.poll() is not None:
            # Exited before ever showing a window.
            result.returncode = proc.returncode
            result.log = _drain(proc)
            result.crashed = proc.returncode is not None and proc.returncode < 0
            return result
        new = [(wid, title) for wid, title in display.windows()
               if wid not in before]
        if new:
            result.sawWindow = True
            result.secondsToWindow = round(time.time() - started, 1)
            result.windows = new
            break
        time.sleep(0.5)

    # Keep watching: the layout-reentrancy class of bug crashes well after
    # the window is up, which a "did it start?" check alone would miss.
    endSettle = time.time() + settle
    while time.time() < endSettle:
        if proc.poll() is not None:
            result.returncode = proc.returncode
            result.log = _drain(proc)
            result.crashed = True
            result.note = ("exited %s after its window appeared"
                           % (result.signalName or
                              "with status %d" % proc.returncode))
            return result
        time.sleep(0.5)

    result.windows = [(wid, title) for wid, title in display.windows()
                      if wid not in before]
    _terminate(proc)
    result.returncode = proc.returncode
    result.log = _drain(proc)
    return result


def _terminate(proc):
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
    except (ProcessLookupError, PermissionError):
        pass
    try:
        proc.wait(timeout=15)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
        except (ProcessLookupError, PermissionError):
            pass
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            pass


def _drain(proc):
    try:
        if proc.stdout is None:
            return ""
        return proc.stdout.read().decode("utf-8", "replace")
    except Exception:
        return ""
