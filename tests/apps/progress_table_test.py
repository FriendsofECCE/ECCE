#!/usr/bin/env python3
"""
The Organizer's project table: the Progress column (#284), headless.

A local-data project with four calculations (running with a stored snapshot,
running without one, completed, never launched) is selected through the
ECCE_TEST_ORGANIZER hook; "table" prints each row's Progress cell and "snap"
saves the window:

- the stored snapshot of a Running calculation shows as one line
  ("Opt step 4 - SCF 7"), nothing for the other states;
- a live ecce_url_progress message changes that one cell ("Freq 14/54")
  without building the table again (same panel, one cell set, no tree or
  summary update);
- the column is in the Basic, Launch and All Properties views.

ECCE_PROGRESS_PNGS=<dir> keeps the window captures there.  Same installed
tree and isolation as progress_box_test.py.  Exit 77 (skip) without an
install or Xvfb.
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
import fixture            # noqa: E402
import progress_box_test as box   # noqa: E402
import run_tests          # noqa: E402

DOT = "·"
failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def cells(answer):
    return dict(c.split("=", 1) for c in answer.split(" | ")[1:])


def field(answer, key):
    m = re.search(r"\b%s=(\S+)" % key, answer)
    return m.group(1) if m else None


def run(display, pngs):
    data = tempfile.mkdtemp(prefix="ecce-progress-data-")
    work = tempfile.mkdtemp(prefix="ecce-progress-")
    try:
        project = os.path.join(data, "users", "local", "progress-test")
        os.makedirs(project)
        shutil.copy(box.PROJECT_META, os.path.join(project, ".ecce-meta"))
        now = time.time()
        stamp = {"sent": now - 20, "at": int(now - 20)}
        running = box.makeCalc(project, "water-running", "Running",
                               box.STORED.format(**stamp))
        box.makeCalc(project, "water-silent", "Running", None)
        box.makeCalc(project, "water-done", "Complete", box.DONE.format(**stamp))
        box.makeCalc(project, "water-new", "Created", None)
        live = os.path.join(work, "live.txt")
        with open(live, "w") as handle:
            handle.write(box.LIVE.format(sent=now, at=int(now)))
        png = lambda n: os.path.join(pngs, n)
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as handle:
            handle.write("\n".join([
                "size 1200 600",
                "table file://" + project,
                "snap " + png("progress-table-stored.png"),
                "counts",
                "progress file://%s %s" % (running, live),
                "table",
                "counts",
                "snap " + png("progress-table-live.png"),
                "progress file://%s %s" % (running, os.path.join(work, "none")),
                "table",
            ]) + "\n")
        result = apps.run(display, "organizer", windowTimeout=60, settle=45,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + project,
                               "ECCE_LOCAL_DATA": data})
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return
        tables = re.findall(r"ECCE_TEST_ORGANIZER: table.*?: (panel=.*)", log)
        counts = box.answers(log, "counts")
        if not check(len(tables) == 3 and len(counts) == 2,
                     "every command answered (%d tables, %d counts)" % (len(tables), len(counts)),
                     run_tests._tail(log, 30)):
            return
        for t in tables:
            print("      " + t)
        stored, after, forgotten = tables
        c0, c1 = cells(stored), cells(after)
        check(c0.get("water-running") == "Opt step 4 %s SCF 7" % DOT,
              "stored snapshot of a running calculation: %r" % c0.get("water-running"), stored)
        check(c0.get("water-silent") == "", "running, no snapshot: empty", stored)
        check(c0.get("water-done") == "" and c0.get("water-new") == "",
              "completed and never launched: empty", stored)
        check(c1.get("water-running") == "Freq 14/54",
              "live message: %r" % c1.get("water-running"), after)
        check(field(after, "panel") == field(stored, "panel"),
              "live: the same table (no rebuild)")
        check(int(field(after, "paints")) == int(field(stored, "paints")) + 1,
              "live: one cell set")
        check(counts[1].startswith("notices=0 applied=0"),
              "live: no tree or summary update", counts[1])
        check(cells(forgotten).get("water-running") == "Opt step 4 %s SCF 7" % DOT,
              "a new run forgets the live line: the stored one again", forgotten)
        for n in ("progress-table-stored.png", "progress-table-live.png"):
            check(os.path.exists(png(n)) and os.path.getsize(png(n)) > 1000,
                  "saved " + n)
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        keep = os.environ.get("ECCE_PROGRESS_PNGS")
        pngs = keep or tempfile.mkdtemp(prefix="ecce-progress-png-")
        os.makedirs(pngs, exist_ok=True)
        try:
            run(display, pngs)
        finally:
            if not keep:
                shutil.rmtree(pngs, ignore_errors=True)
        if failures:
            results.fail("progress", "progress column wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
