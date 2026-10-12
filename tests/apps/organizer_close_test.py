#!/usr/bin/env python3
"""
The Organizer closed twice in a row (#291), headless.

Two close requests reach the frame before the first Quit dialog is
answered, as a double click on the title bar's X does.  The second used to
open a second dialog inside the first one's event loop; answering the first
then deleted the frame in that loop, and the frame's destructor freed the
first dialog, an object on the stack ("free(): invalid pointer").  The
ECCE_TEST_ORGANIZER hook queues the closes ("close 2") and answers the
dialog ("answer ok"); the Organizer must leave with status 0 and no abort.

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

failures = []


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-close-data-")
    work = tempfile.mkdtemp(prefix="ecce-close-")
    try:
        project = os.path.join(data, "users", "local", "close-test")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        now = time.time()
        done = pb.makeCalc(project, "water-done", "Complete",
                           pb.DONE.format(sent=now - 20, at=int(now - 20)))
        url = "file://" + done
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as handle:
            handle.write("summary %s\nprogressbox\n" % url)

        def append(line):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")

        def inspect(_display):
            time.sleep(3)
            append("close 2")
            time.sleep(3)
            append("answer ok")
            time.sleep(6)

        result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": url,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        if result.crashed or "free()" in log or "Aborted" in log:
            failures.append("Organizer aborted on a double close (%s)"
                            % (result.signalName or "see log"))
            print("FAIL  two closes in a row\n" + log[-1500:])
        elif result.returncode != 0:
            failures.append("Organizer did not exit cleanly (status %s)"
                            % result.returncode)
            print("FAIL  exit status %s\n%s" % (result.returncode, log[-1500:]))
        else:
            print("ok    two closes in a row: one dialog, a clean exit")
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("close", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
