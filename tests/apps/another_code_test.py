#!/usr/bin/env python3
"""Organizer "New from This > In Another Code..." (#1037), through its hooks
`anothercode TYPE SUFFIX FOLDER SOURCE...` and `anothercodefinish`.

Two stored calculations (NWChem and Gaussian 16, water) give two ORCA
calculations: the first from the first source, opened in the Calculation
Editor (the test stands in for the editor by writing an ORCA setup over it),
the second a copy of that setup with the second source's geometry.  Both end
Ready with a deck, and the report names what was made.

    tests/apps/another_code_test.py     own Xvfb, services, local data

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS). Exit 77 (skip) without an install or Xvfb.
"""
import os
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
FIX = os.path.join(HERE, "fixtures")
PROJECT_META = os.path.join(HERE, "..", "..", "tools", "screenshots", "data",
                            "project.ecce-meta")
failures = []


def check(ok, what, log=""):
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
        print("\n".join("      | " + l[-300:] for l in log.splitlines()[-15:]))


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
    print(isolate.describe(settings), flush=True)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    os.environ.pop("DISPLAY", None)
    work = tempfile.mkdtemp(prefix="anothercode-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    project = os.path.join(data, "users", "local", "p")
    os.makedirs(project)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    for name, fix in (("nw", "calc-water-opt"), ("g16", "calc-polar-h2o-g16")):
        shutil.copytree(os.path.join(FIX, fix), os.path.join(project, name), symlinks=True)
    os.environ["ECCE_LOCAL_DATA"] = data
    os.environ["ECCE_TEST_ORGANIZER_NOLAUNCH"] = "1"
    cmds = os.path.join(work, "another.commands")
    urls = " ".join("file://" + os.path.join(project, n) for n in ("nw", "g16"))
    open(cmds, "w").write("anothercode orca_es _orca - %s\n" % urls)
    first = os.path.join(project, "nw_orca")
    second = os.path.join(project, "g16_orca")

    def wait_for(path, secs=90):
        end = time.time() + secs
        while time.time() < end:
            if os.path.exists(path):
                return True
            time.sleep(1)
        return False

    def drive(_display):
        if not wait_for(first):
            return
        time.sleep(3)
        # The editor's job: the setup of an ORCA calculation, saved.
        shutil.rmtree(first)
        shutil.copytree(os.path.join(FIX, "calc-orca-water-ccpvdz"), first, symlinks=True)
        os.makedirs(os.path.join(first, "Inputs"), exist_ok=True)
        for root, _d, files in os.walk(first):
            for f in files:
                path = os.path.join(root, f)
                try:
                    body = open(path).read()
                except (OSError, UnicodeDecodeError):
                    continue
                if "ecce:state" in body:
                    open(path, "w").write(body.replace("ecce:state\t\tComplete" , "ecce:state\t\tReady")
                                          .replace("ecce:state\tComplete", "ecce:state\tReady"))
        open(cmds, "a").write("anothercodefinish\n")
        wait_for(os.path.join(second, "Inputs", "orca.orcain"), 120)
        time.sleep(5)

    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            result = apps.run(display, "organizer", windowTimeout=60, settle=10,
                              env={"ECCE_TEST_ORGANIZER": cmds}, inspect=drive)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    log = result.log or ""
    check(not result.crashed, "the Organizer did not crash", log)
    check("startApp CalculationEditor force=0 url=file://" + first in log,
          "the first calculation was opened in the Calculation Editor", log)
    deck = os.path.join(second, "Inputs", "orca.orcain")
    text = open(deck).read() if os.path.exists(deck) else ""
    check("* xyz 0 1" in text, "the second calculation has an ORCA deck, charge 0 mult 1", log)
    atoms = [l.split() for l in text.splitlines()
             if len(l.split()) == 4 and l.split()[0] in ("O", "H")]
    check(sorted(a[0] for a in atoms) == ["H", "H", "O"],
          "its deck holds the Gaussian calculation's three atoms", log)
    check("1 of 1 more made" in log, "the report says the other one was made", log)
    for n in ("nw_orca", "g16_orca"):
        found = ""
        for root, _d, files in os.walk(os.path.join(project, n)):
            for f in files:
                try:
                    for line in open(os.path.join(root, f), errors="replace"):
                        if "ecce:state" in line:
                            found = line.split()[-1]
                except OSError:
                    pass
        check(found == "Ready", "%s is Ready (state '%s')" % (n, found), log)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
