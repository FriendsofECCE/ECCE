#!/usr/bin/env python3
"""
Builder: Save follows the unsaved-changes flag the close prompt reads, the
Open structures pane starts at about three rows, and no blank "Unnamed"
structure is left next to the calculation the Builder opened; and the
Calculation Editor's Geometry Constraints dialog shows every column.

Local data mode, calculations made by tests/filedsi/resourceTest (MOPAC
methane).  ECCE_BUILDER_SCRIPT "savestate"/"savemenu" report and use the
Save item; ECCE_CALCED_SCRIPT "constraints N" fills the dialog and reports
each grid.  Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402

failures = []


def check(ok, what, detail=""):
    print("%s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
        if detail:
            print(detail)


def states(log):
    return re.findall(r"BUILDER: dirty=(\d) save_enabled=(\d) "
                      r"toolbar_save_enabled=(\d)", log)


def builderCase(display, url):
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT",
                       ["wait 4000", "savestate",
                        "add H Bent 3 0 0", "savestate", "savemenu",
                        "wait 1500", "savestate",
                        "add X Terminal 5 0 0", "savestate",
                        "cmd clean", "savestate",
                        "info", "wait 500", "quit"], url)
    st = states(log)
    tail = run_tests._tail(log, 20)
    check(len(st) == 5, "five savestate lines (%d)" % len(st), tail)
    if len(st) != 5:
        return
    check(st[0] == ("0", "0", "0"), "unedited: clean, Save disabled %s" % (st[0],), tail)
    check(st[1] == ("1", "1", "1"), "after adding an atom: dirty, menu and "
          "toolbar Save enabled %s" % (st[1],), tail)
    check(st[2] == ("0", "0", "0"), "Save menu event saves and clears %s" % (st[2],), tail)
    check(st[3] == ("1", "1", "1"), "after adding a dummy X: Save enabled %s" % (st[3],), tail)
    check(st[4] == ("1", "1", "1"), "after Clean: Save enabled %s" % (st[4],), tail)
    m = re.search(r"CONTEXTS: (\d+)", log)
    check(m is not None and m.group(1) == "1",
          "one open structure, no blank Unnamed (%s)" % (m.group(1) if m else "?"), tail)
    m = re.search(r"CONTEXTPANE: min_height=(\d+) rows=(\d+)", log)
    check(m is not None and 40 <= int(m.group(1)) <= 110,
          "Open structures pane is about three rows high (%s)"
          % (str(m.groups()) if m else "?"), tail)


def calcedCase(display, url, kind=""):
    log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT",
                       ["wait 6000", "constraints 20 " + kind,
                        "wait 1500", "quit"], url)
    tail = run_tests._tail(log, 20)
    grids = re.findall(r"grid (\w+) rows=(\d+) cols=(\d+) width=(\d+) "
                       r"colsneed=(\d+) virtual=(\d+)x(\d+) client=(\d+)x(\d+) "
                       r"hscroll=(\d)", log)
    check(len(grids) >= 1, "constraint grids reported (%d)" % len(grids), tail)
    for g in grids:
        name, rows, cols, width, need, vx, vy, cx, cy, hs = g
        check(hs == "0" and int(width) >= int(need),
              "%s grid: %s columns fit, no horizontal scroll (width %s, need %s)"
              % (name, cols, width, need), tail)
        check(int(cy) < 200, "%s grid: %s rows, height %s capped, scrolls "
              "vertically" % (name, rows, cy), tail)
    m = re.search(r"dialog (\d+)x(\d+) display=(\d+)x(\d+)", log)
    check(m is not None and int(m.group(2)) <= int(m.group(4)) and int(m.group(2)) <= 768,
          "dialog fits the screen (%s)" % (str(m.groups()) if m else "?"), tail)


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "builderedit-localdata")
        problem = sym.makeCalculations(folder)
        if problem:
            results.fail("builder-edit", "calculations not made:\n" + problem)
            return
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            url = "file://" + os.path.join(folder, "users", "local",
                                           "proj-mopac", "ch4") + "/"
            builderCase(display, url)
            calcedCase(display, url)
            calcedCase(display, url, "fixed")
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if failures:
            results.fail("builder-edit", "%d wrong: %s"
                         % (len(failures), "; ".join(failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
