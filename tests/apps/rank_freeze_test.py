#!/usr/bin/env python3
"""Rank and Checks Table must come back for any folder (#793).

    tests/apps/rank_freeze_test.py        own Xvfb, services, local data

Four folders go through the Organizer's "rank" and "checks" test hooks: a
calculation killed before it ran, an empty folder, and (the timing case)
60 calculations whose MO table is 1461x1461.  Each hook must answer inside
a limit; the big folder's Rank gather time is printed.  Exit 77 without an
install or Xvfb.  ECCE_FREEZE_CALCS / ECCE_FREEZE_MO set the big folder.
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
NS = "http://www.emsl.pnl.gov/ecce:"
NCALCS = int(os.environ.get("ECCE_FREEZE_CALCS", "60"))
NMO = int(os.environ.get("ECCE_FREEZE_MO", "1461"))
LIMIT = 120     # seconds for the big folder; the cases without data need seconds
failures = []


def check(ok, what, detail=""):
    print(("ok   " if ok else "FAIL ") + what + ("" if ok else "  " + str(detail)))
    if not ok:
        failures.append(what)
    return ok


def bigMo(path, n):
    # One shared file: the calculations symlink to it.
    labels = lambda p: " ".join("%s-%d" % (p, i + 1) for i in range(n))
    row = " ".join("0.001" for _ in range(n))
    with open(path, "w") as h:
        h.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                '<table columnLabel="Basis Function" columnLabels="%s" columns="%d" '
                'name="MO" rowLabel="Basis Function" rowLabels="%s" rows="%d" '
                'units="NA">' % (labels("BasFun"), n, labels("MO"), n))
        for _ in range(n):
            h.write(row + "\n")
        h.write("</table>\n")


def setState(calc, state):
    meta = os.path.join(calc, ".ecce-meta")
    lines = [l for l in open(meta) if ":state\t" not in l]
    lines.append(".\t%sstate\t\t%s\n" % (NS, state))
    open(meta, "w").writelines(lines)


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-frz-data-")
    work = tempfile.mkdtemp(prefix="ecce-frz-")
    try:
        user = os.path.join(data, "users", "local")
        folders = {}
        for name in ("killed", "empty", "big"):
            p = folders[name] = os.path.join(user, name)
            os.makedirs(p)
            shutil.copy(PROJECT_META, os.path.join(p, ".ecce-meta"))
        # Killed before it ran: no outputs, no properties.
        k = os.path.join(folders["killed"], "calc")
        shutil.copytree(SRC, k, symlinks=True)
        shutil.rmtree(os.path.join(k, "Props"))
        os.makedirs(os.path.join(k, "Props"))
        shutil.rmtree(os.path.join(k, "Outputs"), ignore_errors=True)
        os.makedirs(os.path.join(k, "Outputs"))
        setState(k, "Killed")
        mo = os.path.join(work, "MO")
        bigMo(mo, NMO)
        for i in range(NCALCS):
            c = os.path.join(folders["big"], "c%03d" % i)
            shutil.copytree(SRC, c, symlinks=True)
            os.remove(os.path.join(c, "Props", "MO"))
            os.symlink(mo, os.path.join(c, "Props", "MO"))
        cmds = os.path.join(work, "commands")
        outs = {}
        with open(cmds, "w") as h:
            for case in ("killed", "empty", "big"):
                for tool in ("checks", "rank"):
                    outs[case, tool] = os.path.join(work, "%s-%s.csv" % (case, tool))
                    h.write("%s %s file://%s\n" % (tool, outs[case, tool], folders[case]))
        t0 = time.time()
        result = apps.run(display, "organizer", windowTimeout=60, settle=90,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + user,
                               "ECCE_LOCAL_DATA": data})
        print("organizer session: %.1f s" % (time.time() - t0))
        log = result.log or ""
        for line in log.splitlines():
            if re.search(r"(checks|rank)\b.*(ok|failed)|gather", line):
                print("  " + line)
        for m in re.finditer(r"ECCE_TEST_ORGANIZER: (checks|rank) \S+ \S+/(\w+):\s.*\(([\d.]+) s\)", log):
            if m.group(2) == "big":
                check(float(m.group(3)) < LIMIT, "%s of the big folder took %s s" %
                      (m.group(1), m.group(3)))
        check(not result.crashed, "the Organizer did not crash",
              getattr(result, "signalName", ""))
        for tool in ("checks", "rank"):
            check(os.path.exists(outs["killed", tool]) and
                  "calc" in open(outs["killed", tool]).read(),
                  "%s: the killed calculation gets a row" % tool, log[-800:])
            n = len(open(outs["big", tool]).read().splitlines()) - 1 \
                if os.path.exists(outs["big", tool]) else -1
            check(n == NCALCS, "%s: one row per calculation of the big folder" % tool, n)
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("freeze", "rank/checks freeze: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
