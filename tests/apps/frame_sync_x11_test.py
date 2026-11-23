#!/usr/bin/env python3
"""Windows keep painting when the compositor drops _NET_WM_FRAME_DRAWN on a
plain X server, not only under XWayland (#1249).

    tests/apps/frame_sync_x11_test.py [run_tests.py options]

GTK 3 freezes a top-level window's frame clock after each frame until the
compositor answers with _NET_WM_FRAME_DRAWN, with no timeout.  On a loaded
Xorg GNOME session Mutter sometimes does not answer: the first Viewer was
up but drawn greyed and the "Starting the Viewer..." window kept its text
until a focus change made Mutter redraw everything.

Xvfb has no window manager, so GTK never waits there.  This test stands a
window manager in on the Xvfb that advertises _NET_WM_FRAME_DRAWN (as Mutter
does) and never sends it: the limit of a compositor that drops replies.
The Organizer opens a Viewer with its start held back (ECCE_TEST_TOOL_DELAY),
so the Starting window shows with its pulsing gauge, and "toolstate" reports
the GDK frame counter of every shown window once a second.  Checks that the
main window keeps painting while the Viewer tab comes up and that the
Starting window paints whenever a sample finds it up.  A control round with
ECCE_FRAME_SYNC=1 (GTK's default frame sync) must stall, or the stand-in
did not reproduce the freeze and the test proves nothing.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).
"""
import ctypes
import ctypes.util
import os
import re
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import run_tests  # noqa: E402
import progress_box_test as pb  # noqa: E402

failures = []
EXAMPLES = os.path.join(HERE, "..", "..", "data", "client", "examples")
DELAY_MS = 8000


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       "" if ok else ": " + detail))
    if not ok:
        failures.append(what)


class SilentCompositor(object):
    """Claims _NET_WM_FRAME_DRAWN on the X screen and never sends it."""

    def __init__(self, number):
        x = ctypes.CDLL(ctypes.util.find_library("X11"))
        x.XOpenDisplay.restype = ctypes.c_void_p
        x.XOpenDisplay.argtypes = [ctypes.c_char_p]
        x.XDefaultRootWindow.restype = ctypes.c_ulong
        x.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
        x.XCreateSimpleWindow.restype = ctypes.c_ulong
        x.XCreateSimpleWindow.argtypes = [
            ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_int,
            ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, ctypes.c_ulong,
            ctypes.c_ulong]
        x.XInternAtom.restype = ctypes.c_ulong
        x.XInternAtom.argtypes = [ctypes.c_void_p, ctypes.c_char_p,
                                  ctypes.c_int]
        x.XChangeProperty.argtypes = [
            ctypes.c_void_p, ctypes.c_ulong, ctypes.c_ulong, ctypes.c_ulong,
            ctypes.c_int, ctypes.c_int, ctypes.c_void_p, ctypes.c_int]
        x.XFlush.argtypes = [ctypes.c_void_p]
        x.XCloseDisplay.argtypes = [ctypes.c_void_p]
        self.x = x
        self.dpy = x.XOpenDisplay((":%d" % number).encode())
        if not self.dpy:
            raise RuntimeError("cannot open display :%d" % number)
        root = x.XDefaultRootWindow(self.dpy)
        check_win = x.XCreateSimpleWindow(self.dpy, root, -10, -10, 1, 1,
                                          0, 0, 0)

        def atom(name):
            return x.XInternAtom(self.dpy, name.encode(), 0)

        def prop(win, name, kind, values):
            data = (ctypes.c_ulong * len(values))(*values)
            x.XChangeProperty(self.dpy, win, atom(name), kind, 32, 0,
                              ctypes.cast(data, ctypes.c_void_p), len(values))
        XA_ATOM, XA_WINDOW = 4, 33
        prop(check_win, "_NET_SUPPORTING_WM_CHECK", XA_WINDOW, [check_win])
        name = b"ecce-silent-compositor"
        x.XChangeProperty(self.dpy, check_win, atom("_NET_WM_NAME"),
                          atom("UTF8_STRING"), 8, 0,
                          ctypes.cast(ctypes.c_char_p(name), ctypes.c_void_p),
                          len(name))
        prop(root, "_NET_SUPPORTING_WM_CHECK", XA_WINDOW, [check_win])
        prop(root, "_NET_SUPPORTED", XA_ATOM,
             [atom(n) for n in ("_NET_SUPPORTED", "_NET_SUPPORTING_WM_CHECK",
                                "_NET_WM_NAME", "_NET_WM_SYNC_REQUEST",
                                "_NET_WM_FRAME_DRAWN",
                                "_NET_WM_FRAME_TIMINGS")])
        x.XFlush(self.dpy)

    def close(self):
        # The check window goes with the connection, and so does the claim.
        self.x.XCloseDisplay(self.dpy)


