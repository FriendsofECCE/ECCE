#!/usr/bin/env python3
"""Compare between the General Chemistry gas runs, in the tree as imported.

File > Open Example Calculations puts each GROMACS run in a study inside its
topic (Examples/General_Chemistry/Gases,_liquids_and_solids/ar-gas/dynamics),
so Compare must look inside studies: from he-gas it offers ar-gas and the
other way round.  With ECCE_MDCOMPARE_PNG=<dir> the argon, water and NaCl
runs are photographed (atoms drawn, molecules whole across the cell).

    ECCE_TEST_HOME=<install> ECCE_TEST_WRAPPERS=<wrappers> \\
        tests/apps/md_compare_examples_test.py

Exit status 77 (skip) without an installed organizer or Xvfb.
"""

import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps  # noqa: E402
import isolate  # noqa: E402
import xdisplay  # noqa: E402

SKIP = 77
TOPIC = os.path.join("Examples", "General_Chemistry", "Gases,_liquids_and_solids")
bad = 0


def check(ok, what, detail=""):
    global bad
    print("%s  %s%s" % ("ok  " if ok else "FAIL", what,
                        ("  (" + detail + ")") if detail and not ok else ""))
    if not ok:
        bad += 1


def task(root, study):
    found = glob.glob(os.path.join(root, TOPIC, study, "*", ".ecce-meta"))
    return os.path.dirname(found[0]) if found else None


def builder(display, url, lines, png=None, windowTimeout=90, settle=30):
    script = tempfile.mktemp(prefix="bscript-")
    with open(script, "w") as f:
        f.write("\n".join(lines) + "\nquit\n")
    env = {"ECCE_BUILDER_SCRIPT": script, "ECCE_PANEL_MODE": "detail",
           "ECCE_OPEN_PANEL": "Speed Distribution"}
    result = apps.run(display, "builder", args=["-context", url],
                      windowTimeout=windowTimeout, settle=settle, env=env,
                      inspect=(lambda d: subprocess.run(
                          ["import", "-window", "root", png],
                          env=d.env(), timeout=60)) if png else None)
    return result.log or ""


def run(display, data, work):
    root = os.path.join(data, "users", "local")
    script = os.path.join(work, "commands")
    with open(script, "w") as f:
        f.write("examples genchem\n")
    holder = {}

    def imported(_display):
        end = time.time() + 900
        while time.time() < end:
            if task(root, "ar-gas") and task(root, "he-gas"):
                time.sleep(20)
                break
            time.sleep(5)
        holder["done"] = True

    result = apps.run(display, "organizer", windowTimeout=60, settle=10,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_NO_EXAMPLE_IMPORT": ""}, inspect=imported)
    ar, he = task(root, "ar-gas"), task(root, "he-gas")
    check(ar and he, "the gas runs were imported as studies under the topic",
          "%s / %s" % (ar, he))
    if not (ar and he):
        print((result.log or "")[-2000:])
        return
    for own, other in ((he, "ar-gas"), (ar, "he-gas")):
        log = builder(display, "file://" + own + "/", ["wait 6000",
                                                        "mdcompare speed ?"])
        m = re.search(r"BUILDER: mdcompare candidates=(\d+) ms=(\d+) (.*)$", log, re.M)
        offered = m.group(3).split("|") if m else []
        check(m and int(m.group(2)) < 2000,
              "Compare's search takes under 2 s (only the run's topic)",
              m.group(0) if m else "")
        check(any(("/%s/" % other) in o or o.startswith(other + "/")
                  for o in offered),
              "Compare from %s offers %s" % (os.path.basename(os.path.dirname(own)), other),
              m.group(0) if m else log[-1500:])
    shots = os.environ.get("ECCE_MDCOMPARE_PNG")
    if shots:
        os.makedirs(shots, exist_ok=True)
        for study in ("ar-gas", "water-spce", "nacl-dissolving"):
            own = task(root, study)
            if own:
                png = os.path.join(shots, study + ".png")
                t0 = time.time()
                # The picture is taken while the script still waits.
                builder(display, "file://" + own + "/", ["wait 40000"], png,
                        windowTimeout=900, settle=25)
                print("picture of %s: %s (%.0f s to open and settle)"
                      % (study, png, time.time() - t0))


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
    work = tempfile.mkdtemp(prefix="mdcompare-",
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
    if not os.environ.get("ECCE_TEST_KEEP"):
        shutil.rmtree(work, ignore_errors=True)
    print("PASSED" if bad == 0 else "FAILED %d" % bad)
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
