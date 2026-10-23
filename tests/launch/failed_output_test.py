#!/usr/bin/env python3
"""A job that stops in the middle of a parsed section must still be stored.

A failed job's output usually ends inside a section the .desc reads (ORCA
printed MOLECULAR ORBITALS and died before MULLIKEN POPULATION ANALYSIS, the
entry's End).  eccejobmonitor treated that open block at the end of the file
as fatal: it exited before sending the output file and the status, the job
store restarted it from the last bookmark, which sent a progress snapshot
with almost nothing in it, and the calculation ended failed with no output
and its progress gone (#470, #467).

Runs the real scripts/eccejobmonitor in live mode with stdio comms on a
Shell job that writes such an output and exit status 231, and reads its
framed messages.  The monitor must end normally and send the output file,
the job's status 231, the progress, and the last complete block of the
open section -- not the cut-off one.

    tests/launch/failed_output_test.py
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
MONITOR = os.path.join(REPO, "scripts", "eccejobmonitor")

CONF = """host localhost
calcName test
ecceVersion test
monitoringMode live
jobQ Shell
commType stdio
jobOutputFile job.out
jobOutputFile2 job.out
parseTypes ALL
mdTask no
mdPrepareTask no
mdBatchOutput no
mdPMFOutput no
logMode yes
parseDescriptorFile test.desc
timePauseJobExist 1
"""

DESC = """[TE]
Begin=HEAT OF FORMATION\\s*=
Lines=1
Frequency=last
Script=none
[END]

[MO]
Begin=MOLECULAR ORBITALS
End=MULLIKEN POPULATION ANALYSIS
Frequency=last
Script=none
[END]

"""

OUTPUT = """progress opt step=1 energy=-1.0
MOLECULAR ORBITALS
  complete-block-one
MULLIKEN POPULATION ANALYSIS
progress opt step=2 energy=-1.5
HEAT OF FORMATION = -1.5
progress opt step=3 energy=-1.7
MOLECULAR ORBITALS
  cut-off-block-two
ERROR: the job stopped here
"""


def run():
    work = tempfile.mkdtemp(prefix="ecce-failedout-")
    try:
        for name, text in (("eccejobmonitor.conf", CONF), ("test.desc", DESC),
                           ("job.out.src", OUTPUT)):
            with open(os.path.join(work, name), "w") as f:
                f.write(text)
        job = subprocess.Popen(
            ["sh", "-c", "sleep 2; cp job.out.src job.out; "
                         "echo 231 > .ecce.status"], cwd=work)
        threading.Thread(target=job.wait, daemon=True).start()
        mon = subprocess.run(
            ["perl", MONITOR, "-configFile", "eccejobmonitor.conf",
             "-jobId", str(job.pid), "-bookmark", "0"],
            cwd=work, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, timeout=60)
        return mon.returncode, mon.stdout.replace(b"\x00", b" ")
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    if not shutil.which("perl") or not shutil.which("ps"):
        print("SKIP: needs perl and ps")
        return 77
    rc, out = run()
    text = out.decode(errors="replace")
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    check(rc == 0, "monitor ends normally (exit %d)" % rc)
    check("jmERROR fatal" not in text, "no fatal error sent")
    check(re.search(r"jmFILE\s+output\s+\S*\s*\d+\s+job\.out", text),
          "output file job.out sent")
    check(re.findall(r"jmSTATUS\s*(\d+)", text)[-1:] == ["231"],
          "job's own status 231 sent")
    check("complete-block-one" in text, "last complete MO block sent")
    check("cut-off-block-two" not in text, "cut-off MO block not sent")
    check("HEAT OF FORMATION" in text, "earlier property sent")
    if failures:
        print(text[-3000:])
        print("FAILED: " + "; ".join(failures))
        return 1
    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
