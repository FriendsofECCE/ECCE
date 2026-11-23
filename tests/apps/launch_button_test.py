#!/usr/bin/env python3
"""
#1269: the Organizer's Launch button with one calculation selected opens
(or selects) that calculation's editor tab with the Launch panel shown,
instead of a Launcher tab.  The hook `launchbutton URL` is the button.
`start Launcher URL` (the batch paths' route) must still open a Launcher.

Normal layout (the Set-up tab exists), one process.  Exit 77 (skip)
without an install or Xvfb.
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
import one_process_test as op   # noqa: E402

failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def pages(line):
    """Tab captions of a `tabs` answer (the part before menus=)."""
    # Captions may hold spaces ("NWChem Editor"): each runs to the next "N:".
    return [c.rstrip("*") for c in
            re.findall(r"\d+:(.+?)(?=\s+\d+:|\s*$)", line.split("menus=")[0])]


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-launchbtn-data-")
    work = tempfile.mkdtemp(prefix="ecce-launchbtn-")
    try:
        project = os.path.join(data, "users", "local", "lbtn")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        url = "file://" + pb.makeCalc(project, "water-l", "created", None)
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            append("launchbutton " + url, 30)   # new editor tab
            append("tabs", 2)                   # tabs 1
            append("setup info", 2)             # info 1: area=launch
            append("setup input", 8)
            append("setup info", 2)             # info 2: area=input
            append("launchbutton " + url, 10)   # the same tab again
            append("tabs", 2)                   # tabs 2
            append("setup info", 2)             # info 3: area=launch
            append("tabclose 1", 5)
            append("answer no", 3)
            append("answer no", 3)
            append("tabs", 2)                   # tabs 3: Organizer only
            append("launchbutton " + url, 30)   # a new editor tab
            append("tabs", 2)                   # tabs 4
            append("setup info", 2)             # info 4: area=launch
            append("start Launcher " + url, 15)
            append("tabs", 2)                   # tabs 5: + Launcher
            append("close", 3)
            append("answer", 3)
            append("answer no", 3)
            append("answer no", 20)

        op.setLayout(display, "normal")
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_ONE_PROCESS": "1",
                               "ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_LOCAL_DATA": data},
                          inspect=inspect)
        log = result.log or ""
        tabs = pb.answers(log, "tabs")
        info = pb.answers(log, "setup info")
        print("      tabs: " + "\n            ".join(tabs))
        print("      setup info: " + str(info))
        if not check(len(tabs) == 5 and len(info) == 4,
                     "all hook answers arrived", str(tabs) + str(info)
                     + log[-1500:]):
            return
        ed = [sum("Editor" in p for p in pages(t)) for t in tabs]
        launcher = [sum("Launcher" in p for p in pages(t)) for t in tabs]
        check(ed[0] == 1 and launcher[0] == 0 and "area=launch" in info[0],
              "Launch button: one editor tab, Launch panel shown, no Launcher "
              "tab", str((tabs[0], info[0])))
        check("area=input" in info[1], "the editor was switched to the input",
              info[1])
        check(ed[1] == 1 and launcher[1] == 0 and "area=launch" in info[2],
              "Launch button again: still one editor tab, Launch panel shown",
              str((tabs[1], info[2])))
        check(ed[2] == 0 and launcher[2] == 0,
              "the editor tab closed", tabs[2])
        check(ed[3] == 1 and launcher[3] == 0 and "area=launch" in info[3],
              "Launch button after closing: a new editor tab on the Launch "
              "panel", str((tabs[3], info[3])))
        check(launcher[4] == 1 and ed[4] == 1,
              "start Launcher still opens a Launcher tab", tabs[4])
        check(not result.crashed and "Aborted" not in log,
              "no crash", result.signalName or log[-1500:])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("launch-button", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer", "--budget", "600"] + \
        sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
