#!/usr/bin/env python3
"""Opening an example set whose project exists adds what is missing (#571).

    tests/apps/examples_topup_test.py        own Xvfb, services, local data

Examples/Organic_Chemistry is made as an earlier version left it: the topic
Bonding in hydrocarbons with ethane only, a calculation of the user's own
beside it, and the marker of an older exampleSet.  1. "examples organic"
(File > Open Example Calculations) adds the other topics and calculations
and leaves ethane and the user's calculation alone.  2. The first start
after an upgrade (older Organizer.ExampleSet preference) restores a
calculation removed afterwards, again without touching the rest.

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
PROJECT = "Examples/Organic_Chemistry"


def exampleSetVersion():
    src = open(os.path.join(ROOT, "src", "apps", "organizer", "CalcMgr.C")).read()
    return re.search(r'kExampleSetVersion = "(\d+)"', src).group(1)


def folder(path, mark=None):
    os.makedirs(path)
    with open(os.path.join(path, ".ecce-meta"), "w") as f:
        f.write(META % ("contenttype", "ecceProject"))
        if mark:
            f.write(META % ("exampleSet", mark))


def snapshot(calcDir):
    """{relative file: (mtime_ns, size)} of one calculation folder."""
    out = {}
    for base, _dirs, files in os.walk(calcDir):
        for name in files:
            path = os.path.join(base, name)
            st = os.stat(path)
            out[os.path.relpath(path, calcDir)] = (st.st_mtime_ns, st.st_size)
    return out


def mark(root):
    text = open(os.path.join(root, PROJECT, ".ecce-meta"),
                errors="replace").read()
    found = re.findall(r"exampleSet\t\t(\S+)", text)
    return found[-1] if found else None


def run(display, data, work, problems):
    root = os.path.join(data, "users", "local")
    project = os.path.join(root, PROJECT)
    top = os.path.join(root, "Examples")
    folder(top)
    folder(project, "11")
    topic = os.path.join(project, "Bonding_in_hydrocarbons")
    folder(topic)
    shutil.copytree(os.path.join(EXAMPLES, "organic", "bonding-in-hydrocarbons",
                                 "ecceqm-ethane", "stored", "ethane"),
                    os.path.join(topic, "ethane"), symlinks=True)
    shutil.copytree(os.path.join(HERE, "fixtures", "calc-water-opt"),
                    os.path.join(project, "mine"), symlinks=True)
    before = {c: snapshot(p) for c, p in (
        ("ethane", os.path.join(topic, "ethane")),
        ("mine", os.path.join(project, "mine")))}

    def untouched(stage):
        for calc, path in (("ethane", os.path.join(topic, "ethane")),
                           ("mine", os.path.join(project, "mine"))):
            if snapshot(path) != before[calc]:
                problems.append("%s: %s was changed" % (stage, calc))

    def complete(stage, extra=None):
        got = ex.waitImported(root, [PROJECT], ex.ORGANIC)[PROJECT]
        if sorted(got) != sorted(list(ex.ORGANIC) + ["mine"]):
            problems.append("%s: project holds %s" % (stage, sorted(got)))
        if mark(root) != exampleSetVersion():
            problems.append("%s: mark is %r" % (stage, mark(root)))
        untouched(stage)

    script = os.path.join(work, "commands")
    with open(script, "w") as f:
        f.write("examples organic\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_TEST_ORGANIZER": script},
                      inspect=lambda _d: complete("reopen"))
    if result.crashed:
        problems.append("reopen: the Organizer crashed (%s)" % result.signalName)
    log = result.log or ""
    nAdded = len(ex.ORGANIC) - 1
    if not re.search(r"example progress: 1 of %d - " % nAdded, log):
        problems.append("reopen: progress did not count %d imports" % nAdded)
    if not re.search(r"examples [^:]*: ok %s existing" % PROJECT, log):
        problems.append("reopen: hook did not report the existing project")

    # upgrade: one calculation gone, an older preference and mark
    gone = ex.calcPath(project, "bromide")
    shutil.rmtree(gone)
    with open(os.path.join(project, ".ecce-meta"), "a") as f:
        f.write(META % ("exampleSet", "11"))
    before["ethane"] = snapshot(os.path.join(topic, "ethane"))
    before["mine"] = snapshot(os.path.join(project, "mine"))
    prefs = os.path.join(os.environ["ECCE_REALUSERHOME"], ".ECCE")
    os.makedirs(prefs, exist_ok=True)
    with open(os.path.join(prefs, "Organizer"), "w") as f:
        f.write("Organizer.ExampleSet:\t11\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_NO_EXAMPLE_IMPORT": ""},
                      inspect=lambda _d: complete("upgrade"))
    if result.crashed:
        problems.append("upgrade: the Organizer crashed (%s)" % result.signalName)
    if not re.search(r"example progress: 1 of 1 - ", result.log or ""):
        problems.append("upgrade: progress did not count 1 import")
    if os.path.exists(os.path.join(root, "Examples", "Getting_Started")):
        problems.append("upgrade: an unopened set was imported")
    text = open(os.path.join(prefs, "Organizer")).read()
    if not re.search(r"EXAMPLESET:\s*%s\b" % exampleSetVersion(), text, re.I):
        problems.append("upgrade: the preference says %r" % text.strip())
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
    work = tempfile.mkdtemp(prefix="examples-topup-",
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
        print("FAIL  top-up: " + "; ".join(problems))
        print((result.log or "")[-1500:])
    else:
        print("ok    top-up: missing topics and calculations added, existing ones untouched")
    if os.environ.get("ECCE_TEST_KEEP"):
        print("kept " + work)
    else:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED" if problems else "PASSED")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
