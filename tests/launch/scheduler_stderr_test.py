#!/usr/bin/env python3
"""The queue system's stderr file comes back with the job's outputs.

A job on Slurm/PBS/SGE can fail with its real error only in the file the
submit script names (--error, -e).  eccejobmonitor reads that name from the
submit script and sends the file as a "sched" file message, unless it is
empty or the job exited 0 and it is large.  Runs the real monitor on a
Shell-queue job with a submitScript, under three cases: failing job with
text in slurm.err, failing job with an empty one, successful job with a
large one.

    tests/launch/scheduler_stderr_test.py
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
submitScript submit.sh
"""

DESC = """[TE]
Begin=HEAT OF FORMATION\\s*=
Lines=1
Frequency=last
Script=none
[END]
"""

SCRIPT = """#!/bin/sh
#SBATCH --nodes=1
#SBATCH --output=slurm.out
#SBATCH --error=slurm.err
"""


def run(errtext, status):
    work = tempfile.mkdtemp(prefix="ecce-schederr-")
    try:
        files = (("eccejobmonitor.conf", CONF), ("test.desc", DESC),
                 ("submit.sh", SCRIPT), ("slurm.err.src", errtext))
        for name, text in files:
            with open(os.path.join(work, name), "w") as f:
                f.write(text)
        job = subprocess.Popen(
            ["sh", "-c", "sleep 2; echo 'HEAT OF FORMATION = -1.0' > job.out; "
                         "cp slurm.err.src slurm.err; echo %d > .ecce.status"
             % status], cwd=work)
        threading.Thread(target=job.wait, daemon=True).start()
        mon = subprocess.run(
            ["perl", MONITOR, "-configFile", "eccejobmonitor.conf",
             "-jobId", str(job.pid), "-bookmark", "0", "-progress", "1"],
            cwd=work, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, timeout=60)
        return mon.returncode, mon.stdout.replace(b"\x00", b" ").decode(
            errors="replace")
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    if not shutil.which("perl") or not shutil.which("ps"):
        print("SKIP: needs perl and ps")
        return 77
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    sent = r"jmFILE\s+sched\s+\S*\s*\d+\s+slurm\.err"

    rc, text = run("slurmstepd: error: oom-kill event\n", 1)
    check(rc == 0, "failing job: monitor ends normally (exit %d)" % rc)
    check(re.search(sent, text) is not None,
          "failing job: non-empty slurm.err sent")

    rc, text = run("", 1)
    check(rc == 0 and re.search(sent, text) is None,
          "failing job: empty slurm.err not sent")

    rc, text = run("warning line\n" * 10000, 0)
    check(rc == 0 and re.search(sent, text) is None,
          "successful job: large slurm.err not sent")

    rc, text = run("warning line\n", 0)
    check(rc == 0 and re.search(sent, text) is not None,
          "successful job: small slurm.err sent")

    if failures:
        print("FAILED: " + "; ".join(failures))
        return 1
    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
