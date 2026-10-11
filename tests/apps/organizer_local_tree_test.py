#!/usr/bin/env python3
"""The Organizer's tree in local data mode shows the projects directly (#216).

    tests/apps/organizer_local_tree_test.py                own Xvfb, services
    tests/apps/organizer_local_tree_test.py --shots DIR    keep the window PNG

The data folder holds users/local, so the tree used to read
"Local data > users > local > <projects>".  The root now is "Local data"
with the user's projects directly below it.  Display only: every URL is the
one the data layout gives.  The Organizer's ECCE_TEST_ORGANIZER hook runs:

  tree       the tree as drawn: no "users"/"local" level, projects at
             depth 1, the opened calculation selected at depth 2
  summary    selecting a calculation by its URL
  up, home   the toolbar buttons; Up stops at the root
  newproject File > New Project: appears at depth 1 and is selected

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


def run(display, data, work, shot):
    root = os.path.join(data, "users", "local")
    for name in ("alpha", "beta"):
        project = os.path.join(root, name)
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    calc = os.path.join(root, "alpha", "water")
    shutil.copytree(CALC, calc, symlinks=True)
    rootUrl = "file://" + root
    commands = [
        "tree",
        "summary file://" + calc,
        "tree",
        "up",
        "up",
        "tree",
        "home",
        "newproject gamma",
        "tree",
    ]
    if shot:
        commands.append("snap " + shot)
    script = os.path.join(work, "commands")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_ORGANIZER_OPEN": "file://" + calc})
    log = result.log or ""
    what = "Organizer tree, local data"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    got = answers(log)
    if len(got) < 9:
        return fail(what, "%d of %d commands answered" % (len(got), 9), log)
    problems = []

    tree = nodes(got[0][1])
    names = labels(tree)
    if names[:1] != ["Local data"] or "users" in names or "local" in names:
        problems.append("tree is %s, expected Local data with no users/local"
                        % names)
    if not tree or not tree[0].startswith("0:"):
        problems.append("first node is not at depth 0")
    for project in ("alpha", "beta"):
        if "1:" + project not in tree and "[1:" + project not in tree:
            problems.append("project %s not at depth 1: %s" % (project, tree))

    if "[2:water" not in nodes(got[2][1]):
        problems.append("the calculation is not the selected node: %s"
                        % nodes(got[2][1]))
    if "selected=file://%s/water" % (root + "/alpha") not in got[2][1]:
        problems.append("selected url wrong: %s" % got[2][1])

    # Up from a calculation goes to its project, then to the root, and stops.
    if not got[3][1].rstrip("/").endswith("selected=%s/alpha" % rootUrl):
        problems.append("Up from the calculation: %s" % got[3][1])
    if not got[4][1].rstrip("/").endswith("selected=%s" % rootUrl):
        problems.append("Up from the project: %s" % got[4][1])
    if "up=no" not in got[5][1] or "[0:Local data" not in nodes(got[5][1]):
        problems.append("the root should be selected with Up disabled: %s"
                        % got[5][1])
    if not got[6][1].rstrip("/").endswith("selected=%s" % rootUrl):
        problems.append("Home: %s" % got[6][1])

    final = nodes(got[8][1])
    if "[1:gamma" not in final:
        problems.append("new project gamma not selected at depth 1: %s"
                        % final)
    if "selected=%s/gamma" % rootUrl not in got[8][1]:
        problems.append("new project url: %s" % got[8][1])
    if not os.path.isdir(os.path.join(root, "gamma")):
        problems.append("gamma was not made under users/local")

    if problems:
        fail(what, "; ".join(problems), log)
    else:
        print("ok    %s: %s" % (what, " ".join(labels(tree))))
    return got


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--shots", help="save the window PNG here")
    parser.add_argument("--name", default="organizer-local-tree.png")
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
    work = tempfile.mkdtemp(prefix="orgtree-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    shot = None
    if options.shots:
        os.makedirs(options.shots, exist_ok=True)
        shot = os.path.join(options.shots, options.name)
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            run(display, data, work, shot)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