def frames(state):
    return {m.group(1): int(m.group(2))
            for m in re.finditer(r"frames\['([^']*)'\]=(-?\d+)", state)}


def run(display, label, frameSync):
    data = tempfile.mkdtemp(prefix="ecce-framesync-data-")
    work = tempfile.mkdtemp(prefix="ecce-framesync-")
    try:
        project = os.path.join(data, "users", "local", "framesync")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "f_etcl_sn2_irc")
        shutil.copytree(os.path.join(
            EXAMPLES, "organic", "substitution-vs-elimination",
            "ecceqm-f_etcl_sn2_irc", "stored", "f_etcl_sn2_irc"), calc,
            symlinks=True)
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            time.sleep(5)
            append("toolstate", 1)
            append("start CalculationViewer file://" + calc, 1)
            # One a second: the hook reads the file only when the main loop
            # is free, so the samples that land while the Starting window
            # is up cannot be chosen in advance.
            for _ in range(int(DELAY_MS / 1000) + 12):
                append("toolstate", 1)
            append("menu exit", 3)
            append("answer ok", 20)

        env = {"ECCE_ONE_PROCESS": "1", "ECCE_TEST_ORGANIZER": cmds,
               "ECCE_LOCAL_DATA": data, "ECCE_TEST_TOOL_DELAY": str(DELAY_MS)}
        if frameSync:
            env["ECCE_FRAME_SYNC"] = "1"
        result = apps.run(display, "organizer", windowTimeout=60, settle=5,
                          env=env, inspect=inspect)
        log = result.log or ""
        states = pb.answers(log, "toolstate")
        for s in states:
            print(label, "toolstate:", s)
        check(not result.crashed, label + ": no crash",
              result.signalName or log[-1500:])
        if len(states) < 3:
            check(False, label + ": toolstate answers", str(states))
            return None
        samples = [frames(s) for s in states]
        mains = [t for t in samples[0] if t != "Starting"]
        main = mains[0] if mains else None
        final = [f for t, f in samples[-1].items() if t != "Starting"]
        mainPaints = (final[0] - samples[0][main]
                      if main is not None and final else None)
        start = [f["Starting"] for f in samples if "Starting" in f]
        print(label, "main window frames over the Viewer's start:",
              mainPaints, "; Starting window frame counters:", start)
        return {"startFrames": start, "mainPaints": mainPaints,
                "closed": "window=none" in states[-1]}
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        wm = SilentCompositor(display.number)
        try:
            control = run(display, "frame sync on (control)", True)
            fixed = run(display, "default", False)
        finally:
            wm.close()
        if control:
            check((control["mainPaints"] or 0) < 3 and
                  all(f <= 1 for f in control["startFrames"]),
                  "control: the silent compositor freezes GTK windows "
                  "with frame sync on", str(control))
        if fixed:
            check((fixed["mainPaints"] or 0) >= 3,
                  "the main window repaints while the Viewer tab comes up",
                  str(fixed))
            # Shown from 1.5 s until the held-back start ends.
            check(all(f >= 2 for f in fixed["startFrames"]) and
                  (len(fixed["startFrames"]) < 2 or
                   fixed["startFrames"][-1] > fixed["startFrames"][0]),
                  "the Starting window paints while it is up", str(fixed))
            check(fixed["closed"], "the Starting window closed", str(fixed))
        if failures:
            results.fail("frame-sync-x11", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
