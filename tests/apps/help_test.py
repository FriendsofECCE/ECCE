#!/usr/bin/env python3
"""
Help > <app> opens the in-app help viewer at the app's own chapter (#219).

The Organizer's and the Builder's ECCE_TEST_HELP hook runs the real Help
menu handler, saves the help window to a PNG, prints the page it shows and
exits.  Checks the page, that the PNG is not near-empty, and keeps the PNGs in
ECCE_TEST_HELP_PNGS if that is set.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

CASES = [
    ("organizer", "first-calculation.html#1-create-a-project", ()),
    ("builder", "first-calculation.html#3-build-the-molecule", "pipe"),
]

failures = []


def png_is_blank(path):
    """A page of text compresses to well over 2 kB; an empty window does not."""
    return os.path.getsize(path) < 2000


def checkHelp(display, app, page, args, outdir):
    png = os.path.join(outdir, "%s-help.png" % app)
    if args == "pipe":
        authPath = fixture.authFile(
            os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"),
            user=fixture.realUser())
        args = ("-pipe", authPath)
    os.environ["ECCE_TEST_HELP"] = png
    try:
        result = apps.run(display, app, args=args, windowTimeout=60, settle=60)
    finally:
        os.environ.pop("ECCE_TEST_HELP", None)
    log = result.log or ""
    m = re.search(r"ECCE_TEST_HELP: %s: page (\S+), (\w+ ?\w*)" % app, log)
    if result.crashed:
        problem = "CRASHED (%s)" % result.signalName
    elif not m:
        problem = "hook never reported"
    elif m.group(1) != page:
        problem = "opened %s, expected %s" % (m.group(1), page)
    elif not os.path.isfile(png) or png_is_blank(png):
        problem = "no usable screenshot"
    else:
        problem = None
    if problem:
        failures.append(app)
        print("FAIL  Help in %s: %s\n%s" % (app, problem,
                                            run_tests._tail(log, 25)))
    else:
        print("ok    Help in %s -> %s (%s)" % (app, page, png))


def main():
    def checkApp(display, name, results, verbose=False):
        keep = os.environ.get("ECCE_TEST_HELP_PNGS")
        outdir = keep or tempfile.mkdtemp(prefix="ecce-help-")
        os.makedirs(outdir, exist_ok=True)
        try:
            for app, page, args in CASES:
                if app == name:
                    checkHelp(display, app, page, args, outdir)
        finally:
            if not keep:
                shutil.rmtree(outdir, ignore_errors=True)
        if failures:
            results.fail("help", "Help menu wrong in: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0]] + sum((["--app", c[0]] for c in CASES), []) \
        + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
