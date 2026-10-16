#!/usr/bin/env python3
"""Windows keep painting under Mutter's Xwayland (GTK 3 frame sync).

    tests/apps/xwayland_paint_test.py

GTK 3 freezes a top-level window's frame clock after each frame until the
compositor sends _NET_WM_FRAME_DRAWN; Mutter under XWayland does not always
send it, and the window then stops repainting while it still takes input
(dialogs that show no typed text, a Builder view that does not show the atom
just added).  ewxModalGuard turns frame sync off on XWayland.

This runs a private headless gnome-shell (Mutter with its own Xwayland, its
own D-Bus session, nothing shared with the desktop) in place of Xvfb, and
checks that the GDK frame counter advances: the Organizer's text prompt
("busyprompt", ECCE_DEBUG_MODAL "settled" line) and the Builder's window
around an atom added in Atom mode ("info" prints frame=).  The headless
shell sends no _NET_WM_FRAME_DRAWN at all, so with frame sync on
(run with ECCE_FRAME_SYNC=1) both stay at frame 0 or 1 and the test fails.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install, gnome-shell or
Xwayland.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data",
                            "project.ecce-meta")
STRUCTURE = os.path.join(ROOT, "tests", "fragreaders", "data", "benzene.xyz")

failures = []


def check(what, ok, why=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       "" if ok else ": " + why))
    if not ok:
        failures.append(what)


class ShellDisplay(xdisplay.Display):
    """A headless gnome-shell; its Xwayland display stands in for Xvfb."""

    def __init__(self):
        self.number = None
        self.proc = None
        self.pidfile = None
        self.log = None

    def __enter__(self):
        # Short: the Wayland socket path must fit in 108 bytes.
        self.runtime = tempfile.mkdtemp(prefix="ecw", dir="/tmp")
        self.home = tempfile.mkdtemp(prefix="ecce-shell-home-")
        env = dict(os.environ)
        for var in ("DBUS_SESSION_BUS_ADDRESS", "WAYLAND_DISPLAY", "DISPLAY"):
            env.pop(var, None)
        env.update(HOME=self.home, XDG_RUNTIME_DIR=self.runtime,
                   XDG_CONFIG_HOME=os.path.join(self.home, ".config"),
                   XDG_DATA_HOME=os.path.join(self.home, ".local", "share"),
                   XDG_CACHE_HOME=os.path.join(self.home, ".cache"))
        out = subprocess.run(["dbus-daemon", "--session", "--fork",
                              "--print-address=1", "--print-pid=1"],
                             env=env, stdout=subprocess.PIPE, timeout=30)
        address, pid = out.stdout.decode().split()[:2]
        self.dbusPid = int(pid)
        env["DBUS_SESSION_BUS_ADDRESS"] = address
        self.log = tempfile.TemporaryFile()
        self.proc = subprocess.Popen(
            ["gnome-shell", "--headless", "--wayland", "--virtual-monitor",
             "1600x1100", "--wayland-display", "wl-ecce-test"],
            env=env, stdout=self.log, stderr=subprocess.STDOUT)
        socket = os.path.join(self.runtime, "wl-ecce-test")
        deadline = time.time() + 60
        while time.time() < deadline and self.number is None:
            time.sleep(0.5)
            if self.proc.poll() is not None:
                break
            if not os.path.exists(socket):
                continue
            # Mutter writes its own pid into the lock of the X display it
            # serves (Xwayland starts on the first connection).
            for name in os.listdir("/tmp"):
                m = re.match(r"\.X(\d+)-lock$", name)
                if not m:
                    continue
                try:
                    with open(os.path.join("/tmp", name)) as handle:
                        if int(handle.read().strip()) == self.proc.pid:
                            self.number = int(m.group(1))
                except (OSError, ValueError):
                    pass
        auth = [f for f in os.listdir(self.runtime)
                if f.startswith(".mutter-Xwaylandauth")]
        if self.number is None or not auth:
            self.__exit__(None, None, None)
            raise xdisplay.DisplayUnavailable("headless gnome-shell gave no "
                                              "Xwayland display")
        self.xauth = os.path.join(self.home, "xauth")
        shutil.copy(os.path.join(self.runtime, auth[0]), self.xauth)
        self.savedAuth = os.environ.get("XAUTHORITY")
        os.environ["XAUTHORITY"] = self.xauth
        deadline = time.time() + 30
        while time.time() < deadline and not self._ready():
            time.sleep(0.5)
        return self

    def __exit__(self, *exc):
        if self.proc is not None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.proc.kill()
            self.proc = None
        if getattr(self, "dbusPid", None):
            try:
                os.kill(self.dbusPid, 15)
            except OSError:
                pass
            # Services the private bus started (portals, gvfs) write into
            # the runtime directory as they go.
            time.sleep(2)
        if hasattr(self, "savedAuth"):
            if self.savedAuth is None:
                os.environ.pop("XAUTHORITY", None)
            else:
                os.environ["XAUTHORITY"] = self.savedAuth
        for path in (getattr(self, "runtime", None), getattr(self, "home", None)):
            if path:
                shutil.rmtree(path, ignore_errors=True)
        if self.log is not None:
            self.log.close()
            self.log = None
        return False


def organizerPrompt(display, work):
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local", "alpha"))
    shutil.copy(PROJECT_META,
                os.path.join(data, "users", "local", "alpha", ".ecce-meta"))
    script = os.path.join(work, "organizer.commands")
    with open(script, "w") as handle:
        handle.write("busyprompt\n")
    modal = os.path.join(work, "modal.log")
    apps.run(display, "organizer", windowTimeout=60, settle=20,
             env={"ECCE_TEST_ORGANIZER": script, "ECCE_DEBUG_MODAL": modal,
                  "ECCE_LOCAL_DATA": data})
    trace = open(modal).read() if os.path.exists(modal) else ""
    settled = re.search(r"settled 'Busy Prompt' draws=(\d+) .*frame=(-?\d+) "
                        r"frameSync=(\w+)", trace)
    check("the prompt was shown", settled is not None, trace[-600:])
    if settled:
        check("frame sync is off for the prompt", settled.group(3) == "off",
              settled.group(0))
        check("the prompt keeps painting (frame %s, %s draws)"
              % (settled.group(2), settled.group(1)),
              int(settled.group(2)) >= 5, settled.group(0))


def builderView(display, work):
    script = os.path.join(work, "builder.script")
    with open(script, "w") as handle:
        handle.write("\n".join(["wait 3000", "info", "element O", "click",
                                "wait 1500", "info"]) + "\n")
    result = apps.run(display, "builder", args=(STRUCTURE,),
                      windowTimeout=60, settle=25,
                      env={"ECCE_BUILDER_SCRIPT": script,
                           "ECCE_TEST_XYZ_UNITS": "angstrom"})
    log = result.log or ""
    frames = [int(f) for f in re.findall(r"BUILDER: frame=(-?\d+)", log)]
    atoms = re.findall(r"BUILDER: atoms=(\d+)", log)
    check("the Builder answered twice", len(frames) == 2 and len(atoms) == 2,
          log[-600:])
    if len(frames) == 2 and len(atoms) == 2:
        check("an atom was added", int(atoms[1]) > int(atoms[0]), str(atoms))
        check("the Builder window repaints after it (frame %d -> %d)"
              % tuple(frames), frames[1] - frames[0] >= 3 and frames[1] >= 5)


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return SKIP
    for tool in ("gnome-shell", "Xwayland", "dbus-daemon", "xdpyinfo"):
        if not shutil.which(tool):
            print("SKIP: no %s" % tool)
            return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    # The desktop's own compositor must never be reached.
    os.environ.pop("WAYLAND_DISPLAY", None)
    os.environ["ECCE_NO_REAP"] = "1"
    work = tempfile.mkdtemp(prefix="xwpaint-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    try:
        with ShellDisplay() as display:
            log = []
            apps.startServices(display, log)
            try:
                organizerPrompt(display, work)
                builderView(display, work)
            finally:
                apps.stopServices(display)
    except xdisplay.DisplayUnavailable as exc:
        print("SKIP: %s" % exc)
        return SKIP
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
