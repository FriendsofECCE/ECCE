#!/usr/bin/env python3
"""Organizer "Move to..." and "Copy to..." (#929), headless.

The right-click menu of a calculation has both items.  The hook
"server moveto copy|cut SOURCE FOLDER" runs what the items do after the folder
dialog: Copy to leaves the original and makes a copy, Move to moves it, and a
target that Paste refuses (a folder into itself, the folder a calculation is
already in) answers with the refusal text.  Exit 77 (skip) without an install
or Xvfb.
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
    data = tempfile.mkdtemp(prefix="ecce-moveto-data-")
    work = tempfile.mkdtemp(prefix="ecce-moveto-")
    try:
        users = os.path.join(data, "users", "local")
        folders = {}
        for name in ("src", "dest", "dest2"):
            folders[name] = os.path.join(users, name)
            os.makedirs(folders[name])
            shutil.copy(PROJECT_META, os.path.join(folders[name], ".ecce-meta"))
        calc = os.path.join(folders["src"], "water")
        shutil.copytree(SRC, calc, symlinks=True)
        url = lambda p: "file://" + p
        lines = [
            "menulabels %s" % url(calc),
            "server moveto copy %s %s" % (url(calc), url(folders["dest"])),
            "server moveto cut %s %s" % (url(calc), url(folders["dest2"])),
            "server moveto copy %s %s" % (url(folders["dest"]), url(folders["dest"])),
            "server moveto cut %s %s" % (url(os.path.join(folders["dest2"], "water")),
                                         url(folders["dest2"])),
        ]
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as h:
            h.write("\n".join(lines) + "\n")

        result = apps.run(display, "organizer", windowTimeout=60, settle=40,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": url(folders["src"]),
                               "ECCE_LOCAL_DATA": data})
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return

        def ans(i):
            m = re.search(r"ECCE_TEST_ORGANIZER: %s: (.*)$" % re.escape(lines[i]),
                          log, re.M)
            return m.group(1) if m else None
        labels = ans(0)
        if not check(labels is not None, "the hooks answered", log[-1500:]):
            return
        for want in ("Move to...", "Copy to..."):
            check(want in labels, "calculation menu has " + want, labels)
        check(ans(1) == "ok", "Copy to answers ok", ans(1))
        check(os.path.isdir(os.path.join(folders["dest"], "water")),
              "Copy to made the copy")
        check(os.path.isdir(os.path.join(folders["src"], "water")) is False,
              "Move to took the original away") if ans(2) == "ok" else \
            check(False, "Move to answers ok", ans(2))
        check(os.path.isdir(os.path.join(folders["dest2"], "water")),
              "Move to put the calculation in the folder")
        check(os.path.isdir(os.path.join(folders["dest"], "water")),
              "the earlier copy is still there")
        r = ans(3) or ""
        check(r.startswith("refused: ") and "child folder" in r,
              "a folder into itself is refused with the text", r)
        r = ans(4) or ""
        check(r.startswith("refused: ") and "already in this folder" in r,
              "Move to the same folder is refused with the text", r)
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("moveto", "Move to/Copy to wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
