#!/usr/bin/env python3
"""File > Open Example Calculations imports the shipped examples (headless).

    tests/apps/examples_test.py                own Xvfb, services, local data

The Organizer's ECCE_TEST_ORGANIZER command `examples` is the menu item.  It
imports the outputs under data/client/examples (ECCE-QM water, ORCA benzene,
MOPAC ammonia, NWChem methane) into a new project Examples, and a second
call into Examples_2 without touching the first.  Each calculation must
carry the properties its output holds: the files under Props/ are what the
Viewer's Properties menu is built from.  The project records the version of
the example set.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77

# calculation directory (named by the importer) -> Props the example must
# carry.  The names are the property files ECCE stores (Props/<NAME>).
EXPECT = {
    "water": ["TE", "VIBFREQ", "VIBIR", "MULLIKEN", "LOEWDIN", "ESPFIT",
              "MKFIT", "MO"],
    "benzene": ["MO", "VIBFREQ", "ESPFIT", "HIRSHFELD", "MBIS", "NPA"],
    "ammonia": ["MO", "VIBFREQ"],
    "methane": ["MO", "VIBFREQ", "MULLIKEN"],
}

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-15:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def propsOf(project):
    """{calculation: set of Props files} under a project directory."""
    found = {}
    for calc in sorted(os.listdir(project)) if os.path.isdir(project) else []:
        props = os.path.join(project, calc, "Props")
        if os.path.isdir(props):
            found[calc] = set(os.listdir(props))
    return found


def waitImported(root, projects, seconds=240):
    """Wait until each project holds four calculations whose Props stopped
    changing for 20 s."""
    end = time.time() + seconds
    last, stable = None, 0
    while time.time() < end:
        now = {p: propsOf(os.path.join(root, p)) for p in projects}
        if all(len(v) == len(EXPECT) for v in now.values()) and now == last:
            stable += 1
            if stable >= 10:
                return now
        else:
            stable = 0
        last = now
        time.sleep(2)
    return last


def run(display, data, work):
    root = os.path.join(data, "users", "local")
    script = os.path.join(work, "commands")
    with open(script, "w") as handle:
        handle.write("examples\nexamples\n")
    box = {}

    def inspect(_display):
        box["props"] = waitImported(root, ["Examples", "Examples_2"])

    result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                      env={"ECCE_TEST_ORGANIZER": script}, inspect=inspect)
    log = result.log or ""
    what = "Open Example Calculations"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    answers = re.findall(r"ECCE_TEST_ORGANIZER: examples: (.*)$", log, re.M)
    problems = []
    if answers != ["ok Examples", "ok Examples_2"]:
        problems.append("answers %s" % answers)
    for project in ("Examples", "Examples_2"):
        got = (box.get("props") or {}).get(project, {})
        for calc, names in EXPECT.items():
            held = [c for c in got if c.startswith(calc)]
            if not held:
                problems.append("%s: no calculation %s (have %s)"
                                % (project, calc, sorted(got)))
                continue
            missing = [n for n in names if n not in got[held[0]]]
            if missing:
                problems.append("%s/%s lacks %s (has %s)"
                                % (project, held[0], missing,
                                   sorted(got[held[0]])))
        meta = os.path.join(root, project, ".ecce-meta")
        text = open(meta).read() if os.path.exists(meta) else ""
        if not re.search(r"exampleSet[^0-9]*1", text):
            problems.append("%s records no example-set version" % project)
    if problems:
        fail(what, "; ".join(problems), log)
    else:
        print("ok    %s: %s" % (what, ", ".join(sorted(EXPECT))))


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
    work = tempfile.mkdtemp(prefix="examples-",
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
    if os.environ.get("ECCE_TEST_KEEP"):
        print("kept " + work)
    else:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
