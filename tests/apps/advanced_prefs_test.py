#!/usr/bin/env python3
"""Preferences > Advanced reaches the programs ECCE starts (#307), headless.

The ECCE_TEST_ORGANIZER hook ticks switches in the real Preferences window
(as a click does) and asks what a program started at that moment sees.  A
switch ticked in the window must show up there, an unticked one must go, and
a variable set outside ECCE (here ECCE_NATIVE_DIALOGS) must win.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import run_tests  # noqa: E402

failures = []

STEPS = [
    ("prefs advanced Advanced.PerPixelOff on", "ok"),
    ("prefs childenv ECCE_GL_PERPIXEL", "ECCE_GL_PERPIXEL=0"),
    ("prefs advanced Advanced.DebugGL on", "ok"),
    ("prefs childenv ECCE_DEBUG_GL_VISUAL", "ECCE_DEBUG_GL_VISUAL=1"),
    ("prefs advanced Advanced.PerPixelOff off", "ok"),
    ("prefs childenv ECCE_GL_PERPIXEL", "ECCE_GL_PERPIXEL=UNSET"),
    ("prefs advanced Advanced.NativeDialogs on", "ok"),
    ("prefs childenv ECCE_NATIVE_DIALOGS", "ECCE_NATIVE_DIALOGS=explicit"),
    ("prefs advanced Advanced.MoSymLog on", "ok"),
    ("prefs childenv ECCE_DEBUG_MOSYM_LOG", None),   # path under ~/.ECCE
]


def run(display):
    work = tempfile.mkdtemp(prefix="ecce-advprefs-")
    data = tempfile.mkdtemp(prefix="ecce-advprefs-data-")
    try:
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as handle:
            handle.write("".join(c + "\n" for c, _ in STEPS))
        result = apps.run(display, "organizer", windowTimeout=60, settle=25,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data,
                               "ECCE_NATIVE_DIALOGS": "explicit"})
        log = result.log or ""
        # The hook answers in order; a command may appear twice.
        answers = [l.split("ECCE_TEST_ORGANIZER: ", 1)[1]
                   for l in log.splitlines() if "ECCE_TEST_ORGANIZER: " in l]
        for number, (command, want) in enumerate(STEPS):
            got = None
            if number < len(answers) and answers[number].startswith(command + ": "):
                got = answers[number][len(command) + 2:].strip()
            if want is None:
                ok = got is not None and got.endswith("/.ECCE/mosym.log")
            else:
                ok = got == want
            if ok:
                print("ok    %s -> %s" % (command, got))
            else:
                failures.append(command)
                print("FAIL  %s: wanted %r, got %r" % (command, want, got))
        if failures:
            print(log[-1500:])
    finally:
        shutil.rmtree(work, ignore_errors=True)
        shutil.rmtree(data, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("advanced", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
