#!/usr/bin/env python3
"""Builder panes: the column keeps its width across tabs, every pane folds (#328).

    tests/apps/builder_panes_test.py            own Xvfb and services

width  Structure -> Properties -> Structure leaves the column (the Panel
       Tabs pane spans it) the same width, within TOLERANCE px.
tools  A Structure pane folds to its caption, the others grow, it unfolds
       again, and the fold survives a switch to Properties and back.
props  The same for a docked property pane (stacked layout).
Headless, through the Builder's ECCE_VIEWER_SCENE hooks (panefold,
panestate, columntab).
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps          # noqa: E402
import fixture       # noqa: E402
import pbc_ux_test   # noqa: E402

TOLERANCE = 4
CAPTION = 40          # px: a folded pane is its caption bar
STRUCTURE = ["Open_structures", "Build", "Build", "Selection",
             "Symmetry", "Log"]
NOT_PROPERTY = {"Viewer", "Panel Tabs", "Property Index", "Column Toggle",
                "Open structures", "Build", "Build", "Selection",
                "Symmetry", "Log", "Atom Table", "Residue Table"}


def lines(work, name):
    path = os.path.join(work, name + ".txt")
    return open(path).read().splitlines() if os.path.exists(path) else []


def rect(work, name, pane):
    """(w, h) of `pane` in a panestate file, from its tool line or shown row."""
    for line in lines(work, name):
        if line.startswith('tool "%s" ' % pane):
            m = re.search(r"rect (-?\d+) (-?\d+) (\d+) (\d+)", line)
            return int(m.group(3)), int(m.group(4))
    return None


def flag(work, name, pane, key):
    for line in lines(work, name):
        if line.startswith('tool "%s" ' % pane):
            m = re.search(r" %s (\d+)" % key, line)
            return int(m.group(1)) if m else None
    return None


def shownWidth(work, name, pane):
    for line in lines(work, name):
        m = re.match(r'shown "%s" rect (-?\d+) (-?\d+) (\d+) (\d+)' % pane,
                     line)
        if m:
            return int(m.group(3))
    return None


def run(display, results, label, url, script, mode):
    auth = fixture.authFile(os.path.join(fixture.stateHome(), ".ECCE",
                                         "auth.pipe"), user=fixture.USER)
    work, problems, log = pbc_ux_test.runScene(
        display, "panes-" + label, script, url,
        ("-pipe", auth, "-context", url), env={"ECCE_PANEL_MODE": mode})
    for p in problems:
        results.fail("builder panes " + label, p)
    return work


def shownNames(work, name):
    return [l.split('"')[1] for l in lines(work, name)
            if l.startswith('shown "')]


def check(display, results):
    run_url, error = pbc_ux_test.install("panes-run")
    unrun_url, error2 = pbc_ux_test.install("panes-unrun", pbc_ux_test.notRun)
    if error or error2:
        results.fail("builder panes", "cannot install fixture: %s"
                     % (error or error2))
        return

    # width: detail layout, a calculation with results
    results.checks += 1
    work = run(display, results, "width", run_url,
               ["columntab 0", "panestate a", "columntab 1", "panestate b",
                "columntab 0", "panestate c"], "detail")
    widths = []
    for name in "abc":
        w = shownWidth(work, name, "Panel Tabs")
        widths.append(w)
    if None in widths:
        results.fail("builder panes width", "no Panel Tabs pane: %s" % widths)
    elif max(widths) - min(widths) > TOLERANCE:
        results.fail("builder panes width",
                     "column width changed Structure/Properties/Structure: "
                     "%s" % widths)
    else:
        print("builder panes: column width %s (Structure, Properties, "
              "Structure)" % widths)

    # tools: a calculation not yet run opens with the building tools
    results.checks += 1
    tools = " ".join(STRUCTURE)
    work = run(display, results, "tools", unrun_url,
               ["columntab 0", "panestate before " + tools,
                "panefold Build", "panestate folded " + tools,
                "columntab 1", "columntab 0", "panestate back " + tools,
                "panefold Build", "panestate open " + tools], "detail")
    problems = foldProblems(work, "Build", "tools")
    if not problems:
        o = rect(work, "open", "Build")
        b = rect(work, "before", "Build")
        if flag(work, "open", "Build", "folded") != 0:
            problems.append("Build still folded after unfolding")
        elif not o or not b or abs(o[1] - b[1]) > 15:
            problems.append("unfolded height %s, was %s" % (o, b))
        if flag(work, "back", "Build", "folded") != 1:
            problems.append("fold lost on Structure -> Properties -> Structure")
    for p in problems:
        results.fail("builder panes tools", p)

    # props: stacked layout, the property panes shown by default
    results.checks += 1
    work = run(display, results, "props1", run_url,
               ["columntab 1", "panestate s"], "stacked")
    names = [n for n in shownNames(work, "s") if n not in NOT_PROPERTY]
    if len(names) < 2:
        results.fail("builder panes props", "need two shown property panes, "
                     "got %s" % names)
        return
    pane = names[0].replace(" ", "_")
    both = " ".join(n.replace(" ", "_") for n in names[:2])
    work = run(display, results, "props", run_url,
               ["columntab 1", "panestate before " + both,
                "panefold " + pane, "panestate folded " + both,
                "columntab 0", "columntab 1", "panestate back " + both],
               "stacked")
    problems = foldProblems(work, names[0], "props", other=names[1])
    if not problems and flag(work, "back", names[0], "folded") != 1:
        problems.append("fold lost on Properties -> Structure -> Properties")
    for p in problems:
        results.fail("builder panes props", p)

    #  Last: it floats the Build pane, and the layout is saved.
    narrow(display, results, unrun_url)


def clipProblems(work, name, label):
    """Nothing in the Build pane ends beyond its scroll window's width."""
    rows = lines(work, name)
    if not rows:
        return ["%s: no toolclip file" % label]
    need = [r for r in rows if r.startswith("need")]
    found = ["%s: %s" % (label, r) for r in rows
             if r.startswith("clipped") or r == "hscroll 1"]
    if not found:
        print("builder panes: %s Build pane unclipped (%s px, %s)"
              % (label, rows[0].split()[1], " ".join(need)))
    return found


