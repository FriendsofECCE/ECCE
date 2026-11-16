#!/usr/bin/env python3
"""
Feedback in the Organizer while a tool starts (#1071), headless.

A start request (the test hook's "start", as a tool button does) must show
a busy cursor and "Opening the Builder for <calculation>..." at once, the
small "Starting the Builder..." window once the start takes longer than
1.5 s, and drop a second request for the same tool and calculation while
it starts.  ECCE_TEST_TOOL_DELAY holds back the tool's "up" signal so the
slow start can be looked at.  Everything must clear when the tool is up.
Runs with the tool in the Organizer's process (ECCE_ONE_PROCESS=1) and as
a separate program through the gateway.

Same installed tree, isolation and services as run_tests.py.  Exit 77
(skip) without an install or Xvfb.
"""

import os
import re
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
PNG = os.environ.get("ECCE_TOOLSTART_PNG", "")


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display, oneProcess):
    mode = "one process" if oneProcess else "separate program"
    data = tempfile.mkdtemp(prefix="ecce-toolstart-data-")
    work = tempfile.mkdtemp(prefix="ecce-toolstart-")
    try:
        project = os.path.join(data, "users", "local", "toolstart")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url = "file://" + pb.makeCalc(project, "water-a", "created", None)
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        # In one process the tool's window is built on the Organizer's own
        # thread, so the delay counts from after that.
        delay = 15000 if oneProcess else 6000
        wait = 14 if oneProcess else 20

        def inspect(_display):
            append("toolstate", 1)
            append("start Builder " + url, 0.3)
            append("toolstate", 0.5)
            append("start Builder " + url, 0.3)   # while it starts
            append("toolstate", 1.7)
            append("toolstate", 0.3)
            if PNG:
                append("toolshot " + PNG, 0.5)
            time.sleep(wait)                       # the held-back "up"
            append("toolstate", 1)
            append("closetools", 3)
            append("answer no", 3)

        env = {"ECCE_TEST_ORGANIZER": cmds, "ECCE_TEST_MESSAGES": "1",
               "ECCE_TEST_TOOL_DELAY": str(delay),
               "ECCE_LOCAL_DATA": data}
        if oneProcess:
            env["ECCE_ONE_PROCESS"] = "1"
        # Without ECCE_ONE_PROCESS a tool is a program the gateway starts.
        result = apps.run(display, "organizer" if oneProcess else "gateway", windowTimeout=60, settle=20,
                          env=env, inspect=inspect)
        log = result.log or ""
        check(not result.crashed and "Aborted" not in log,
              mode + ": the Organizer ran to the end without a crash",
              result.signalName or log[-1500:])
        if os.environ.get("ECCE_TOOLSTART_LOG"):
            print(log[-8000:])
        states = pb.answers(log, "toolstate")
        if not check(len(states) >= 5, mode + ": toolstate answered",
                     log[-1500:]):
            return
        before, first, second, slow, up = states[:5]
        check(before.startswith("state=none") and "busy=0" in before and
              before.endswith("launches=0"),
              mode + ": nothing shows before a start", before)
        check("state=starting" in first and "busy=1" in first and
              'status="Opening the Builder for water-a' in first
              and "window=none" in first,
              mode + ": busy cursor and status line at once, no window yet",
              first)
        check(second.endswith("launches=1") and slow.endswith("launches=1"),
              mode + ": the second request was dropped while the first "
              "starts", second)
        check("state=starting" in slow and "window=shown" in slow,
              mode + ": the small window shows after 1.5 s", slow)
        check(up.startswith("state=up") and "busy=0" in up and
              'status=""' in up and "window=none" in up,
              mode + ": everything clears when the tool is up", up)
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        if not os.environ.get("ECCE_TOOLSTART_ONLY"):
            run(display, True)
        run(display, False)
        if failures:
            results.fail("tool-start", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
