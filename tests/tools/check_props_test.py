#!/usr/bin/env python3
"""scripts/ecce-check-props finds a cut-off table and passes sound ones (#719)."""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
TOOL = os.path.join(REPO, "scripts", "ecce-check-props")
GOOD = os.path.join(REPO, "tests", "apps", "fixtures", "calc-orca-water-ccpvdz")


def run(top):
    p = subprocess.run([sys.executable, TOOL, top], capture_output=True, text=True)
    return p.returncode, p.stdout


def main():
    work = tempfile.mkdtemp(prefix="ecce-checkprops-")
    failures = []
    try:
        rc, out = run(GOOD)
        if rc != 0 or out:
            failures.append("sound calculation flagged: " + out)
        calc = os.path.join(work, "proj", "bad")
        shutil.copytree(GOOD, calc)
        mo = os.path.join(calc, "Props", "MO")
        text = open(mo).read()
        end = text.rindex("</table>")
        cut = text[:end].rstrip().rsplit(None, 7)[0]      # drop 7 values
        open(mo, "w").write(cut + "</table>\n")
        rc, out = run(work)
        line = out.strip().split("\n")
        if rc != 1 or len(line) != 1:
            failures.append("expected one finding, got rc=%d: %r" % (rc, out))
        else:
            f = line[0].split("\t")
            ok = (f[0] == calc and f[1] == "MO" and f[2] == "ORCA"
                  and "rows=" in f[3] and "values=" in f[3])
            if not ok:
                failures.append("finding lacks calc/property/code/shape: %r" % line[0])
            print(line[0])
    finally:
        shutil.rmtree(work, ignore_errors=True)
    for f in failures:
        print("FAIL " + f)
    print("FAILED" if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
