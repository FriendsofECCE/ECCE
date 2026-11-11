#!/usr/bin/env python3
"""The real eccejobmonitor follows a Follow=yes File= entry while the job
runs (#949).

A made-up job (a sleep process, queue "shell") and a descriptor with one
Follow=yes entry and one ordinary Lines=1 entry.  The followed file grows
between polls, once with half a line.  Checked, live:
  - each follow block starts "#ECCE-FOLLOW <file> <offset>" with the offset
    of its first byte, and the blocks together are the file, every byte once;
  - half a line waits for its newline;
  - a block arrives while the job still runs, not only at the end;
  - the followed file is not sent again as a jmFILE at the end, while an
    ordinary File= entry still is.
In post mode (an import) the followed file sends nothing.

    tests/parsers/monitor_follow_test.py
"""

import os
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
MONITOR = os.path.join(ROOT, "scripts", "eccejobmonitor")
failures = []

DESC = """[LINE]
Script=x.line
Begin=^point
Lines=1
[END]

[FOLLOWED]
Script=x.follow
File=grow.xyz
Follow=yes
Begin=grow.xyz
[END]

[ATEND]
Script=x.end
File=end.dat
Begin=end.dat
[END]
"""


def check(ok, what):
    print("%s %s" % ("ok   " if ok else "FAIL ", what))
    if not ok:
        failures.append(what)


def messages(path):
    raw = open(path, "rb").read().decode("utf-8", "replace")
    msgs, cur, i = [], "", 0
    while True:
        i = raw.find("\001", i)
        if i < 0:
            break
        seq, length = raw[i + 1], int(raw[i + 2:i + 6])
        cur += raw[i + 6:i + 6 + length]
        i += 6 + length
        if seq in "03":
            msgs.append(cur.replace("\002", "\n").split("\000"))
            cur = ""
    return msgs


def follow_blocks(msgs):
    return [m for m in msgs if m[0] == "jmPROP" and len(m) > 4 and m[2] == "FOLLOWED"]


def run(work, mode, job=None, steps=()):
    out = os.path.join(work, "out-%s" % mode)
    cmd = ["perl", MONITOR, "-commType", "stdio", "-monitoringMode", mode,
           "-jobOutputFile", "job.out", "-parseDescriptorFile", "t.desc",
           "-parseTypes", "ALL", "-outputFile", out, "-calcName", "p",
           "-logFile", os.path.join(work, "m-%s.log" % mode),
           "-filesToTransfer", "", "-importDir", work + os.sep,
           "-timePauseReadLine", "1", "-timePauseJobExist", "1",
           "-timePauseFileExist", "1", "-timePauseFileExistStat", "1"]
    if job:
        cmd += ["-jobQ", "shell", "-jobId", str(job.shell_pid)]
    # A file, not a pipe: nothing reads the pipe while the monitor runs.
    log = open(os.path.join(work, "monitor-%s.log" % mode), "w+b")
    proc = subprocess.Popen(cmd, cwd=work, stdin=subprocess.DEVNULL,
                            stdout=log, stderr=subprocess.STDOUT)
    sizes = []
    for step in steps:
        time.sleep(2.5)
        step()
        sizes.append(os.path.getsize(out) if os.path.exists(out) else 0)
    if job:
        time.sleep(2.5)
        running = messages(out) if os.path.exists(out) else []
        subprocess.run(["kill", str(job.shell_pid)])
        job.wait()
        open(os.path.join(work, ".ecce.status"), "w").write("0\n")
    else:
        running = None
    try:
        proc.wait(timeout=90)
    except subprocess.TimeoutExpired:
        proc.kill()
        check(False, "%s monitor ended" % mode)
    ok = proc.returncode == 0
    check(ok, "%s monitor exit status %s" % (mode, proc.returncode))
    if not ok:
        # The follow checks below only fail as a consequence; the cause is here.
        log.seek(0)
        sys.stdout.write(log.read().decode("utf-8", "replace"))
        mlog = os.path.join(work, "m-%s.log" % mode)
        if os.path.exists(mlog):
            sys.stdout.write(open(mlog, encoding="utf-8", errors="replace").read()[-4000:])
    log.close()
    return messages(out), running


def main():
    work = tempfile.mkdtemp(prefix="ecce-follow-")
    try:
        # newline="\n": the follow checks compare bytes, and Windows text
        # mode would write CRLF.
        def write(path, text, mode="w"):
            with open(path, mode, newline="\n") as handle:
                handle.write(text)

        write(os.path.join(work, "t.desc"), DESC)
        write(os.path.join(work, "end.dat"), "x\n")
        grow = os.path.join(work, "grow.xyz")
        out = os.path.join(work, "job.out")
        write(out, "point 1\n")
        write(grow, "3\nframe 1\n")

        def append(path, text):
            return lambda: write(path, text, "a")

        steps = [append(grow, "C 0 0 0\nN 1 0 0\nH 2 0"),   # half a line
                 append(grow, " 0\n3\nframe 2\nC 0 0 1\n"),
                 append(out, "point 2\n"),
                 append(grow, "N 1 0 1\nH 2 0 1\n")]
        # The monitor checks the job by the process id of the shell it runs
        # under (MSYS on Windows, where Python's pid is a Windows one).
        job = subprocess.Popen(["sh", "-c", "echo $$; exec sleep 120"],
                               stdout=subprocess.PIPE, text=True)
        job.shell_pid = int(job.stdout.readline())
        msgs, running = run(work, "live", job, steps)
        blocks = follow_blocks(msgs)
        whole = open(grow, newline="").read()
        got, pos, ok = "", 0, True
        for b in blocks:
            body = b[4]
            head, _, rest = body.partition("\n")
            if head != "#ECCE-FOLLOW grow.xyz %d" % pos:
                ok = False
                print("     block header %r at byte %d" % (head, pos))
            got += rest
            pos += len(rest)
        check(ok, "follow blocks carry the offset of their first byte")
        check(got == whole, "the follow blocks are the file, every byte once (%d blocks)"
              % len(blocks))
        check(len(blocks) >= 3, "the file is sent as it grows (%d blocks)" % len(blocks))
        check(all(b[4].endswith("\n") for b in blocks), "no block ends in half a line")
        check(len(follow_blocks(running)) >= 2, "blocks arrive while the job runs")
        files = [m[4] for m in msgs if m[0] == "jmFILE"]
        check("grow.xyz" not in files, "the followed file is not sent again at the end")
        check("end.dat" in files, "an ordinary File= entry is still sent at the end")
        check(any(m[0] == "jmPROP" and len(m) > 3 and m[2] == "LINE" for m in msgs),
              "ordinary entries still parsed")

        msgs, _ = run(work, "post")
        check(not follow_blocks(msgs), "post mode: no follow blocks")
        check("grow.xyz" not in [m[4] for m in msgs if m[0] == "jmFILE"],
              "post mode: the followed file is not sent as a file either")
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("ok" if not failures else "%d failures" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
