#!/usr/bin/env python3
"""A job that ends during the job monitor's last wait must not read as killed.

eccejobmonitor's JobWait() polls a Shell job until its output file appears,
inside an alarm.  When the alarm cut the last pause short, it asked only
whether the job was still alive: a job that had written its output and its
status file and exited during that pause was reported as "neither queued
nor running", and status 302, which eccejobstore stores as the user-cancel
state "killed".

This runs the real scripts/eccejobmonitor on this host with stdio comms and
a Shell job timed to end between the last poll and the alarm, and reads its
framed messages: the status it sends must be the job's own 0.

    tests/launch/jobwait_test.py [--control]

--control also runs a job that never writes its files, which must still
end with 302, so the test can tell the two apart.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
MONITOR = os.path.join(REPO, "scripts", "eccejobmonitor")

PAUSE = 4       # timePauseJobExist: polls at 0 and 4 s
LIMIT = 6       # timeLimitJobExist: the alarm at 6 s cuts the second pause
JOB_END = 5     # the job writes its files and exits in between

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
timePauseJobExist %d
timeLimitJobExist %d
""" % (PAUSE, LIMIT)

DESC = """[TE]
Begin=HEAT OF FORMATION\\s*=
Lines=1
Frequency=last
Script=none
[END]
"""


def statuses(out):
    """jmSTATUS values from the monitor's framed stdout, in order."""
    return re.findall(rb"jmSTATUS\x00?\s*(\d+)", out.replace(b"\x00", b" "))


def run(writes):
    work = tempfile.mkdtemp(prefix="ecce-jobwait-")
    try:
        with open(os.path.join(work, "eccejobmonitor.conf"), "w") as f:
            f.write(CONF)
        with open(os.path.join(work, "test.desc"), "w") as f:
            f.write(DESC)
        body = ("printf 'HEAT OF FORMATION = -1.0\\n' > job.out; "
                "echo 0 > .ecce.status; " if writes else "")
        job = subprocess.Popen(["sh", "-c", "sleep %d; %s" % (JOB_END, body)],
                               cwd=work)
        #  Reaped at once: a zombie would still answer ps -p.
        threading.Thread(target=job.wait, daemon=True).start()
        t0 = time.time()
        mon = subprocess.run(
            ["perl", MONITOR, "-configFile", "eccejobmonitor.conf",
             "-jobId", str(job.pid), "-bookmark", "0"],
            cwd=work, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, timeout=60)
        took = time.time() - t0
        logs = [n for n in os.listdir(work) if n.startswith("eccejobmonitor.log")]
        log = ""
        for n in logs:
            with open(os.path.join(work, n), errors="replace") as f:
                log += f.read()
        return statuses(mon.stdout), took, log
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--control", action="store_true")
    args = ap.parse_args()
    if not shutil.which("perl") or not shutil.which("ps"):
        print("SKIP: needs perl and ps")
        return 77

    failures = []
    got, took, log = run(True)
    waits = [l for l in log.splitlines() if "JobWait" in l or "JobCheck" in l]
    print("job ending at %ds, between the last poll and the alarm: statuses %s "
          "(%.1fs)" % (JOB_END, [s.decode() for s in got], took))
    for line in waits:
        print("  " + line)
    if not got or got[-1] != b"0":
        failures.append("finished job reported status %s, not 0"
                        % (got[-1].decode() if got else "none"))
    if b"302" in got:
        failures.append("finished job reported 302 (stored as killed)")

    if args.control:
        got, took, _ = run(False)
        print("control, a job that leaves nothing: statuses %s"
              % [s.decode() for s in got])
        if not got or got[-1] != b"302":
            failures.append("control: expected 302, got %s" % got)

    if failures:
        print("FAILED: " + "; ".join(failures))
        return 1
    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
