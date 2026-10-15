#!/usr/bin/env python3
"""
Preferences > Colours > Run-state icons: the Organizer's tree follows at once.

A local-data project with a running calculation is opened through the
ECCE_TEST_ORGANIZER hook.  "iconstyle flat|shaded" does what the Preferences
dialog does in the Organizer's own process (saves the setting, publishes
ecce_preferences_misc); "treeicon <url>" sums the pixels of the tree icon
shown.  Flat must change the icon and Shaded bring the first one back.
The broker does not return a client's own messages to a filtered
subscription, so this fails if the Organizer filters that topic.

Same installed tree and isolation as progress_box_test.py.  Exit 77 (skip)
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

import apps               # noqa: E402
import progress_box_test as box   # noqa: E402
import run_tests          # noqa: E402

failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-iconstyle-data-")
    work = tempfile.mkdtemp(prefix="ecce-iconstyle-")
    try:
        project = os.path.join(data, "users", "local", "iconstyle-test")
        os.makedirs(project)
        shutil.copy(box.PROJECT_META, os.path.join(project, ".ecce-meta"))
        now = time.time()
        stamp = {"sent": now - 20, "at": int(now - 20)}
        running = box.makeCalc(project, "water-running", "Running",
                               box.STORED.format(**stamp))
        url = "file://" + running
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as handle:
            handle.write("\n".join([
                "summary " + url,
                "treeicon " + url,
                "iconstyle flat",
                "treeicon " + url,
                "iconstyle shaded",
                "treeicon " + url,
            ]) + "\n")
        result = apps.run(display, "organizer", windowTimeout=60, settle=40,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + running,
                               "ECCE_LOCAL_DATA": data})
        log = result.log or ""
        sums = re.findall(r"ECCE_TEST_ORGANIZER: treeicon.*?: sum=(\d+)", log)
        if not check(len(sums) == 3, "three tree icons read (%d)" % len(sums),
                     run_tests._tail(log, 30)):
            return
        shaded, flat, again = sums
        check(flat != shaded, "Flat changes the tree icon at once")
        check(again == shaded, "Shaded brings the shaded icon back")
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("iconstyle", "tree icons did not follow: %s"
                         % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