def narrow(display, results, url):
    """A saved column narrower than the Build pane needs is raised (#328)."""
    results.checks += 1
    shots = os.environ.get("ECCE_PANES_SHOTS")
    shot = []
    if shots:
        os.makedirs(shots, exist_ok=True)
        shot = ["xshot " + os.path.join(shots, "build-pane-narrow.png")]
    work = run(display, results, "narrow", url,
               ["columntab 0", "toolclip default Build", "narrowcolumn 150",
                "toolclip narrow Build"] + shot +
               ["toolfloat Build", "toolclip floating Build"], "detail")
    for label in ("default", "narrow", "floating"):
        for p in clipProblems(work, label, label):
            results.fail("builder panes narrow", p)


def foldProblems(work, pane, label, other=None):
    """`pane` went from tall to a caption; some other pane got the height."""
    found = []
    b, f = rect(work, "before", pane), rect(work, "folded", pane)
    if not b or not f:
        return ["%s: pane %s not in the state files" % (label, pane)]
    if flag(work, "before", pane, "pin") != 1:
        found.append("%s: no fold button on %s" % (label, pane))
    if flag(work, "folded", pane, "folded") != 1:
        found.append("%s: %s not folded" % (label, pane))
    if f[1] >= CAPTION or f[1] >= b[1]:
        found.append("%s: %s is %d px high folded, was %d"
                     % (label, pane, f[1], b[1]))
    names = [other] if other else [n.replace("_", " ") for n in STRUCTURE]
    grew = 0
    for n in names:
        nb, nf = rect(work, "before", n), rect(work, "folded", n)
        if n != pane and nb and nf and nf[1] > nb[1]:
            grew += nf[1] - nb[1]
    if grew < 10:
        found.append("%s: the neighbours did not take the height (+%d px)"
                     % (label, grew))
    if not found:
        print("builder panes: %s folds %d -> %d px, neighbours +%d px"
              % (pane, b[1], f[1], grew))
    return found


class _Results(object):
    def __init__(self):
        self.failures, self.notes, self.checks = [], [], 0

    def fail(self, where, message):
        self.failures.append("%s: %s" % (where, message))


def main():
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

    results = _Results()
    xdisplay.SCREEN = "1366x768x24"
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
                check(display, results)
            finally:
                restore()
        finally:
            apps.stopServices(display)
    for failure in results.failures:
        print("FAIL  " + failure)
    if not results.failures:
        print("PASS  builder panes")
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.exit(main())
