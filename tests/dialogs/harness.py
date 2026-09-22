"""
Headless driver for the ``scripts/codereg`` dialogs.

Each codereg dialog is a standalone process with a narrow, already-existing
contract (see ``scripts/codereg/globals.py``):

    argv:  <restoreFile> <udpPort> <GUIValues|NO_GUIValues> <Writable|...>
           <DebugOn|DebugOff> <category> <theory> <runtype> <calcName>
           <rxnFlag> <symmetry> <nElec> <mult> <nFrozen> <nOcc> <nVirt>
           <nModes> [chainedMetadynamics]
    stdin: nothing
    in:    the restore file, a GUIValues dump (name|value|unit|...)
    out:   one ``name|value|unit|enabled|export|type`` line per changed
           widget, over UDP to <udpPort>

That is a testable interface as it stands -- no new "test mode" and no
changes to shipped code are needed to drive it.  This module supplies the
argv, optionally a restore file, binds the UDP socket, and runs the dialog
under a headless display.

Headless display, in order of preference:

  * ``broadwayd`` -- GTK3's own displayless backend.  Needs no X server at
    all, and is present on a stock Debian desktop (``libgtk-3-bin``), so
    this suite has the same "runs on any dev box and in CI" property as
    tests/parsers.  Verified to run wxPython 4.2.3 / wxWidgets 3.2.7.
  * ``xvfb-run`` -- the conventional choice, if installed.
  * ``$DISPLAY`` -- last resort, and only when ECCE_DIALOG_TESTS_USE_DISPLAY
    is set, because it flashes real windows on a real desktop.

If none is available the runner skips rather than fails.
"""

import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
CODEREG = os.path.join(REPO, "scripts", "codereg")
SHIM = os.path.join(HERE, "_introspect.py")

# argv positions, from globals.py's Globals.__init__ -- named so a change
# there shows up here as a rename rather than a silent off-by-one.
ARGV_FIELDS = (
    "restoreFile", "port", "restoreFlag", "readOnlyFlag", "debugFlag",
    "category", "theory", "runType", "calculationName", "reactionStudyFlag",
    "symmetryGroup", "numElectrons", "spinMultiplicity", "numFrozenOrbs",
    "numOccupiedOrbs", "numVirtualOrbs", "numNormalModes",
)

DEFAULTS = {
    "restoreFlag": "NO_GUIValues",
    "readOnlyFlag": "Writable",
    "debugFlag": "DebugOff",
    "category": "SCF",
    "theory": "RHF",
    "runType": "Energy",
    "calculationName": "harness",
    "reactionStudyFlag": "0",
    "symmetryGroup": "C1",
    "numElectrons": "10",
    "spinMultiplicity": "1",
    "numFrozenOrbs": "1",
    "numOccupiedOrbs": "5",
    "numVirtualOrbs": "2",
    "numNormalModes": "3",
}


class HarnessUnavailable(Exception):
    """No headless display available; the caller should skip, not fail."""


class DialogError(Exception):
    pass


