#!/usr/bin/env python3
"""The Periodic Builder pane and the Builder's layout choice, headless.

    tests/apps/pbc_ux_test.py [--png DIR] [--only REGEX]

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS); exit 77 (skip) without an install or Xvfb.

pane   In each panel layout (classic, stacked, accordion, detail), on a
       1366x768 screen: Tools > Periodic Builder opens a docked pane inside
       the window with a close button and a ticked menu item; closing it
       with its close button hides it and unticks the item; the menu item
       opens it again, docked.  The pane's controls get their full width.
layout A calculation that has not been run opens with the building tools
       (Build) shown and is not read-only; one with results opens in the
       viewing layout (no Build), in the layouts that have a Structure tab
       and in classic.
"""
import argparse
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402

BASE = os.path.join(HERE, "fixtures", "calc-water-vib")
MODES = ("classic", "stacked", "accordion", "detail")
SCREEN = "1366x768x24"
PBC = "Periodic_Builder"
MARKERS = ("ended by SIG", "Segmentation fault", "double free or corruption",
           "terminate called", "Unhandled standard exception")
TOOLS = ["Periodic_Builder", "Build", "Selection", "Atom_Table", "Symmetry"]


def readState(path):
    """{'tools': {name: {key: int}}, 'shown': [names], 'client': (w, h),
    'readonly': int}"""
    out = {"tools": {}, "shown": [], "client": None, "readonly": None}
    if not os.path.exists(path):
        return None
    for line in open(path):
        m = re.match(r'tool "([^"]+)" (\d) shown (\d) floating (\d) close (\d) '
                     r'child (\d) ticked (\d) rect (-?\d+) (-?\d+) (\d+) (\d+) '
                     r'need (\d+) have (\d+)', line)
        if m:
            g = [int(v) for v in m.groups()[1:]]
            out["tools"][m.group(1)] = dict(
                exists=g[0], shown=g[1], floating=g[2], close=g[3], child=g[4],
                ticked=g[5], x=g[6], y=g[7], w=g[8], h=g[9], need=g[10],
                have=g[11])
        elif line.startswith("shown "):
            out["shown"].append(line.split('"')[1])
        elif line.startswith("client "):
            out["client"] = tuple(int(v) for v in line.split()[1:3])
        elif line.startswith("readonly "):
            out["readonly"] = int(line.split()[1])
    return out


def paneProblems(st, label, want):
    """Problems with the Periodic Builder pane in state `st`."""
    if st is None:
        return ["%s: no state written" % label]
    t = st["tools"].get("Periodic Builder")
    if not t or not t["exists"]:
        return ["%s: no Periodic Builder pane" % label]
    found = []
    if not want:
        if t["shown"]:
            found.append("%s: still shown" % label)
        if t["ticked"]:
            found.append("%s: menu item still ticked" % label)
        return found
    cw, ch = st["client"]
    if not t["shown"]:
        found.append("%s: not shown" % label)
    if t["floating"]:
        found.append("%s: floating" % label)
    if not t["child"]:
        found.append("%s: window left the frame" % label)
    if not t["close"]:
        found.append("%s: no close button" % label)
    if not t["ticked"]:
        found.append("%s: menu item not ticked" % label)
    if t["x"] < 0 or t["y"] < 0 or t["x"] + t["w"] > cw or t["y"] + t["h"] > ch:
        found.append("%s: pane %dx%d+%d+%d outside the %dx%d window"
                     % (label, t["w"], t["h"], t["x"], t["y"], cw, ch))
    if t["have"] < t["need"]:
        found.append("%s: controls need %d px, the pane gives %d"
                     % (label, t["need"], t["have"]))
    return found


def runScene(display, name, script, url, args, env=None, timeout=90):
    work = os.path.join(fixture.stateHome(), "pbc-ux", name)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    path = os.path.join(work, "scene")
    with open(path, "w") as handle:
        handle.write("\n".join(script + [""]))
    environment = {"ECCE_VIEWER_SCENE": path, "ECCE_VIEWER_SCENE_OUT": work,
                   "ECCE_VIEWER_SCENE_HOLD": "1",
                   "ECCE_REALUSER": fixture.USER}
    environment.update(env or {})
    result = apps.run(display, "builder", args=args, windowTimeout=120,
                      settle=timeout, env=environment)
    log = result.log or ""
    problems = []
    if result.crashed:
        problems.append("builder CRASHED (%s)" % result.signalName)
    for m in MARKERS:
        if m in log:
            problems.append("output contains %r" % m)
    failed = os.path.join(work, "FAILED")
    if os.path.exists(failed):
        problems.append("script stopped: " + open(failed).read().strip())
    elif not result.exitedAfterWindow:
        problems.append("builder did not finish the script in %ds" % timeout)
    return work, problems, log


