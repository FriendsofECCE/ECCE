#!/usr/bin/env python3
"""
tools/screenshots/shots.py end to end (#860): two Builder shots of the water
optimisation, one measuring and one assigning Mulliken charges, written on a
white 3-D background.  Checks the hooks' own state lines in the Builder log
(label drawn, nothing selected, charges stored) and that the picture has a
white viewer corner, not only that a PNG exists.

Same installed tree and isolation as shot_hooks_test.py.  Exit 77 (skip)
without an installed tree, Xvfb or ImageMagick.
"""

import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SKIP = 77
LIST = {
    "calculations": {"water": "tests/apps/fixtures/calc-water-opt"},
    "shots": [
        {"name": "measure", "calc": "water", "size": [1100, 800],
         "script": ["measure dist 0 1"]},
        {"name": "charges", "calc": "water", "panel": "Mulliken Charges",
         "size": [1100, 800], "script": ["assigncharges Mulliken Charges"]},
    ],
}


def pixel(path, x, y):
    out = subprocess.run(["convert", path, "-format",
                          "%%[pixel:p{%d,%d}]" % (x, y), "info:"],
                         stdout=subprocess.PIPE).stdout.decode()
    return out.strip()


def main():
    install = os.environ.get("ECCE_TEST_HOME", "/opt/ecce")
    if not (shutil.which("Xvfb") and shutil.which("convert")
            and shutil.which("import")
            and os.path.exists(os.path.join(install, "bin", "builder"))):
        print("SKIP: needs Xvfb, ImageMagick and an installed tree")
        return SKIP
    work = tempfile.mkdtemp(prefix="shots-driver-",
                            dir=os.environ.get("XDG_CACHE_HOME"))
    try:
        listing = os.path.join(work, "list.json")
        with open(listing, "w") as handle:
            json.dump(LIST, handle)
        run = subprocess.run(
            [sys.executable, os.path.join(REPO, "tools", "screenshots", "shots.py"),
             listing, "--out", os.path.join(work, "out"), "--tmp",
             os.path.join(work, "tmp"), "--display", "133"],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        text = run.stdout.decode(errors="replace")
        problems = []
        if run.returncode:
            problems.append("shots.py failed: " + text[-800:])
        for name in ("measure", "charges"):
            png = os.path.join(work, "out", name + ".png")
            if not os.path.exists(png):
                problems.append("no %s.png" % name)
                continue
            #  Inside the viewer, near its top left corner: the white 3-D background.
            if pixel(png, 300, 300) not in ("white", "srgb(255,255,255)"):
                problems.append("%s: viewer background is %s" % (name, pixel(png, 300, 300)))
        def log(name):
            try:
                return open(os.path.join(work, "tmp", name + ".log"),
                            errors="replace").read()
            except OSError:
                return ""
        if not re.search(r"SHOTHOOK: measure dist labels=1 selected=0", log("measure")):
            problems.append("measure: no label drawn or atoms left selected")
        m = re.search(r"SHOTHOOK: assigncharges .* first=(\S+) sum=(\S+) label1=\[(\S+)\]",
                      log("charges"))
        if not m or abs(float(m.group(1))) < 0.05:
            problems.append("charges: not assigned (%s)" % (m.group(0) if m else None))
        print("%s  screenshot driver%s" % ("ok  " if not problems else "FAIL",
                                           "" if not problems else ": " + "; ".join(problems)))
        return 1 if problems else 0
    finally:
        if os.environ.get("SHOTS_DRIVER_KEEP"):
            print("kept %s" % work)
        else:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
