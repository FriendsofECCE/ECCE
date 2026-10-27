#!/usr/bin/env python3
"""A Slurm job cancelled on the machine ends the monitor as cancelled (#636).

After scancel of a pending job, sacct says "CANCELLED by <uid>" and squeue
no longer lists it.  The monitor waited for the job to start for ever, so
ECCE kept it Submitted through every Reconnect.  Runs the real
eccejobmonitor with stand-in sacct and squeue on the PATH:

  a. sacct "CANCELLED by 4086", squeue empty -> status 304 (cancelled)
  b. sacct "FAILED", squeue empty             -> status 302 (no status file)

    tests/launch/slurm_cancel_test.py
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
MONITOR = os.path.join(REPO, "scripts", "eccejobmonitor")

CONF = """host localhost
calcName test
ecceVersion test
monitoringMode live
jobQ Slurm
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
"""


def run(sacctState):
    work = tempfile.mkdtemp(prefix="ecce-slurmcancel-")
    try:
        bin_ = os.path.join(work, "bin")
        os.makedirs(bin_)
        for name, body in (("sacct", "echo '%s'\n" % sacctState),
                           ("squeue", "exit 0\n")):
            path = os.path.join(bin_, name)
            with open(path, "w") as f:
                f.write("#!/bin/sh\n" + body)
            os.chmod(path, 0o755)
        for name, text in (("eccejobmonitor.conf", CONF), ("test.desc", DESC)):
            with open(os.path.join(work, name), "w") as f:
                f.write(text)
        env = dict(os.environ, PATH=bin_ + os.pathsep + os.environ["PATH"])
        try:
            mon = subprocess.run(
                ["perl", MONITOR, "-configFile", "eccejobmonitor.conf",
                 "-jobId", "48236722", "-bookmark", "0", "-progress", "1"],
                cwd=work, env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, timeout=120)
        except subprocess.TimeoutExpired:
            return None, "the monitor was still waiting after 120 s"
        return mon.returncode, mon.stdout.replace(b"\x00", b" ").decode(
            errors="replace")
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    if not shutil.which("perl"):
        print("SKIP: needs perl")
        return 77
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    rc, text = run("CANCELLED by 4086")
    check(rc == 0, "cancelled: the monitor ends (%s)" % (rc if rc is not None else text))
    check(re.search(r"jmSTATUS 304\b", text) is not None,
          "cancelled: status 304 sent")

    rc, text = run("FAILED")
    check(rc == 0, "failed: the monitor ends (%s)" % (rc if rc is not None else text))
    check(re.search(r"jmSTATUS 302\b", text) is not None and "jmSTATUS 304" not in text,
          "failed: status 302, not cancelled")

    if failures:
        print("FAILED: " + "; ".join(failures))
        return 1
    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
