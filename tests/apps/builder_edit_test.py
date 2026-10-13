#!/usr/bin/env python3
"""
Builder: Save follows the unsaved-changes flag the close prompt reads, the
Open structures pane starts at about three rows, and no blank "Unnamed"
structure is left next to the calculation the Builder opened.  (The
Geometry Constraints dialog is tested in builder_constraints_test.py.)

Local data mode, calculations made by tests/filedsi/resourceTest (MOPAC
methane).  ECCE_BUILDER_SCRIPT "savestate"/"savemenu" report and use the
Save item.  Exit 77 (skip) without an install or Xvfb.
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
                        "popup selector",
                        "select 1", "info", "key viewer copy", "select -1",
                        "key viewer paste", "info", "key viewer undo", "info",
                        "key text cut", "info",
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
    # The element selector opens below its own tool, not at the pointer
    # (#304); the pointer of a fresh Xvfb is far from the toolbar.
    tool = re.search(r"BUILDER popup selector tool: (-?\d+),(-?\d+) (\d+)x(\d+)", log)
    pop = re.search(r"BUILDER popup selector after 500 ms: at (-?\d+),(-?\d+) "
                    r"size (\d+)x(\d+)", log)
    if tool and pop:
        tx, ty, tw, th = map(int, tool.groups())
        px, py, pw, ph = map(int, pop.groups())
        check(tw > 1 and th > 1, "the selector tool has a rectangle %s"
              % (tool.groups(),), tail)
        check(abs(py - (ty + th)) <= 2 and abs(px - tx) <= 2,
              "the periodic table opens below the selector (tool %s, "
              "popup %s)" % (tool.groups(), pop.groups()), tail)
    else:
        check(False, "the element selector reported where it opened", tail)
    # Ctrl+C/V/Z with the 3-D view focused edit the structure; Ctrl+X in a
    # text field cuts its text and leaves the structure alone.
    atoms = [int(a) for a in re.findall(r"BUILDER: atoms=(\d+)", log)]
    if len(atoms) >= 4:
        check(atoms[1] > atoms[0], "Ctrl+C, Ctrl+V in the view paste "
              "the selected atom (%s)" % atoms[:2], tail)
        check(atoms[2] == atoms[0], "Ctrl+Z in the view undoes the paste "
              "(%s)" % atoms[:3], tail)
        check(atoms[3] == atoms[0], "Ctrl+X in a text field leaves the "
              "structure alone (%s)" % atoms[:4], tail)
    else:
        check(False, "atom counts around the shortcuts (%s)" % atoms, tail)
    cut = re.search(r"key text cut: ok focus=(\S+) text=\[(.*)\]", log)
    check(cut is not None and cut.group(2) == "",
          "Ctrl+X in a text field cuts its text (%s)"
          % ((cut.groups() if cut else None),), tail)
    view = re.search(r"key viewer copy: ok focus=(\S+)", log)
    check(view is not None and "Text" not in view.group(1),
          "the view had the focus for Ctrl+C (%s)"
          % (view.group(1) if view else None), tail)
    m = re.search(r"CONTEXTPANE: min_height=(\d+) rows=(\d+)", log)
    check(m is not None and 40 <= int(m.group(1)) <= 110,
          "Open structures pane is about three rows high (%s)"
          % (str(m.groups()) if m else "?"), tail)


def paneSizeCase(display, url):
    """No tool pane beside Build is fixed (a fixed neighbour stops Build's
    sash, #293), and the panes still open at their content heights."""
    log = sym.scripted(display, "builder", "ECCE_BUILDER_SCRIPT",
                       ["wait 4000", "panel Build",
                        "panesize Open structures", "panesize Build",
                        "panesize Selection", "panesize Symmetry",
                        "quit"], url)
    tail = run_tests._tail(log, 25)
    seq = [(n, s, int(h), int(f), r) for n, s, h, f, r in re.findall(
        r"PANESIZE (.+?): (\w+) y=-?\d+ height=(-?\d+) "
        r"min_height=(\d+) resizable=(\d)", log)]
    check(len(seq) == 4, "four pane size reports (%d)" % len(seq), tail)
    if len(seq) != 4:
        return
    ctx, b0, sel0, sym0 = seq
    check(ctx[2] <= 140, "Open structures still opens short (%s)" % ctx[2], tail)
    check(b0[2] == 150 and sel0[2] == sel0[3] and sym0[2] == sym0[3],
          "panes open at their default heights (Build %s, Selection %s/%s, "
          "Symmetry %s/%s)" % (b0[2], sel0[2], sel0[3], sym0[2], sym0[3]), tail)
    check(sel0[4] == "1" and sym0[4] == "1", "Selection and Symmetry are not "
          "fixed, so they do not stop Build's sash", tail)


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
            paneSizeCase(display, url)
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
