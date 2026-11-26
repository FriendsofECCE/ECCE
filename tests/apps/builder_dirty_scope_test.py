#!/usr/bin/env python3
"""
A Builder is marked modified only by its own edits (#1307).

In one-program mode every Builder and Viewer tab shares one event
dispatcher, and the change events carry no sender. Each scenario uses a
fresh Duplicate (never run), opened in a Builder tab:

  negative  two Viewers of other calculations are opened and closed, then the
            Builder is closed: no "Save Builder Changes?" question.
  control   a Viewer is open, a real edit is made in the Builder
            (`add C Tetrahedral 8 0 0` from its script), the Builder is
            closed: it MUST ask.
  control2  as control, and a NEW Viewer opens after the edit: it must still
            ask (the scoping must not clear a real dirty flag).

Driven through the Organizer hook files and ECCE_BUILDER_SCRIPT, as the
release round does (tests/releaseround/run_round.py). Exit 77 (skip)
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
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "releaseround"))

import run_round as rr    # noqa: E402
import run_tests          # noqa: E402

failures = []


def check(ok, what, detail=""):
    print("%-5s %s" % ("ok" if ok else "FAIL", what), flush=True)
    if not ok:
        failures.append(what)
        if detail:
            print("      " + detail[-1500:], flush=True)


def checkDirty(display):
    import one_process_test as op
    op.setLayout(display, "normal")
    root = tempfile.mkdtemp(prefix="ecce-dirtyscope-")
    r = rr.Round(display, root)
    org = r.org("scope")
    try:
        ans, _ = org.start()
        a = org.send("examples", 300) or ""
        calcs = rr.calcsUnder(rr.urlPath(a.split()[1], r.data)) \
            if a.startswith("ok ") else []
        done = [c for c in calcs if rr.completed(c[2])] or calcs
        if not check(len(done) >= 2, "two finished example calculations",
                     "examples: %r startup: %r" % (a, ans)):
            return
        r.home(org)

        def dup():
            before = set(c[0] for c in rr.calcsUnder(rr.urlPath(r.project)))
            org.send("server paste copy %s file://%s" % (r.project, done[0][0]), 120)
            pasted = [c[0] for c in rr.calcsUnder(rr.urlPath(r.project))
                      if c[0] not in before]
            org.send("duplicate file://" + pasted[0], 120)
            fresh = [c[0] for c in rr.calcsUnder(rr.urlPath(r.project))
                     if c[0] not in before and c[0] not in pasted]
            return "file://" + fresh[0]

        def open_(line):
            org.send(line)
            org.waitTool()
            time.sleep(3)

        def openBuilder(url, lines):
            org.builderScript(lines)
            open_("start Builder " + url)
            # The Viewers opened next must not run the Builder's script.
            open(org.script, "w").close()

        def waitEdit(since, timeout=90):
            end = time.time() + timeout
            while time.time() < end:
                if "BUILDER: atoms=" in org.logText(since):
                    return True
                time.sleep(1)
            return False

        edit = ["wait 25000", "add C Tetrahedral 8 0 0", "info", "wait 45000"]

        # negative
        u = dup()
        openBuilder(u, ["wait 2000"])
        open_("start CalculationViewer file://" + done[0][0])
        open_("start CalculationViewer file://" + done[1][0])
        org.closeTab("Viewer")
        org.closeTab("Viewer")
        asked, tabs = org.closeTab("Builder")
        check(asked is False, "unedited Builder beside two Viewers closes "
              "without a question", "asked=%s tabs=%s" % (asked, tabs))

        # control: edit with a Viewer open
        u = dup()
        mark = org.mark()
        openBuilder(u, edit)
        open_("start CalculationViewer file://" + done[1][0])
        got = waitEdit(mark)
        asked, tabs = org.closeTab("Builder")
        check(got and asked is True, "a real edit in the Builder, Viewer "
              "open, still asks on close",
              "edit ran=%s asked=%s tabs=%s" % (got, asked, tabs))

        # control2: a new Viewer opens after the edit
        u = dup()
        mark = org.mark()
        openBuilder(u, edit)
        open_("start CalculationViewer file://" + done[1][0])
        got = waitEdit(mark)
        open_("start CalculationViewer file://" + done[0][0])
        asked, tabs = org.closeTab("Builder")
        check(got and asked is True, "an edited Builder stays modified when "
              "a new Viewer opens",
              "edit ran=%s asked=%s tabs=%s" % (got, asked, tabs))
    finally:
        org.quit()
        org.kill()
        shutil.rmtree(root, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        checkDirty(display)
        if failures:
            results.fail("builder_dirty_scope", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer", "--budget", "1800"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
