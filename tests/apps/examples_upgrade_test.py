#!/usr/bin/env python3
"""The first start after an upgrade replaces the flat example projects of
an earlier version with Examples/<Set>/<Topic>/<calculation> (headless).

    tests/apps/examples_upgrade_test.py        own Xvfb, services, local data

The data folder is made as version 8 left it: Examples (water, benzene and a
calculation of the user's own), Examples_Organic (ethane and one of the
user's own) and Examples_Spectroscopy (sulfate only), each marked with
exampleSet 8, and the Organizer preference that says the examples were
imported.  After the start, the three sets are there in the new layout; the
user's calculations are untouched; the shipped ones of the old projects, and
Examples_Spectroscopy, which they left empty, are gone.

Same installed tree and isolation as run_tests.py.  Exit 77 (skip) without
an install or Xvfb.
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
import examples_test as ex   # noqa: E402

SKIP = 77
EXAMPLES = os.path.join(ROOT, "data", "client", "examples")
META = ".\thttp://www.emsl.pnl.gov/ecce:%s\t\t%s\n"


def oldProject(root, name, calcs):
    """calcs: {name: source calculation folder}"""
    folder = os.path.join(root, name)
    os.makedirs(folder)
    with open(os.path.join(folder, ".ecce-meta"), "w") as f:
        f.write(META % ("contenttype", "ecceProject"))
        f.write(META % ("exampleSet", "8"))
    for calc, source in calcs.items():
        shutil.copytree(source, os.path.join(folder, calc), symlinks=True)


def stored(setDir, example, calc):
    return os.path.join(EXAMPLES, setDir, example, "stored", calc)


def run(display, data, work, problems):
    root = os.path.join(data, "users", "local")
    mine = os.path.join(HERE, "fixtures", "calc-water-opt")
    oldProject(root, "Examples", {
        "water": stored("getting-started", "ecceqm-water", "water"),
        "benzene": stored("getting-started", "orca-benzene", "benzene"),
        "mine": mine})
    oldProject(root, "Examples_Organic", {
        "ethane": stored("organic", "bonding-in-hydrocarbons/ecceqm-ethane",
                         "ethane"),
        "myown": mine})
    oldProject(root, "Examples_Spectroscopy", {
        "sulfate": stored("spectroscopy", "sulfate-coordination/orca-sulfate",
                          "sulfate")})
    prefs = os.path.join(os.environ["ECCE_REALUSERHOME"], ".ECCE")
    os.makedirs(prefs, exist_ok=True)
    with open(os.path.join(prefs, "Organizer"), "w") as f:
        f.write("Organizer.ExampleSet:\t8\n")
    seen = {}

    def wait(_display):
        for project, expect in (("Examples/Getting_Started", ex.EXPECT),
                                ("Examples/Organic_Chemistry", ex.ORGANIC),
                                ("Examples/Spectroscopy", ex.SPECTROSCOPY)):
            seen[project] = ex.waitImported(root, [project], expect)[project]

    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_NO_EXAMPLE_IMPORT": ""}, inspect=wait)
    if result.crashed:
        problems.append("the Organizer crashed (%s)" % result.signalName)
    for project, expect in (("Examples/Getting_Started", ex.EXPECT),
                            ("Examples/Organic_Chemistry", ex.ORGANIC),
                            ("Examples/Spectroscopy", ex.SPECTROSCOPY)):
        if sorted(seen.get(project) or {}) != sorted(expect):
            problems.append("%s holds %s" % (project, sorted(seen.get(project) or {})))
    top = os.path.join(root, "Examples")
    for gone in ("water", "benzene"):
        if os.path.exists(os.path.join(top, gone)):
            problems.append("old Examples/%s was not removed" % gone)
    if not os.path.isdir(os.path.join(top, "mine")):
        problems.append("the user's calculation Examples/mine is gone")
    organic = os.path.join(root, "Examples_Organic")
    if sorted(d for d in os.listdir(organic) if not d.startswith(".")) != ["myown"]:
        problems.append("Examples_Organic holds %s" % sorted(os.listdir(organic)))
    if os.path.exists(os.path.join(root, "Examples_Spectroscopy")):
        problems.append("the emptied Examples_Spectroscopy was kept")
    text = open(os.path.join(prefs, "Organizer")).read()
    if not re.search(r"EXAMPLESET:\s*9", text, re.I):
        problems.append("the preference still says %r" % text.strip())
    return result


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
    work = tempfile.mkdtemp(prefix="examples-upgrade-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    problems = []
    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            result = run(display, data, work, problems)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin",
                                         "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    if problems:
        print("FAIL  upgrade: " + "; ".join(problems))
        print((result.log or "")[-1500:])
    else:
        print("ok    upgrade: old projects replaced, the user's calculations kept")
    if os.environ.get("ECCE_TEST_KEEP"):
        print("kept " + work)
    else:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED" if problems else "PASSED")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
