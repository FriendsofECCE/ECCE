#!/usr/bin/env python3
"""Organizer auto-refresh of the shown folder (#889), headless.

A calculation copied into the selected local project from outside shows in the
tree within a few seconds with the selection unchanged; removing it removes
it; while a modal dialog is open (hook "modal N") no reload runs until it
closes.  Exit 77 (skip) without an install or Xvfb.
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
import fixture    # noqa: E402
import run_tests  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data", "project.ecce-meta")
SRC = os.path.join(fixture.FIXTURES, "calc-water-opt")
failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + str(detail)) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-autoref-data-")
    work = tempfile.mkdtemp(prefix="ecce-autoref-")
    try:
        project = os.path.join(data, "users", "local", "auto")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        shutil.copytree(SRC, os.path.join(project, "water"), symlinks=True)
        u = "file://" + project
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def send(*lines):
            with open(cmds, "a") as h:
                for line in lines:
                    h.write(line + "\n")

        def drive(_display):
            send("select " + u, "autoshown x", "treechildren " + u)
            time.sleep(4)
            send("tableselect " + u + "/water")
            time.sleep(2)
            shutil.copytree(SRC, os.path.join(project, "added"), symlinks=True)
            time.sleep(5)
            send("treechildren " + u, "selection x", "autoreloads x", "autoshown x", "tableselected x")
            time.sleep(2)
            shutil.rmtree(os.path.join(project, "added"))
            time.sleep(5)
            send("treechildren " + u)
            time.sleep(2)
            send("autoreloads x", "modal 8")
            time.sleep(2)
            shutil.copytree(SRC, os.path.join(project, "during"), symlinks=True)
            time.sleep(4)
            send("autoreloads x", "treechildren " + u)   # answered inside the dialog
            time.sleep(8)
            send("autoreloads x", "treechildren " + u)
            time.sleep(3)

        result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": u,
                               "ECCE_LOCAL_DATA": data}, inspect=drive)
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return

        def ans(word):
            return re.findall(r"ECCE_TEST_ORGANIZER: %s .*?: (.*)$" % word, log, re.M)
        kids, sel, reloads = ans("treechildren"), ans("selection"), ans("autoreloads")
        if not check(len(kids) == 5 and len(sel) == 1 and len(reloads) == 4 and
                     ans("autoshown")[0].startswith(u + " watched"),
                     "the hooks answered", log[-2000:]):
            return
        rows = ans("tableselected")
        check(len(rows) == 1 and rows[0].endswith("/water"),
              "the selected table row stays selected after the reload", rows)
        check("water" in kids[0], "the project was shown with its calculation", kids[0])
        check("added" not in kids[0], "no copy yet", kids[0])
        check("added" in kids[1], "a copied-in calculation appears by itself", "%s reloads=%s %s" % (kids[1], reloads, ans("autoshown")))
        check(sel[0].rstrip("/") == u, "the selection is unchanged", sel[0])
        check("added" not in kids[2] and "water" in kids[2],
              "a removed calculation disappears", kids[2])
        check("during" not in kids[3], "no reload while a modal dialog is open", kids[3])
        check(reloads[2] == reloads[1] and int(reloads[3]) > int(reloads[2]),
              "no reload counted during the dialog, one after", reloads)
        check("during" in kids[4], "the reload runs once the dialog closes", kids[4])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("autorefresh", "auto-refresh wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
