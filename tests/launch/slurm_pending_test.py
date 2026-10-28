#!/usr/bin/env python3
"""A pending Slurm job (#641): the real eccejobmonitor, with stand-in sacct
and squeue on the PATH, polls a job that is PENDING for several rounds and
then RUNNING.  Checks that the monitor

  - keeps waiting while the job is pending (never dies, no status sent),
  - sends the scheduler's reason and estimated start (UTC) once per change,
    not once per poll,
  - sends the running status afterwards.

    tests/launch/slurm_pending_test.py
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

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

DESC = "[TE]\nBegin=HEAT\nLines=1\nFrequency=last\nScript=none\n[END]\n"

# Polls 1-3 pending for Resources, 4-5 for Priority, then running.
SACCT = """#!/bin/sh
n=$(cat count 2>/dev/null || echo 0)
n=$((n + 1))
echo $n > count
if [ $n -le 5 ]; then echo PENDING; else echo RUNNING; echo "HEAT OF FORMATION = 1" > job.out; fi
"""
SQUEUE = """#!/bin/sh
n=$(cat count 2>/dev/null || echo 0)
if [ $n -le 3 ]; then echo "Resources|2026-10-30T12:20:00"; else echo "Priority|N/A"; fi
"""


def main():
    if not shutil.which("perl"):
        print("SKIP: needs perl")
        return 77
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    cxx = shutil.which("g++") or shutil.which("c++")
    if cxx:
        tmp = tempfile.mkdtemp(prefix="ecce-queuewait-")
        try:
            exe = os.path.join(tmp, "testQueueWait")
            built = subprocess.run(
                [cxx, "-std=c++17", "-o", exe,
                 os.path.join(HERE, "testQueueWait.C")], capture_output=True)
            ran = built.returncode == 0 and subprocess.run([exe]).returncode == 0
            check(ran, "QueueWait.H unit test (testQueueWait.C)")
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    work = tempfile.mkdtemp(prefix="ecce-slurmpend-")
    try:
        bin_ = os.path.join(work, "bin")
        os.makedirs(bin_)
        for name, body in (("sacct", SACCT), ("squeue", SQUEUE)):
            path = os.path.join(bin_, name)
            with open(path, "w") as f:
                f.write(body)
            os.chmod(path, 0o755)
        for name, text in (("eccejobmonitor.conf", CONF), ("test.desc", DESC)):
            with open(os.path.join(work, name), "w") as f:
                f.write(text)
        env = dict(os.environ, PATH=bin_ + os.pathsep + os.environ["PATH"])
        mon = subprocess.Popen(
            ["perl", MONITOR, "-configFile", "eccejobmonitor.conf",
             "-jobId", "48236722", "-bookmark", "0"],
            cwd=work, env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)
        text = b""
        deadline = time.time() + 60
        os.set_blocking(mon.stdout.fileno(), False)
        while time.time() < deadline and b"jmSTATUS" not in text:
            time.sleep(0.5)
            try:
                chunk = mon.stdout.read()
            except BlockingIOError:
                chunk = None
            if chunk:
                text += chunk
            if mon.poll() is not None:
                break
        died = mon.poll() is not None and b"jmSTATUS" not in text
        mon.kill()
        mon.wait()
        text = text.replace(b"\x00", b" ").decode(errors="replace")

        queue = re.findall(r"queue ([^\s\x01]+\|[^\s\x01]+)", text)
        check(not died, "the monitor kept waiting while pending")
        check(queue == ["Resources|2026-10-30T12:20:00Z", "Priority|N/A"],
              "reason and start sent once per change: %s" % queue)
        check(re.search(r"jmSTATUS 301\b", text) is not None,
              "running status sent after the pending rounds")
        first = text.find("jmSTATUS")
        check(first < 0 or text.rfind("queue ", 0, first) >= 0,
              "the reason came before the running status")
    finally:
        shutil.rmtree(work, ignore_errors=True)

    if failures:
        print("FAILED: " + "; ".join(failures))
        return 1
    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
