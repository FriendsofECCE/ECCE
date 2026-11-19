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

The script answers the warning with yes/cancel, then submits benzene with a
stand-in engine to check that it leaves Ready without running the chemistry.

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


def launcher(s, env, wrappers, url, answer, png=None, name="x", launch=False):
    """Runs the Launcher on the calculation; returns its log text."""
    work = os.path.join(s.state, name)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    script = os.path.join(work, "script")
    with open(script, "w") as handle:
        commands = "wait 2500\n"
        if launch:
            commands += "machine localhost\nprocs 1\nrundir %s\nlaunch\n" % os.path.join(work, "run")
        else:
            commands += "sizecheck\n"
        if answer:
            commands += "answer %s\n" % answer
        commands += "print launcherror\nprint state\nquit\n"
        handle.write(commands)
    log = os.path.join(work, "launcher.log")
    lenv = dict(env, ECCE_LAUNCHER_SCRIPT=script, ECCE_TEST_DIALOG_ANSWER="cancel",
                ECCE_TEST_DIALOG_CLOSE="60")
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
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
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
    # Hosted (ECCE_TEST_HOSTED=1), the Launcher runs as a tab of the Organizer.
    hosted = ("organizer",) if os.environ.get("ECCE_TEST_HOSTED") == "1" else ()
    for exe in ("launcher",) + hosted:
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
        help_result = subprocess.run([os.path.join(build, "ecce-launch"), "--help"],
                                     env=env, capture_output=True, text=True, timeout=30)
        check(help_result.returncode == 0 and "--accept-large-qm" in help_result.stdout,
              "ecce-launch --help documents explicit large-QM acceptance")
        option_result = subprocess.run(
            [os.path.join(build, "ecce-launch"), "--accept-large-qm", "file:///unused"],
            env=dict(env, ECCE_SESSION_ID="0123456789abcdef"),
            capture_output=True, text=True, timeout=30)
        check(option_result.returncode == 2 and "not running" in option_result.stderr,
              "ecce-launch accepts the option before checking for a session")
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
        check("sizecheck: cancel" in text and "FAIL" not in text,
              "Cancel does not launch")
        text = launcher(s, env, wrappers, urls["h2o-geovib"], None, None, "water")
        check("ECCE_TEST_DIALOG:" not in text and "sizecheck: go" in text,
              "water GeoVib is not warned about")
        check("FAIL" not in text, "no script failure")
        # Submission is the subject of this check; avoid a costly frequency run.
        deck = os.path.join(s.state, "submission.qmin")
        with open(deck, "w") as handle:
            handle.write("# Input consumed by the submission stub, not a chemistry engine.\n")
        # The stored warning fixture omits empty VDoc collections.
        for collection in ("Inputs", "Outputs", "Props"):
            os.makedirs(os.path.join(urls["c6h6-geovib"][len("file://"):], collection),
                        exist_ok=True)
        rc, out = s.driver("mdfile", urls["c6h6-geovib"], deck, "ecceqm.qmin")
        check(rc == 0, "the submission fixture has a stored input: " + out)
        stub = os.path.join(s.state, "qm-stub")
        with open(stub, "w") as handle:
            handle.write("#!/bin/sh\necho end_of_output\n")
        os.chmod(stub, 0o755)
        text = launcher(s, dict(env, ECCE_ECCEQM=stub), wrappers,
                        urls["c6h6-geovib"], "yes", name="benzene-launch", launch=True)
        say(text[-2500:])
        check("answered Large ECCE-QM calculation: yes" in text,
              "the Launcher script answers the large calculation question")
        check("state = Ready" not in text and "state = " in text
              and "FAIL" not in text, "the large calculation leaves Ready after scripted Launch")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
