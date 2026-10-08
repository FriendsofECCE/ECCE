#!/usr/bin/env python3
"""
The Builder on a calculation with its MOs panel open, in every panel layout
(View > Panel layout), on a laptop-sized screen.

For each layout the Builder opens the water fixture with ECCE_PANEL_MODE,
ECCE_OPEN_PANEL=MOs and ECCE_PANEL_METRICS, and the metrics must show the
viewer and the MOs pane on screen, inside the window, at a usable size, and
every pane still a child of the frame, and the MOs pane at least as wide
as its own controls (the MO list and the Compute and Cutoff rows were cut
off at the right).  "default" sets no ECCE_PANEL_MODE,
so it is the layout a new install gets (list + detail).

The case this was written for: fitting the window to the screen (#189)
moved the AUI frame's panes into a scrolled window, after which the viewer
was laid out ~6800 px tall with the molecule off screen and the MOs panel
below the window's edge.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install, Xvfb or fixture.
"""

import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import xdisplay   # noqa: E402

LAYOUTS = [("classic", "classic"), ("stacked", "stacked"),
           ("accordion", "accordion"), ("detail", "detail"),
           ("default", "detail")]
SCREEN = "1366x768x24"
PANEL = "MOs"
MIN_VIEWER = 200      # px, each way
MIN_PANEL = 80        # px high: the MOs pane's own minimum

failures = []


def readMetrics(path):
    """{'mode': str, 'client': (w, h), 'panes': {name: (x, y, w, h, on, docked)}}"""
    out = {"mode": None, "client": None, "panes": {}}
    if not os.path.exists(path):
        return out
    with open(path) as handle:
        for line in handle:
            m = re.match(r'pane "([^"]+)" (-?\d+) (-?\d+) (\d+) (\d+) (\w+) (\w+)'
                         r'(?: need (\d+) have (\d+))?', line)
            if m:
                out["panes"][m.group(1)] = (
                    int(m.group(2)), int(m.group(3)), int(m.group(4)),
                    int(m.group(5)), m.group(6) == "onscreen",
                    m.group(7) == "docked")
                if m.group(8):
                    out.setdefault("width", {})[m.group(1)] = (
                        int(m.group(8)), int(m.group(9)))
            elif line.startswith("client "):
                out["client"] = tuple(int(v) for v in line.split()[1:3])
            elif line.startswith("mode "):
                out["mode"] = line.split()[1]
    return out


def paneProblem(metrics, name, minW, minH):
    pane = metrics["panes"].get(name)
    if pane is None:
        return "%s pane not shown" % name
    x, y, w, h, onscreen, docked = pane
    cw, ch = metrics["client"]
    if not docked:
        return "%s pane moved out of the frame" % name
    if not onscreen:
        return "%s pane is not on screen" % name
    if x < 0 or y < 0 or x + w > cw or y + h > ch:
        return ("%s pane at %dx%d+%d+%d is outside the %dx%d window"
                % (name, w, h, x, y, cw, ch))
    if w < minW or h < minH:
        return "%s pane is only %dx%d" % (name, w, h)
    return None


def clipProblem(metrics, name):
    """The pane narrower than its own controls: the ends of rows are cut."""
    need, have = metrics.get("width", {}).get(name, (0, 0))
    if need == 0:
        return "%s pane: no content width reported" % name
    if have < need:
        return ("%s pane is %d px wide but its controls need %d: the right "
                "end of its rows is clipped" % (name, have, need))
    return None


def installCalc():
    """The water fixture in the logged-in user's own home: in the fixture
    account the Builder stops at an "ECCE Authentication" dialog."""
    user = fixture.realUser()
    fixture.ensureRealUserAccount()
    target = os.path.join(fixture.stateDir(), "htdocs", "Ecce", "users",
                          user, "panel-layouts-water")
    shutil.rmtree(target, ignore_errors=True)
    shutil.copytree(os.path.join(fixture.FIXTURES, "calc-water-vib"), target,
                    symlinks=True)
    return "http://localhost:%d/Ecce/users/%s/panel-layouts-water" % (
        fixture.dataserverPort(), user)


def check(display, url, label, mode, expected):
    metricsPath = tempfile.mktemp(prefix="ecce-panels-", suffix=".txt")
    env = {"ECCE_OPEN_PANEL": PANEL, "ECCE_PANEL_METRICS": metricsPath}
    if mode != "default":
        env["ECCE_PANEL_MODE"] = mode
    authPath = fixture.authFile(
        os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"),
        user=fixture.realUser())
    #  The metrics are written 12 s after the calculation's panels exist.
    result = apps.run(display, "builder",
                      args=("-pipe", authPath, "-context", url),
                      windowTimeout=60, settle=40, env=env)
    metrics = readMetrics(metricsPath)
    try:
        os.unlink(metricsPath)
    except OSError:
        pass
    log = result.log or ""
    problem = None
    if result.crashed:
        problem = "CRASHED (%s)" % result.signalName
    elif not result.sawWindow:
        problem = "no window"
    elif metrics["client"] is None:
        problem = "no layout metrics written"
    elif metrics["mode"] != expected:
        problem = "layout %s, expected %s" % (metrics["mode"], expected)
    else:
        moved = [n for n, p in metrics["panes"].items() if not p[5]]
        problem = ("panes moved out of the frame: " + ", ".join(moved)
                   if moved else
                   paneProblem(metrics, "Viewer", MIN_VIEWER, MIN_VIEWER) or
                   paneProblem(metrics, PANEL, 1, MIN_PANEL) or
                   clipProblem(metrics, PANEL))
    if problem is None:
        need, have = metrics.get("width", {}).get(PANEL, (0, 0))
        print("ok    %s (%d px, controls need %d)" % (label, have, need))
        return
    failures.append(label)
    print("FAIL  %s: %s\n%s" % (label, problem, run_tests._tail(log, 15)))


def main():
    xdisplay.SCREEN = SCREEN

    def checkApp(display, name, results, verbose=False):
        if not fixture.available():
            results.notes.append("no calculation fixture checked in; skipped")
            return
        url = installCalc()
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            for mode, expected in LAYOUTS:
                check(display, url, "%s layout, %s panel" % (mode, PANEL),
                      mode, expected)
        finally:
            restorePrefs()
        if failures:
            results.fail("panels", "%d of %d layouts wrong: %s"
                         % (len(failures), len(LAYOUTS), ", ".join(failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
