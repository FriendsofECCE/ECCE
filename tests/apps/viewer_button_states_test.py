#!/usr/bin/env python3
"""
The Organizer's calculation summary (#1088, item 9): the Viewer button keeps
its place under the Launcher row in every run state, including after a
Terminate (a submitted job that becomes killed), and never lies inside the
Progress box.  Headless, through the ECCE_TEST_ORGANIZER hooks "summary",
"setstate" and "progressbox".  Exit 77 (skip) without an install or Xvfb.
"""

import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps               # noqa: E402
import progress_box_test as box   # noqa: E402
import run_tests          # noqa: E402

STATES = ["Created", "Ready", "Submitted", "Running", "Complete",
          "Unsuccessful", "Killed"]
failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def rect(answer, key):
    m = re.search(r"\b%s=(-?\d+),(-?\d+),(-?\d+),(-?\d+)" % re.escape(key), answer)
    return tuple(int(v) for v in m.groups()) if m else None


def overlap(a, b):
    return (a[2] > 0 and a[3] > 0 and b[2] > 0 and b[3] > 0 and
            a[0] < b[0] + b[2] and b[0] < a[0] + a[2] and
            a[1] < b[1] + b[3] and b[1] < a[1] + a[3])


def judge(tag, answer):
    viewer = rect(answer, "toolrect:Viewer")
    launcher = rect(answer, "toolrect:Launcher")
    boxr = rect(answer, "boxrect")
    if not check(viewer and launcher and boxr, tag + ": positions reported", answer):
        return
    print("      %s: viewer=%s launcher=%s box=%s" % (tag, viewer, launcher, boxr))
    check(not overlap(viewer, boxr), tag + ": Viewer button outside the Progress box",
          "viewer=%s box=%s" % (viewer, boxr))
    check(viewer[1] > launcher[1], tag + ": Viewer below the Launcher row",
          "viewer=%s launcher=%s" % (viewer, launcher))
    if boxr[2] > 0 and boxr[3] > 0:
        check(boxr[1] + boxr[3] <= viewer[1],
              tag + ": a shown Progress box sits above the Viewer row",
              "viewer=%s box=%s" % (viewer, boxr))


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-viewer-data-")
    work = tempfile.mkdtemp(prefix="ecce-viewer-")
    try:
        project = os.path.join(data, "users", "local", "viewer-test")
        os.makedirs(project)
        shutil.copy(box.PROJECT_META, os.path.join(project, ".ecce-meta"))
        urls = {}
        for st in STATES:
            urls[st] = "file://" + box.makeCalc(project, "calc-" + st.lower(), st, None)
        urls["Terminated"] = "file://" + box.makeCalc(project, "calc-terminated",
                                                     "Submitted", None)
        lines = []
        for st in STATES:
            lines += ["summary " + urls[st], "progressbox"]
        lines += ["summary " + urls["Terminated"], "progressbox",
                  "setstate %s killed" % urls["Terminated"], "progressbox"]
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as handle:
            handle.write("size 1100 1000\n" + "\n".join(lines) + "\n")
        result = apps.run(display, "organizer", windowTimeout=60, settle=45,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + project,
                               "ECCE_LOCAL_DATA": data})
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return
        got = box.answers(log, "progressbox")
        if not check(len(got) == len(STATES) + 2,
                     "every command answered (%d)" % len(got),
                     run_tests._tail(log, 30)):
            return
        for st, answer in zip(STATES + ["Submitted", "terminated -> Killed"], got):
            judge(st, answer)
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("viewer", "Viewer button misplaced: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
