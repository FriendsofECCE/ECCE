#!/usr/bin/env python3
"""Organizer menus and project table (#805), headless.

The ECCE_TEST_ORGANIZER hook "menulabels URL" lists the right-click menu of
an item: a calculation has Cut/Copy/Paste/Rename/Delete, "New from This" and
"Run" submenus and no tool list, Checks, Rank, Exclude or New Series items;
a folder offers Compare Energies.  "tablecolumns" and "tableenergies" give
the project table's headings (no Viewed, Size or separate Application; one
Type; a Total energy column) and its Total energy cells.  Exit 77 (skip)
without an install or Xvfb.
"""
import os
import re
import shutil
import sys
import tempfile

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
    data = tempfile.mkdtemp(prefix="ecce-menus-data-")
    work = tempfile.mkdtemp(prefix="ecce-menus-")
    try:
        project = os.path.join(data, "users", "local", "menus")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "water")
        shutil.copytree(SRC, calc, symlinks=True)
        u = "file://" + calc
        cmds = os.path.join(work, "commands")
        shots = os.environ.get("ECCE_SHOTS805", "")
        with open(cmds, "w") as h:
            h.write("menulabels %s\n" % u)
            h.write("menulabels file://%s\n" % project)
            h.write("tablecolumns x\n")
            h.write("tableenergies x\n")
            if shots:
                os.makedirs(shots, exist_ok=True)
                h.write("snap %s/organizer.png\n" % shots)
        result = apps.run(display, "organizer", windowTimeout=60, settle=40,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + project,
                               "ECCE_LOCAL_DATA": data})
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return

        def ans(word):
            return re.findall(r"ECCE_TEST_ORGANIZER: %s .*?: (.*)$" % word, log, re.M)
        menus = ans("menulabels")
        if not check(len(menus) == 2, "the hooks answered", log[-1500:]):
            return
        m = [x.strip() for x in menus[0].split(" | ")]
        for want in ("Cut", "Copy", "Paste", "Rename", "Delete", "New from This",
                     "New from This/Rerun", "New from This/Continue from Last Geometry",
                     "New from This/Continue from Final Orbitals", "Run",
                     "Run/Launch...", "Run/Reconnect...", "Run/Terminate",
                     "Run/Re-Parse Outputs", "Run/Shell in Run Directory"):
            check(any(x == want or x.startswith(want + " (disabled)") or
                      x.replace("&", "") == want for x in m),
                  "calculation menu has " + want, m)
        for bad in ("Builder", "Calculation Editor", "Basis Set Tool", "Viewer",
                    "Checks", "Rank", "Exclude", "Include again", "New Series",
                    "Duplicate", "Supporting"):
            check(not any(bad in x for x in m), "calculation menu has no " + bad, m)
        f = menus[1]
        check("Compare Energies..." in f, "a folder offers Compare Energies", f)
        check("Checks" not in f and "Rank" not in f, "a folder has no Checks or Rank", f)

        heads = [x.strip() for x in " ".join(ans("tablecolumns")).split("|")]
        check("Total energy" in heads and "Type" in heads, "table has Type and Total energy", heads)
        check(not any(x in heads for x in ("Viewed", "Size", "Application")),
              "table has no Viewed, Size or Application column", heads)
        en = ans("tableenergies")
        check(bool(en) and re.search(r"water=\S*\d+\.\d{6}$", en[0].strip()) is not None,
              "Total energy cell is a six-decimal number", en)
        check(bool(en) and ("water=−" in en[0] or "water=-" not in en[0]),
              "negative energies use U+2212", en)
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("menus", "menus wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
