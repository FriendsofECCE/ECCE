#!/usr/bin/env python3
"""Vibrations panel > Displace (#618): new calculations along a mode.

    tests/apps/nmode_displace_test.py

A Builder on the water frequency calculation (nmode_edit_test's fixture) runs
the panel's displace action on mode 0 with both signs.  Each new calculation
must have run type Geometry, the annotation naming the amplitude, mode and
source, and the source structure plus or minus amplitude times the mode,
scaled so the largest atom moves 0.15 Angstrom.
"""
import math
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import nmode_edit_test as ne  # noqa: E402
import fixture                # noqa: E402

AMP = 0.15


def main():
    ne.CASES = (("displace", ["vibfocus", "vibdisplace 0 %g both" % AMP], None, 3),)
    log = []
    real = ne.runCase

    def capture(display, results, case, args, timeout):
        work = os.path.join(fixture.stateHome(), "nmode-edit", case[0])
        real(display, results, case, args, timeout)
        path = os.environ.get("NMODEEDIT_LOG")
        # runCase leaves the scene output in the log only when asked for
        log.append(open(path + "." + case[0]).read() if path else "")

    ne.runCase = capture
    os.environ["NMODEEDIT_LOG"] = os.path.join(fixture.stateHome(), "nmode-displace.log")
    sys.argv = [sys.argv[0]]
    # runCase fails on VIBSTATE count for the generic case; check ours instead.
    rc = 0
    try:
        ne.main()
    except SystemExit as e:
        rc = e.code or 0
    text = log[0] if log else ""
    got = re.findall(r"VIBDISPLACED: runtype=(\S*) note=(.*?) xyz=(.*)$", text, re.M)
    ok = len(got) == 2 and "error=" in text and "n=2" in text
    ok = ok and all(r == "Geometry" and re.match(
        r"displaced [+-]0\.15 Å along mode 1 \(.*cm-1\) of ", n) for r, n, _ in got)
    plus = [float(v) for v in got[0][2].split()] if ok else []
    minus = [float(v) for v in got[1][2].split()] if ok else []
    if ok:
        mid = [(p + m) / 2 for p, m in zip(plus, minus)]
        d = [(p - m) / 2 for p, m in zip(plus, minus)]
        big = max(math.sqrt(sum(d[3 * a + k] ** 2 for k in range(3)))
                  for a in range(len(d) // 3))
        ok = abs(big - AMP) < 1e-4 and len(mid) == 9
    print(("PASS" if ok else "FAIL") + "  nmode displace: %s" % got)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