def install(name, mutate=None):
    calc = os.path.join(fixture.stateHome(), "pbc-ux", name)
    shutil.rmtree(calc, ignore_errors=True)
    shutil.copytree(BASE, calc, symlinks=True)
    if mutate:
        mutate(calc)
    url, error = fixture.install(name, source_dir=calc)
    return url, error


def notRun(calc):
    """The water calculation as a new one: no results, state Created."""
    shutil.rmtree(os.path.join(calc, "Props"))
    os.makedirs(os.path.join(calc, "Props"))
    db = os.path.join(calc, ".DAV", ".state_for_dir")
    data = open(db, "rb").read()
    #  Same length as "Complete": the metadata file is a paged database.
    assert b"Complete" in data
    open(db, "wb").write(data.replace(b"Complete", b"Created "))


def check(display, results, png=None, only=None):
    state = fixture.stateHome()
    auth = fixture.authFile(os.path.join(state, ".ECCE", "auth.pipe"),
                            user=fixture.USER)
    url, error = install("pbc-ux-run")
    if error:
        results.fail("pbc ux", "cannot install fixture: " + error)
        return
    for mode in MODES:
        if only and not re.search(only, "pane " + mode):
            continue
        results.checks += 1
        shot = []
        if png:
            os.makedirs(png, exist_ok=True)
            shot = ["xshot " + os.path.join(png, "pbc-%s.png" % mode)]
        script = (["panelmode " + mode, "toolmenu %s on" % PBC,
                   "pbcpress create"] + shot +
                  ["panestate open " + " ".join(TOOLS)] +
                  ["paneclose " + PBC, "panestate closed " + " ".join(TOOLS),
                   "toolmenu %s on" % PBC, "panestate reopen " + " ".join(TOOLS)])
        args = ("-pipe", fixture.authFile(
            os.path.join(state, ".ECCE", "auth.pipe"), user=fixture.USER),
            "-context", url)
        work, problems, log = runScene(display, "pane-" + mode, script, url,
                                       args)
        for label, want in (("open", True), ("closed", False),
                            ("reopen", True)):
            problems += paneProblems(readState(os.path.join(work,
                                     label + ".txt")),
                                     "%s layout, %s" % (mode, label), want)
        for p in problems:
            results.fail("pbc ux pane", p)
        if problems:
            results.notes.append("\n".join(log.splitlines()[-15:]))

    unrun, error = install("pbc-ux-unrun", notRun)
    if error:
        results.fail("pbc ux", "cannot install fixture: " + error)
        return
    for mode in ("classic", "detail", "stacked"):
        for name, calc, building in (("unrun", unrun, True),
                                     ("run", url, False)):
            if only and not re.search(only, "layout %s %s" % (mode, name)):
                continue
            results.checks += 1
            args = ("-pipe", fixture.authFile(
                os.path.join(state, ".ECCE", "auth.pipe"), user=fixture.USER),
                "-context", calc)
            work, problems, log = runScene(
                display, "layout-%s-%s" % (name, mode),
                ["panestate state " + " ".join(TOOLS)], calc, args,
                env={"ECCE_PANEL_MODE": mode})
            st = readState(os.path.join(work, "state.txt"))
            label = "%s calculation, %s layout" % (name, mode)
            if st is None:
                problems.append(label + ": no state written")
            else:
                if building:
                    if st["readonly"] != 0:
                        problems.append(label + ": opened read-only")
                    if "Build" not in st["shown"]:
                        problems.append(label + ": Build is not shown (%s)"
                                        % ", ".join(st["shown"]))
                else:
                    if st["readonly"] != 1:
                        problems.append(label + ": not read-only")
                    if "Build" in st["shown"]:
                        problems.append(label + ": Build shown for a run "
                                        "calculation")
            for p in problems:
                results.fail("pbc ux layout", p)
            if problems:
                results.notes.append("\n".join(log.splitlines()[-15:]))
    fixture.remove("pbc-ux-run")
    fixture.remove("pbc-ux-unrun")


class _Results(object):
    def __init__(self):
        self.failures, self.notes, self.checks = [], [], 0

    def fail(self, where, message):
        self.failures.append("%s: %s" % (where, message))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--png", help="write one screenshot per layout here")
    parser.add_argument("--only")
    args = parser.parse_args()

    import isolate
    import xdisplay

    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    if not apps.installed("builder"):
        print("SKIP  no installed builder")
        return 77
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"
    xdisplay.SCREEN = SCREEN

    results = _Results()
    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        try:
            if not all(apps.serviceState(display).values()):
                print("FAIL  services did not come up: %s" % log)
                return 1
            fixture.ensureRealUserAccount()
            restore = fixture.settleUpgradeNotices()
            try:
                check(display, results, args.png, args.only)
            finally:
                restore()
        finally:
            apps.stopServices(display)
    for note in results.notes:
        print("  " + note)
    for failure in results.failures:
        print("FAIL  " + failure)
    if not results.failures:
        print("PASS  pbc ux (%d checks)" % results.checks)
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.exit(main())
