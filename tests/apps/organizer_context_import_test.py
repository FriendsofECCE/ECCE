#!/usr/bin/env python3
"""Right-click > New offers Import Calculation from Output File (headless).

Users looked for the import under the tree's New submenu, where every
other way to make a calculation is, and found it only under File.  The
ECCE_TEST_ORGANIZER command `contextmenu <url>` lists the New submenu of
that node's right-click menu; for a project it must hold the import.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import run_tests  # noqa: E402
import progress_box_test as pb  # noqa: E402

LABEL = "Import Calculation from Output File..."
failures = []


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-ctximport-data-")
    work = tempfile.mkdtemp(prefix="ecce-ctximport-")
    try:
        project = os.path.join(data, "users", "local", "ctx-test")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url = "file://" + project
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as handle:
            handle.write("contextmenu %s\n" % url)
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data})
        lines = [l for l in (result.log or "").splitlines()
                 if "ECCE_TEST_ORGANIZER: contextmenu" in l]
        if not lines:
            failures.append("no contextmenu outcome")
            print("FAIL  no contextmenu outcome\n" + (result.log or "")[-1500:])
        elif LABEL not in lines[-1]:
            failures.append("New submenu lacks the import")
            print("FAIL  New submenu: " + lines[-1])
        else:
            print("ok    right-click New offers the import: " + lines[-1])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("context import", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
