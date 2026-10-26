#!/usr/bin/env python3
"""
The Builder opens on a calculation when wxbuilder.ini was last written by an
older ECCE version (#605).

A modal notice shown from Builder::restoreSettings(), before the frame
existed, opened out of sight on macOS and the Viewer never appeared.  Here
wxbuilder.ini records Version=9.0.0, the Builder opens an ECCE-QM calculation
with ECCE_PANEL_METRICS, and the metrics file must be written: that happens
only once the calculation is open, which a modal in the constructor
prevents.  The stored version must then be the running one.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                  # noqa: E402
import ecceqm_builder_test   # noqa: E402
import fixture               # noqa: E402
import run_tests             # noqa: E402
import xdisplay              # noqa: E402

OLD_VERSION = "9.0.0"


def setStoredVersion(version):
    """Write Version=<version> into wxbuilder.ini; returns a restore call."""
    path = os.path.join(fixture.stateHome(), ".ECCE", "wxbuilder.ini")
    original = None
    lines = []
    if os.path.exists(path):
        with open(path) as handle:
            original = handle.read()
        lines = [l for l in original.splitlines()
                 if not l.startswith(("Version=", "ResetPerspective="))]
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as handle:
        handle.write("\n".join(["Version=%s" % version] + lines) + "\n")

    def restore():
        if original is None:
            if os.path.exists(path):
                os.remove(path)
        else:
            with open(path, "w") as handle:
                handle.write(original)
    return path, restore


def main():
    xdisplay.SCREEN = ecceqm_builder_test.SCREEN

    def checkApp(display, name, results, verbose=False):
        if not os.path.isdir(ecceqm_builder_test.QM_CALC):
            results.notes.append("%s is missing; skipped"
                                 % ecceqm_builder_test.QM_CALC)
            return
        folder = os.path.join(fixture.stateDir(), "upgrade-localdata")
        urls = ecceqm_builder_test.installLocal(
            folder, {"o2": ecceqm_builder_test.QM_CALC})
        os.environ["ECCE_LOCAL_DATA"] = folder
        path, restore = setStoredVersion(OLD_VERSION)
        metrics = tempfile.mktemp(prefix="ecce-upgrade-", suffix=".txt")
        try:
            result = apps.run(display, "builder", args=("-context", urls["o2"]),
                              windowTimeout=60, settle=30,
                              env={"ECCE_PANEL_METRICS": metrics})
            with open(path) as handle:
                stored = [l.split("=", 1)[1] for l in handle.read().splitlines()
                          if l.startswith("Version=")]
            problem = None
            if result.crashed:
                problem = "CRASHED (%s)" % result.signalName
            elif not result.sawWindow:
                problem = "no window"
            elif not os.path.exists(metrics):
                problem = ("the calculation never opened (no panel metrics): "
                           "windows %s" % result.windows)
            elif stored == [OLD_VERSION]:
                problem = "wxbuilder.ini still records %s" % OLD_VERSION
            if problem:
                results.fail("upgrade", "%s\n%s"
                             % (problem, run_tests._tail(result.log or "", 15)))
            else:
                print("ok    Builder opens after an upgrade (stored %s)"
                      % ", ".join(stored))
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)
            if os.path.exists(metrics):
                os.unlink(metrics)
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
