#!/usr/bin/env python3
"""
The main window's close asks before quitting whatever tab is in front
(#1291), headless.

The Quit question belongs to the window, not to the Organizer's tab: a
close with a clean Builder or Calculation Editor tab selected must ask
exactly as a close with the Organizer tab selected does.
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

import apps          # noqa: E402
import run_tests     # noqa: E402
import progress_box_test as pb          # noqa: E402
import one_process_test as one          # noqa: E402

failures = []


def check(ok, what, detail=""):
    print(("ok   " if ok else "FAIL ") + what)
    if detail:
        print("     " + str(detail).replace("\n", "\n     ")[:1500])
    if not ok:
        failures.append(what)
    sys.stdout.flush()


def closeWithTab(display, tool, url, label):
    """Start `tool` (nothing edited), then close the window: the Quit
    question must come up, and Cancel must leave the session running."""
    one.setLayout(display, "small")
    data = tempfile.mkdtemp(prefix="ecce-close-data-")
    work = tempfile.mkdtemp(prefix="ecce-close-")
    try:
        project = os.path.join(data, "users", "local", "closeask")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = "file://" + pb.makeCalc(project, "water-h", "created", None)
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            if tool:
                append("start %s %s" % (tool, calc if url else ""), 25)
            append("tabsel", 2)
            append("hostclose", 4)
            append("answer cancel", 4)   # Quit ECCE: Cancel, still running
            append("inprocess", 3)
            append("hostclose", 4)
            append("answer", 4)          # Quit
            time.sleep(20)

        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        for line in re.findall(r"^ECCE close:.*$", log, re.M):
            print("     " + line)
        check(pb.answers(log, "answer cancel") == ["answered Quit ECCE"],
              "%s: closing the window asks Quit ECCE" % label,
              str(pb.answers(log, "answer cancel")) +
              "\n" + "\n".join(re.findall(r"^ECCE close:.*$", log, re.M)))
        check(len(pb.answers(log, "inprocess")) == 1,
              "%s: Cancel leaves the session running" % label,
              str(pb.answers(log, "inprocess")))
        check(result.returncode == 0 and "Aborted" not in log,
              "%s: the second close ends the process cleanly" % label,
              "returncode %s\n%s" % (result.returncode, log[-800:]))
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, results):
        closeWithTab(display, None, False, "Organizer tab")
        closeWithTab(display, "Builder", True, "clean Builder tab")
        closeWithTab(display, "CalcEd", True, "Calculation Editor tab")
        if failures:
            results.fail("close-asks", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer", "--budget", "1700"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
