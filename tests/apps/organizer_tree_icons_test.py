#!/usr/bin/env python3
"""The Organizer tree's icons: indent, state slot, order (regression test).

    tests/apps/organizer_tree_icons_test.py                own Xvfb, services
    tests/apps/organizer_tree_icons_test.py --shots DIR    keep the window PNG

The resource icon is in the tree's normal image list and the run state is the
tree's state image.  "treerects" reports, for each drawn node, the left edge of
its icons and where its label starts.  A child must start to the right of its
parent, a node without a run state must have no empty slot before its icon, and
a calculation (which has one) has the state slot in front of the icon.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data",
                            "project.ecce-meta")
CALC = os.path.join(HERE, "fixtures", "calc-water-opt")

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-15:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def answers(log):
    """[(command, outcome)] in the order the Organizer answered."""
    return re.findall(r"ECCE_TEST_ORGANIZER: (.+?): (ok.*|not .*|no .*)$",
                      log or "", re.M)


def nodes(outcome):
    """['0:Local data', '[1:alpha', ...] from a tree answer (labels may
    hold spaces, so split at the next depth marker)."""
    body = outcome.split(" selected=")[0]
    if not body.startswith("ok "):
        return []
    return re.findall(r"\[?\d+:.*?(?= \[?\d+:|$)", body[3:])


def labels(tree):
    return [t.lstrip("[").split(":", 1)[1] for t in tree]


def rects(outcome):
    """[(depth, label, x, textx, state)] from a treerects answer."""
    out = []
    for m in re.finditer(r" (\d+):(.*?):(-?\d+):(-?\d+):(-?\d+)(?= \d+:|$)",
                         outcome):
        out.append((int(m.group(1)), m.group(2), int(m.group(3)),
                    int(m.group(4)), int(m.group(5))))
    return out


def run(display, data, work, shot):
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, "alpha")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    calc = os.path.join(project, "water")
    shutil.copytree(CALC, calc, symlinks=True)
    commands = ["summary file://" + calc, "treerects"]
    if shot:
        commands.append("snap " + shot)
    script = os.path.join(work, "commands")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_TEST_ORGANIZER_NOLAUNCH": "1",
                           "ECCE_ORGANIZER_OPEN": "file://" + calc})
    log = result.log or ""
    what = "Organizer tree icons"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    got = answers(log)
    if len(got) < 2:
        return fail(what, "%d of 2 commands answered" % len(got), log)
    nodes_ = rects(got[1][1])
    if len(nodes_) < 3:
        return fail(what, "too few nodes drawn: %s" % got[1][1], log)
    problems = []
    # The resource icon is 16 px wide in the normal list, so no state-less
    # node carries an empty slot; a child's label (and so its icon) starts at
    # least one indent right of its parent's, and the state slot adds to it.
    if "iconw=16" not in got[1][1]:
        problems.append("normal image list is not 16 px wide: %s" % got[1][1])
    for index, (depth, label, x, textx, state) in enumerate(nodes_):
        if not index:
            continue
        prev = nodes_[index - 1]
        if depth > prev[0] and textx - prev[3] < 16:
            problems.append("%s is not indented from %s (labels at %d, %d)"
                            % (label, prev[1], textx, prev[3]))
        if depth == prev[0] + 1 and prev[4] < 0 and state >= 0 and \
                textx - prev[3] < 16 + 16:
            problems.append("%s: the state icon is not before its icon" % label)
    if not any(n[4] >= 0 for n in nodes_):
        problems.append("no node has a run state: %s" % got[1][1])
    if not any(n[4] < 0 for n in nodes_):
        problems.append("every node has a run state: %s" % got[1][1])
    if problems:
        fail(what, "; ".join(problems), log)
    else:
        print("ok    %s: %s" % (what, " ".join(
            "%d:%s@%d" % (n[0], n[1], n[2]) for n in nodes_)))
    return got


def check(display, results, verbose=False):
    """run_tests.py entry: the tree icon layout test."""
    folder = tempfile.mkdtemp(prefix="orgicons-",
                              dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(folder, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    del failures[:]
    results.checks += 1
    try:
        run(display, data, folder, None)
    finally:
        os.environ.pop("ECCE_LOCAL_DATA", None)
        shutil.rmtree(folder, ignore_errors=True)
    for failure in failures:
        results.fail("organizer tree icons", failure)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--shots", help="save the window PNG here")
    parser.add_argument("--name", default="organizer-tree-icons.png")
    options = parser.parse_args()
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="orgicons-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    shot = None
    if options.shots:
        os.makedirs(options.shots, exist_ok=True)
        shot = os.path.join(options.shots, options.name)
    with xdisplay.Display() as display:
        apps.startServices(display)
        try:
            run(display, data, work, shot)
        finally:
            apps.stopServices(display)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
