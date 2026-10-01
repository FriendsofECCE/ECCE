#!/usr/bin/env python3
"""A file that appears during FileWait's last pause must be seen.

eccejobmonitor's FileWait() and FileWait2() poll for a file inside an alarm.
When the alarm cut the last pause short they gave up without one more look,
so a file written just before the alarm counted as missing: FileWait made
the monitor send status 302 for a finished job (stored as the user-cancel
state "killed"), FileWait2 made it die while the job was running.

Runs the real scripts/eccejobmonitor with stdio comms.  A helper waits for
the monitor to log that it started waiting, then writes the file five
seconds later: after the poll at 4 s and before the alarm at 6 s.

    tests/launch/filewait_test.py
"""

import os
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import jobwait_test as jw  # noqa: E402

CONF = jw.CONF + """timePauseFileExist 4
timeLimitFileExist 6
timePauseFileExistStat 4
timeLimitFileExistStat 6
"""

OUT = "printf 'HEAT OF FORMATION = -1.0\\n' > job.out"


def waitFor(marker):
    return ("until grep -q '%s' eccejobmonitor.log* 2>/dev/null; do "
            "sleep 0.2; done; sleep 5" % marker)


#  FileWait: the job is gone and has written its output; its status file
#  turns up late, as it can on a shared file system.
STATUS_LATE = "%s; (%s; echo 0 > .ecce.status) >/dev/null 2>&1 &" % (
    OUT, waitFor("FileWait: waiting"))

#  FileWait2: the queue says the job runs, and its output file turns up
#  late.  Shell would report "pending" until the file exists, so the queue
#  is a Slurm whose sacct is a stand-in.
SACCT = ("#!/bin/sh\nif [ -e .ecce.status ]; then echo COMPLETED; "
         "else echo RUNNING; fi\n")
OUTPUT_LATE = "%s; %s; echo 0 > .ecce.status; sleep 1" % (
    waitFor("FileWait2: waiting"), OUT)


def run(script, slurm=False):
    work = tempfile.mkdtemp(prefix="ecce-filewait-")
    try:
        with open(os.path.join(work, "eccejobmonitor.conf"), "w") as f:
            f.write(CONF.replace("jobQ Shell", "jobQ slurm") if slurm else CONF)
        env = dict(os.environ)
        if slurm:
            os.mkdir(os.path.join(work, "bin"))
            path = os.path.join(work, "bin", "sacct")
            with open(path, "w") as f:
                f.write(SACCT)
            os.chmod(path, 0o755)
            env["PATH"] = os.path.join(work, "bin") + ":" + env["PATH"]
        with open(os.path.join(work, "test.desc"), "w") as f:
            f.write(jw.DESC)
        job = subprocess.Popen(["sh", "-c", script], cwd=work)
        import threading
        threading.Thread(target=job.wait, daemon=True).start()
        t0 = time.time()
        mon = subprocess.run(
            ["perl", jw.MONITOR, "-configFile", "eccejobmonitor.conf",
             "-jobId", str(job.pid), "-bookmark", "0"],
            cwd=work, env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, timeout=40)
        took = time.time() - t0
        log = ""
        for n in os.listdir(work):
            if n.startswith("eccejobmonitor.log"):
                with open(os.path.join(work, n), errors="replace") as f:
                    log += f.read()
        return jw.statuses(mon.stdout), took, log
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    if not shutil.which("perl") or not shutil.which("ps"):
        print("SKIP: needs perl and ps")
        return 77
    failures = []
    for label, script, slurm in (
            ("FileWait, status file late", STATUS_LATE, False),
            ("FileWait2, output file late", OUTPUT_LATE, True)):
        try:
            got, took, log = run(script, slurm)
        except subprocess.TimeoutExpired:
            failures.append("%s: the monitor did not finish" % label)
            continue
        print("%s: statuses %s (%.1fs)" % (label, [s.decode() for s in got], took))
        for line in log.splitlines():
            if "FileWait" in line:
                print("  " + line)
        if not got or got[-1] != b"0":
            failures.append("%s: last status %s, not 0"
                            % (label, got[-1].decode() if got else "none"))
        if b"302" in got:
            failures.append("%s: reported 302 (stored as killed)" % label)
    if failures:
        print("FAILED: " + "; ".join(failures))
        return 1
    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