class Display(object):
    """A headless display for the lifetime of a test run."""

    def __init__(self, debug=False):
        # When set, every dialog is launched with DebugOn, which makes
        # globals.py/templates.py emit their [ecce-dialog] troubleshooting
        # lines on stderr -- including why a restored value was skipped.
        self.debug = debug
        self.env = {}
        self.wrapper = []
        self.kind = None
        self._proc = None

    def __enter__(self):
        if shutil.which("broadwayd"):
            self._startBroadway()
        elif shutil.which("xvfb-run"):
            self.kind = "xvfb-run"
            self.wrapper = ["xvfb-run", "-a"]
        elif (os.environ.get("DISPLAY")
              and os.environ.get("ECCE_DIALOG_TESTS_USE_DISPLAY")):
            self.kind = "DISPLAY=%s" % os.environ["DISPLAY"]
        else:
            raise HarnessUnavailable(
                "no headless display: install broadwayd (libgtk-3-bin) or "
                "xvfb, or set ECCE_DIALOG_TESTS_USE_DISPLAY=1 to use $DISPLAY "
                "(opens real windows on your desktop)")
        return self

    def _startBroadway(self):
        # Pick a display number nobody is using; broadwayd listens on
        # 8080+N, so probe that rather than guessing.
        for number in range(30, 60):
            if _portFree(8080 + number):
                break
        else:
            raise HarnessUnavailable("no free broadway display number")
        self._proc = subprocess.Popen(
            ["broadwayd", ":%d" % number],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        deadline = time.time() + 10
        while time.time() < deadline:
            if _portFree(8080 + number):
                if self._proc.poll() is not None:
                    raise HarnessUnavailable("broadwayd exited immediately")
                time.sleep(0.1)
                continue
            break
        else:
            raise HarnessUnavailable("broadwayd did not start")
        self.kind = "broadwayd :%d" % number
        self.env = {
            "GDK_BACKEND": "broadway",
            "BROADWAY_DISPLAY": ":%d" % number,
            "DISPLAY": "",
        }

    def __exit__(self, *exc):
        if self._proc is not None:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self._proc.kill()
        return False


def _portFree(port):
    probe = socket.socket()
    try:
        probe.connect(("127.0.0.1", port))
    except OSError:
        return True
    finally:
        probe.close()
    return False


def buildArgv(**overrides):
    unknown = set(overrides) - set(ARGV_FIELDS)
    if unknown:
        raise ValueError("unknown dialog argument(s): %s"
                         % ", ".join(sorted(unknown)))
    values = dict(DEFAULTS)
    values.update({k: str(v) for k, v in overrides.items()})
    return [values.get(field, "") for field in ARGV_FIELDS]


def runDialog(display, script, restore=None, timeout=90, **overrides):
    """Run one codereg dialog headlessly and return what it built.

    ``script`` is a bare name (``ged16theory.py``) or a path.  ``restore`` is
    an optional list of GUIValues dump lines; passing it switches the dialog
    into ``GUIValues`` restore mode automatically.

    Returns the parsed inventory dict: every widget the dialog constructed,
    with its GUIValues ``name``, its live choice list, default and value,
    plus every line the dialog sent back over the socket.
    """
    if display.debug:
        overrides.setdefault("debugFlag", "DebugOn")
    path = script if os.path.isabs(script) else os.path.join(CODEREG, script)
    if not os.path.exists(path):
        raise DialogError("no such dialog script: %s" % path)

    workdir = tempfile.mkdtemp(prefix="ecce-dialog-")
    try:
        restorePath = os.path.join(workdir, "restore.in")
        if restore is not None:
            overrides.setdefault("restoreFlag", "GUIValues")
            with open(restorePath, "w") as handle:
                for line in restore:
                    handle.write(line.rstrip("\n") + "\n")
                handle.write("END_GUIValues\n")
        else:
            open(restorePath, "w").close()
        overrides["restoreFile"] = restorePath

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(("127.0.0.1", 0))
        sock.settimeout(0)
        overrides["port"] = sock.getsockname()[1]

        outPath = os.path.join(workdir, "inventory.json")
        env = dict(os.environ)
        env.update(display.env)
        env["ECCE_HARNESS_SCRIPT"] = path
        env["ECCE_HARNESS_OUT"] = outPath
        env["PYTHONPATH"] = CODEREG + os.pathsep + env.get("PYTHONPATH", "")
        env["PYTHONDONTWRITEBYTECODE"] = "1"

        argv = [sys.executable, SHIM] + buildArgv(**overrides)
        try:
            proc = subprocess.run(
                display.wrapper + argv, env=env, cwd=CODEREG,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                timeout=timeout)
        except subprocess.TimeoutExpired:
            raise DialogError("%s did not exit within %ds -- a dialog that "
                              "blocks here is a real bug (Finalize() is "
                              "already neutered by the shim)"
                              % (os.path.basename(path), timeout))

        if not os.path.exists(outPath):
            raise DialogError(
                "%s produced no inventory (exit %d)\n--- stderr ---\n%s"
                % (os.path.basename(path), proc.returncode,
                   proc.stderr.decode("utf-8", "replace").strip()))
        with open(outPath) as handle:
            inventory = json.load(handle)

        inventory["stdout"] = proc.stdout.decode("utf-8", "replace")
        inventory["stderr"] = proc.stderr.decode("utf-8", "replace")
        inventory["returncode"] = proc.returncode
        inventory["emitted"] = _drain(sock)
        sock.close()
        return inventory
    finally:
        shutil.rmtree(workdir, ignore_errors=True)


def _drain(sock):
    lines = []
    while True:
        try:
            chunk = sock.recv(65536)
        except OSError:
            break
        for line in chunk.decode("utf-8", "replace").splitlines():
            if line:
                lines.append(line)
    return lines


def choiceWidgets(inventory):
    """Every widget in an inventory that offers a fixed list of strings."""
    found = []
    for rec in inventory["widgets"]:
        live = rec.get("live") or {}
        choices = live.get("choices")
        if not choices:
            continue
        found.append({
            "name": live.get("name") or rec["args"].get("name"),
            "choices": choices,
            "default": live.get("default"),
            "selection": live.get("selection"),
            "label": rec["args"].get("label", ""),
            "line": rec.get("line"),
            "widget": rec["widget"],
        })
    return found
