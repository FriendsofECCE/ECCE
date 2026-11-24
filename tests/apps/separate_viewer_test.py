#!/usr/bin/env python3
"""
Separate windows (ECCE_ONE_PROCESS=0), headless (#1106): with a Builder
program showing one calculation, a Viewer start for another calculation
must start a Viewer program, and Quit must end the Organizer (the tools,
separate programs, stay open, as in 9.5.2).  Processes are found by the test's command file in their
environment, so the check also runs on releases without the toolstate
hook.

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


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def ours(marker):
    """(pid, comm, viewer) of the processes of this run, gateway's included."""
    found = []
    for pid in os.listdir("/proc"):
        if not pid.isdigit():
            continue
        try:
            env = open("/proc/%s/environ" % pid, "rb").read().split(b"\0")
            comm = open("/proc/%s/comm" % pid).read().strip()
        except OSError:
            continue
        if marker.encode() not in b"\0".join(env):
            continue
        found.append((int(pid), comm, b"ECCE_INVOKE_VIEWER=1" in env))
    return found


def tools(procs):
    return [p for p in procs if p[1] in ("builder", "organizer", "gateway")]


def run(display, batch=False):
    """batch: both start lines in one write, so the Organizer has the second
    request queued while the first tool starts (#1106)."""
    data = tempfile.mkdtemp(prefix="ecce-sepviewer-data-")
    work = tempfile.mkdtemp(prefix="ecce-sepviewer-")
    seen = {}
    try:
        project = os.path.join(data, "users", "local", "sepviewer")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        urlA = "file://" + pb.makeCalc(project, "benzene", "complete", None)
        urlB = "file://" + pb.makeCalc(project, "water", "complete", None)
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            if batch:
                append("start Builder " + urlA + "\nstart CalculationViewer " +
                       urlB, 70)
                seen["builder"] = tools(ours(cmds))
            else:
                append("start Builder " + urlA, 30)
                seen["builder"] = tools(ours(cmds))
                append("start CalculationViewer " + urlB, 40)
            append("toolstate", 2)
            seen["viewer"] = tools(ours(cmds))
            append("close", 4)
            append("answer ok", 40)
            seen["quit"] = tools(ours(cmds))
            for pid, _comm, _v in ours(cmds):
                try:
                    os.kill(pid, 9)
                except OSError:
                    pass

        env = {"ECCE_TEST_ORGANIZER": cmds, "ECCE_TEST_MESSAGES": "1",
               "ECCE_ONE_PROCESS": "0", "ECCE_LOCAL_DATA": data}
        result = apps.run(display, "gateway", windowTimeout=60, settle=20,
                          env=env, inspect=inspect)
        log = result.log or ""
        for k in ("builder", "viewer", "quit"):
            print("      %s: %s" % (k, seen.get(k)))
        print("      toolstate: %s" % pb.answers(log, "toolstate"))
        if os.environ.get("ECCE_SEPVIEWER_LOG"):
            print(log[-12000:])
        if batch:
            st = pb.answers(log, "toolstate")
            check(bool(st) and st[-1].startswith("state=up") and
                  re.search(r"\blaunches=2\b", st[-1]),
                  "two requests in one batch: both tools came up",
                  str(st) + log[-3000:])
        b = [p for p in seen.get("builder", []) if p[1] == "builder"]
        if not batch:
            check(len(b) == 1 and not b[0][2], "a Builder program started",
                  str(seen.get("builder")))
        v = [p for p in seen.get("viewer", []) if p[1] == "builder" and p[2]]
        check(len(v) == 1, "a Viewer program started beside the Builder",
              str(seen.get("viewer")) + log[-3000:])
        org = [p for p in seen.get("quit", []) if p[1] == "organizer"]
        check(not org, "Quit ended the Organizer",
              str(seen.get("quit")) + log[-3000:])
        # Separate programs outlive the Organizer's Quit (9.5.2 too): the
        # gateway ends the session when the last one closes.  Reported only.
        left = [p for p in seen.get("quit", []) if p[1] == "builder"]
        print("      tools open after Quit: %d" % len(left))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display, False)
        run(display, True)
        if failures:
            results.fail("separate-viewer", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
