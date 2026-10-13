#!/usr/bin/env python3
"""The ECCE-QM size warning at Launch (#248), headless, with the tree's binaries.

    tests/apps/ecceqm_size_warning_test.py [--build build-cmake] [--png DIR]

The Launcher's test hook (ECCE_LAUNCHER_SCRIPT) runs "sizecheck", the check
Launch makes before anything is submitted, on a calculation opened with
-context:

  * benzene, GeoVib, B3LYP/6-31G* (12 atoms, 96 functions: N^2 x nbf = 13824)
    warns about the time of a frequency run, Launch anyway goes on, Cancel
    does not;
  * water, GeoVib, B3LYP/6-31G* (3 atoms, 18 functions) is not asked.

The memory paragraph (more than 25 atoms or 150 functions) is not reached by
a stored fixture, so the arithmetic is only checked for benzene's absence of it.
Exit status 77 (skip) without Xvfb, import or binaries.
"""

import argparse
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import energies_groups_test as eg  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
FIXTURES = os.path.join(REPO, "tests", "ecceqm", "fixtures")


def launcher(s, env, wrappers, url, answer, png=None, name="x"):
    """Runs the Launcher on the calculation; returns its log text."""
    work = os.path.join(s.state, name)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    script = os.path.join(work, "script")
    with open(script, "w") as handle:
        handle.write("wait 2500\nsizecheck\nquit\n")
    log = os.path.join(work, "launcher.log")
    lenv = dict(env, ECCE_LAUNCHER_SCRIPT=script, ECCE_TEST_DIALOG_ANSWER=answer,
                ECCE_TEST_DIALOG_CLOSE="7")
    proc = subprocess.Popen([os.path.join(wrappers, "ecce-launcher"), "-context", url],
                            env=lenv, cwd=os.path.join(s.home, "bin"),
                            stdout=open(log, "w"), stderr=subprocess.STDOUT,
                            start_new_session=True)
    deadline = time.time() + 120
    shot = False
    while proc.poll() is None and time.time() < deadline:
        with open(log, errors="replace") as handle:
            text = handle.read()
        if png and not shot and "ECCE_TEST_DIALOG:" in text:
            time.sleep(2)
            subprocess.run(["import", "-window", "root", png], env=env, timeout=60,
                           stderr=subprocess.DEVNULL)
            shot = True
        time.sleep(0.4)
    bo.stop(proc)
    with open(log, errors="replace") as handle:
        return handle.read()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in ("launcher",):
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    for name in ("c6h6-geovib", "h2o-geovib"):
        if not os.path.isdir(os.path.join(FIXTURES, name)):
            harness.skip("fixture %s is missing" % name)
    import xdisplay

    s = harness.Session(build, "qmsize", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in ("launcher",):
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    say("binaries: %s" % os.path.realpath(os.path.join(s.home, "bin", "launcher")))

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        urls = {n: eg.install(s, n, FIXTURES) for n in ("c6h6-geovib", "h2o-geovib")}

        text = launcher(s, env, wrappers, urls["c6h6-geovib"], "yes",
                        os.path.join(png, "qm-size-warning.png"), "benzene-yes")
        say(text[-1500:])
        flat = " ".join(text.split())
        dialogs = ["ECCE_TEST_DIALOG:" + d.split("[LAUNCHER]")[0]
                   for d in flat.split("ECCE_TEST_DIALOG:")[1:]]
        check(len(dialogs) == 1 and "[Large ECCE-QM calculation]" in dialogs[0],
              "benzene GeoVib B3LYP 6-31G* is warned about: %s" % dialogs)
        check(dialogs and "12 atoms with 96 basis functions" in dialogs[0]
              and "roughly 14 to 28 minutes" in dialogs[0],
              "the warning names the size and a time range")
        check(dialogs and "RAM" not in dialogs[0],
              "no memory paragraph below 25 atoms and 150 functions")
        check("sizecheck: go" in text, "Launch anyway goes on")
        text = launcher(s, env, wrappers, urls["c6h6-geovib"], "cancel", None,
                        "benzene-cancel")
        check("sizecheck: cancel" in text, "Cancel does not launch")
        text = launcher(s, env, wrappers, urls["h2o-geovib"], "yes", None, "water")
        check("ECCE_TEST_DIALOG:" not in text and "sizecheck: go" in text,
              "water GeoVib is not warned about")
        check("FAIL" not in text, "no script failure")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
