#!/usr/bin/env python3
"""Duplicate and Continue from Last Geometry add the copy
under its project and select it; nothing else in the tree moves.

    tests/apps/organizer_duplicate_test.py

It used to reload the project (every expanded sibling project collapsed and
the selection fell to the root) and then select the copy.  The Organizer's
ECCE_TEST_ORGANIZER "duplicate" command runs the menu item and reports the
tree's selection and collapse events and the tree afterwards.

A copy's state (#1264): a stored example (imported, state Loaded) and a
completed calculation give a ready copy with an input deck; one whose
settings are incomplete (no basis set) stays created.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
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
EXAMPLE = os.path.join(ROOT, "data", "client", "examples", "genchem",
                       "molecular-orbitals-of-diatomics", "ecceqm-LiF",
                       "stored", "LiF")
# original in project gamma -> the state its Duplicate must have
EXPECTED = {"lif": "Ready", "h2o": "Ready", "incomplete": "Created"}

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-15:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def run(display, data, work):
    root = os.path.join(data, "users", "local")
    for name in ("alpha", "beta"):
        project = os.path.join(root, name)
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        shutil.copytree(CALC, os.path.join(project, "water"), symlinks=True)
    gamma = os.path.join(root, "gamma")
    os.makedirs(gamma)
    shutil.copy(PROJECT_META, os.path.join(gamma, ".ecce-meta"))
    shutil.copytree(CALC, os.path.join(gamma, "h2o"), symlinks=True)
    shutil.copytree(EXAMPLE, os.path.join(gamma, "lif"), symlinks=True)
    shutil.copytree(EXAMPLE, os.path.join(gamma, "incomplete"), symlinks=True)
    os.remove(os.path.join(gamma, "incomplete", "Parameters",
                           "BasisSet.ecce_basisset"))
    url = "file://" + root
    # beta is opened (expanded) first; duplicating in alpha must not close it.
    commands = ["summary %s/beta/water" % url,
                "duplicate %s/alpha/water" % url,
                "duplicate %s/alpha/water last" % url]
    commands += ["duplicate %s/gamma/%s" % (url, n) for n in sorted(EXPECTED)]
    script = os.path.join(work, "commands")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_ORGANIZER_OPEN": "%s/alpha/water" % url})
    log = result.log or ""
    what = "Organizer duplicate"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    check_states(gamma, log)
    got = re.findall(r"ECCE_TEST_ORGANIZER: (duplicate \S+/alpha/.+?): (ok.*)$",
                     log, re.M)
    if len(got) < 2:
        return fail(what, "%d of 2 commands answered" % len(got), log)
    for n, (command, outcome) in enumerate(got):
        events, tree = outcome[3:].split(" |", 1)
        names = re.findall(r"\[?\d+:\S+", tree)
        problems = []
        if "collapsed" in events or "sel:root" in events:
            problems.append("tree moved: %s" % events.strip())
        selected = [x for x in names if x.startswith("[")]
        if len(selected) != 1 or not selected[0].startswith("[2:water"):
            problems.append("copy not the selection: %s" % tree)
        if sum(1 for x in names if x.lstrip("[").startswith("2:water")) \
                != 3 + n:
            problems.append("projects do not hold the copies: %s" % tree)
        if "[1:alpha" in names or "1:alpha" not in names:
            problems.append("alpha not expanded below the root: %s" % tree)
        if "1:beta" not in names:
            problems.append("beta lost: %s" % tree)
        if problems:
            fail(what + " " + command, "; ".join(problems), log)
        else:
            print("ok    %s: %s" % (command, events.strip() or "no events"))


def state_of(calc):
    try:
        with open(os.path.join(calc, ".ecce-meta"), errors="replace") as handle:
            found = re.search(r"ecce:state\t+([^\t\n]+)", handle.read())
    except OSError:
        return "no .ecce-meta"
    return found.group(1).strip() if found else "no state"


def check_states(gamma, log):
    copies = sorted(set(os.listdir(gamma)) - set(EXPECTED) - {".ecce-meta"})
    for original, want in sorted(EXPECTED.items()):
        mine = [c for c in copies if c.startswith(original)]
        what = "copy of %s" % original
        if len(mine) != 1:
            fail(what, "expected one copy, found %s" % (mine or "none"), log)
            continue
        copy = os.path.join(gamma, mine[0])
        state = state_of(copy)
        inputs = os.path.join(copy, "Inputs")
        deck = os.path.isdir(inputs) and any(
            os.path.getsize(os.path.join(inputs, f)) > 0
            for f in os.listdir(inputs))
        if state != want:
            fail(what + " is %s, not %s" % (state, want), "wrong state", log)
        elif want == "Ready" and not deck:
            fail(what + " is Ready with no input deck", "no deck", log)
        else:
            print("ok    %s: %s%s" % (what, state,
                                      ", with a deck" if deck else ""))


def main():
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
    work = tempfile.mkdtemp(prefix="orgdup-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            run(display, data, work)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
