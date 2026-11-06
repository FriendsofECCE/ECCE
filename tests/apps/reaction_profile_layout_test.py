#!/usr/bin/env python3
"""
Reaction overview energy profile: no label may touch a line or another label.

The ECCE_TEST_ORGANIZER hook "reactionprofile dE_ts dG_ts dE_p dG_p [png]"
lays out a profile from those numbers alone and prints its label and line
rectangles; profile_layout.overlaps() checks them.  The cases are those where
labels are crowded: the SN2 numbers, a TS and product close in energy, a
negative barrier in dE, a negative barrier in dG, a product above the TS, and
nearly equal dE and dG.  ECCE_PROFILE_SHOTS=<dir> keeps the PNGs.  Exit 77
(skip) without an install or Xvfb.
"""

import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps            # noqa: E402
import run_tests       # noqa: E402
import profile_layout  # noqa: E402

CASES = [
    ("sn2", (27.3, 27.9, -9.5, -9.0)),
    ("ts-product-close", (30.0, 31.0, 29.4, 30.6)),
    ("negative-dE-barrier", (-4.0, 6.5, -20.0, -15.0)),
    ("negative-dG-barrier", (-6.0, -2.0, -12.0, -10.0)),
    ("product-above-ts", (20.0, 22.0, 25.0, 24.0)),
    ("flat", (0.3, 0.5, -0.2, 0.1)),
    ("large", (180.0, 175.0, -250.0, -240.0)),
]

failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display):
    work = tempfile.mkdtemp(prefix="ecce-profile-")
    shots = os.environ.get("ECCE_PROFILE_SHOTS") or work
    try:
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as f:
            for name, v in CASES:
                f.write("reactionprofile %s %s %s %s %s\n" % (v + (os.path.join(shots, name + ".png"),)))
        data = os.path.join(work, "data")
        os.makedirs(os.path.join(data, "users", "local"))
        result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                          env={"ECCE_TEST_ORGANIZER": cmds, "ECCE_LOCAL_DATA": data})
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return
        got = re.findall(r"ECCE_TEST_ORGANIZER: reactionprofile .*?: (ok.*)", log)
        if not check(len(got) == len(CASES), "every case answered", run_tests._tail(log, 30)):
            return
        for (name, v), answer in zip(CASES, got):
            lines = answer.split(" | ")
            bad = profile_layout.overlaps(lines)
            check(not bad, "%s: no label touches a line or another label" % name, "; ".join(bad[:8]))
            kinds = {t[4].split(":")[0] for t in profile_layout.parse(lines).get("1", {}).get("texts", [])}
            check({"title", "legend", "tick", "name", "barrier", "reaction", "axis-label"} <= kinds,
                  "%s: every label is drawn" % name, str(kinds))
            check("png saved" in lines, "%s: PNG written" % name)
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("profile", "energy profile layout wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
