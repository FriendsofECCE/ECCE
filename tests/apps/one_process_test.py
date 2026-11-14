#!/usr/bin/env python3
"""
Builder and Calculation Editor as windows of the Organizer's process
(#983, ECCE_ONE_PROCESS=1), headless.

The Organizer's test hook starts tools as its buttons do; with the switch
on they must open in the Organizer's process (same pid in the message
trace), the Builder's save notice must reach the Organizer, a second
Builder (another calculation) must open beside the first, the editor must
get its invoke, and closing the tool windows must leave the Organizer
running and answering.

Same installed tree, isolation and services as run_tests.py.  Exit 77
(skip) without an install or Xvfb.
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import run_tests  # noqa: E402
import progress_box_test as pb  # noqa: E402

failures = []
# --gdb: run the Organizer under gdb, for a backtrace on a crash.
GDB = "--gdb" in sys.argv
if GDB:
    sys.argv.remove("--gdb")


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url1 = "file://" + pb.makeCalc(project, "water-a", "created", None)
        url2 = "file://" + pb.makeCalc(project, "water-b", "created", None)
        cmds = os.path.join(work, "commands")
        script = os.path.join(work, "builder-script")
        open(cmds, "w").close()
        with open(script, "w") as handle:
            handle.write("save\n")

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("start Builder " + url1, 25)
            append("start Builder " + url2, 15)
            append("start CalculationEditor " + url1, 15)
            append("inprocess", 2)
            append("closetools", 5)
            append("answer no", 3)    # questions the tools ask, if any
            append("answer no", 3)
            append("inprocess", 3)

        extra = {"ECCE_TEST_GDB": "1"} if GDB else {}
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**extra, "ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_TEST_MESSAGES": "1",
                               "ECCE_BUILDER_SCRIPT": script,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        check(not result.crashed and "Aborted" not in log,
              "the Organizer ran to the end without a crash",
              result.signalName or log[-1500:])

        inproc = pb.answers(log, "inprocess")
        pid = None
        if check(len(inproc) == 2, "both window counts answered", log[-2000:]):
            m = re.match(r"pid=(\d+) Builder=(\d+) CalculationEditor=(\d+)",
                         inproc[0])
            pid = m.group(1) if m else None
            check(m and m.group(2) == "2" and m.group(3) == "1",
                  "two Builders and one editor open in the Organizer",
                  inproc[0])
            check("Builder=0 CalculationEditor=0" in inproc[1],
                  "closing the tool windows leaves the Organizer running",
                  inproc[1])
        check(pb.answers(log, "closetools") == ["queued", "closed"],
              "the tool windows closed", str(pb.answers(log, "closetools")))

        trace = re.findall(r"ECCE_TEST_MESSAGES: (\S+) (\S+) pid=(\d+) "
                           r"(\S+) from (\S*) (\S*)", log)
        builders = set(t[1] for t in trace
                       if t[0] == "Builder" and t[3] == "ecce_invoke")
        check(len(builders) == 2 and all(t[2] == pid for t in trace),
              "each Builder got its invoke, in the Organizer's process",
              str(sorted(builders)) + " pid " + str(pid))
        check(any(t[0] == "CalculationEditor" and t[3] == "ecce_invoke"
                  and t[2] == pid for t in trace),
              "the editor got its invoke in the Organizer's process")
        check(any(t[0] == "Organizer" and t[3].startswith("ecce_url_")
                  and t[4] == "Builder" for t in trace),
              "the Builder's save notice reached the Organizer",
              "\n".join(" ".join(t) for t in trace if t[0] == "Organizer"))
        for t in trace:
            print("      trace: " + " ".join(t))
        if GDB:
            print(log[-20000:])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def runQuit(display):
    """Quit asks the in-process Builder about its unsaved edit: Cancel there
    cancels the Quit, No lets the process end."""
    data = tempfile.mkdtemp(prefix="ecce-oneproc-data-")
    work = tempfile.mkdtemp(prefix="ecce-oneproc-")
    try:
        project = os.path.join(data, "users", "local", "oneproc")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url = "file://" + pb.makeCalc(project, "water-q", "created", None)
        cmds = os.path.join(work, "commands")
        script = os.path.join(work, "builder-script")
        open(cmds, "w").close()
        with open(script, "w") as handle:
            handle.write("wait 3000\nadd C Tetrahedral 8 0 0\nsavestate\n")

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("start Builder " + url, 25)
            append("inprocess", 2)
            append("close", 3)          # File > Quit
            append("answer", 3)         # "Quit"
            append("answer cancel", 3)  # the Builder's save question
            append("inprocess", 2)
            append("close", 3)
            append("answer", 3)
            # The process ends after the broker's 10 s linger (OnExit).
            append("answer no", 25)
            if GDB:
                # Where a process that did not end is: attach, all threads.
                pids = [p for p in os.listdir("/proc") if p.isdigit() and
                        open("/proc/%s/comm" % p).read().strip() == "organizer"]
                for p in pids:
                    print(subprocess.run(
                        ["gdb", "-batch", "-p", p, "-ex",
                         "thread apply all bt 40"],
                        capture_output=True, text=True).stdout[-15000:])

        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={**({"ECCE_TEST_GDB": "1"} if GDB else {}),
                               "ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_BUILDER_SCRIPT": script,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        inproc = pb.answers(log, "inprocess")
        check(len(inproc) == 2 and
              "Builder=1 CalculationEditor=0" in inproc[0],
              "the Builder opened in the Organizer", str(inproc) + log[-1500:])
        check("BUILDER: dirty=1" in log, "the Builder has unsaved changes")
        check(len(inproc) == 2 and
              "Builder=1 CalculationEditor=0" in inproc[1],
              "Cancel in the Builder's question cancels the Quit", str(inproc))
        check(pb.answers(log, "answer cancel") ==
              ["answered Save Builder Changes?"] and
              pb.answers(log, "answer no") ==
              ["answered Save Builder Changes?"],
              "the Builder asked about its changes at each Quit",
              str(pb.answers(log, "answer")))
        check(result.returncode == 0 and "Aborted" not in log,
              "No in the Builder's question ends the process cleanly",
              "returncode %s\n%s" % (result.returncode, log[-1500:]))
        if GDB:
            print(log[-20000:])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        runQuit(display)
        if failures:
            results.fail("one-process", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
