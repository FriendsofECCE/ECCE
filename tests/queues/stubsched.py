#!/usr/bin/env python3
"""STAND-IN batch scheduler for tests/queues: NOT PBS, LSF or Moab.

One program installed under the names of the real clients (qsub qdel qstat,
bsub bkill bjobs, msub mjobctl checkjob).  It records the submitted script,
runs it in the background the way a scheduler would (directives ignored),
prints a job id in the format the real client prints, and answers status
queries from what it recorded.  Job state lives in ../spool/<manager>/ next
to the directory holding the links, so it does not depend on the environment
ECCE hands the submit command.

Formats, from the vendors' documentation and not checked against a live
installation (none is installed here):
  PBS   qsub prints "<seq>.<server>", e.g. "12345.pbsserver"       (PBS Pro, Torque)
  LSF   bsub prints "Job <12345> is submitted to queue <normal>."  (IBM LSF; with
        no -q: "... submitted to default queue <normal>.")
  Moab  msub prints a blank line, the bare id, a newline: "\\n12345\\n"; with Torque
        underneath the id can carry a prefix, "Moab.12345" (STUB_MOAB_PREFIX=1)
"""

import os
import signal
import subprocess
import sys
import time

BIN = os.path.dirname(os.path.abspath(sys.argv[0]))
SPOOL = os.path.join(os.path.dirname(BIN), "spool")
SERVER = "stubserver"
FIRST_ID = 12345


def mgrOf(name):
    return {"qsub": "pbs", "qdel": "pbs", "qstat": "pbs",
            "bsub": "lsf", "bkill": "lsf", "bjobs": "lsf",
            "msub": "moab", "mjobctl": "moab", "checkjob": "moab"}[name]


def nextSeq(mgr):
    os.makedirs(os.path.join(SPOOL, mgr), exist_ok=True)
    path = os.path.join(SPOOL, mgr, "counter")
    fd = os.open(path, os.O_RDWR | os.O_CREAT)
    try:
        import fcntl
        fcntl.flock(fd, fcntl.LOCK_EX)
        raw = os.read(fd, 32).decode().strip()
        seq = int(raw) if raw else FIRST_ID
        os.lseek(fd, 0, 0)
        os.ftruncate(fd, 0)
        os.write(fd, str(seq + 1).encode())
    finally:
        os.close(fd)
    return seq


def jobDir(mgr, seq):
    return os.path.join(SPOOL, mgr, str(seq))


def seqOf(text):
    """The numeric part of whatever id form the client was given."""
    digits = "".join(c if c.isdigit() else " " for c in text).split()
    return int(digits[0]) if digits else None


def record(mgr, text):
    with open(os.path.join(SPOOL, mgr, "commands.log"), "a") as h:
        h.write(text + "\n")


def submit(mgr, script_text, queue):
    seq = nextSeq(mgr)
    d = jobDir(mgr, seq)
    os.makedirs(d)
    path = os.path.join(d, "script")
    with open(path, "w") as h:
        h.write(script_text)
    os.chmod(path, 0o755)
    with open(os.path.join(d, "queue"), "w") as h:
        h.write(queue or "")
    #  No scheduler runs a job in the submitter's session, and the client
    #  must return at once: detach completely and close every inherited fd.
    cmd = path
    if os.path.exists(os.path.join(SPOOL, "no-csh")):
        #  The job sees csh and tcsh as unusable files; probe records that.
        cmd = ("bwrap --dev-bind / / --ro-bind /dev/null /usr/bin/tcsh "
               "--ro-bind /dev/null /usr/bin/bsd-csh -- sh -c "
               "'csh -fc true >/dev/null 2>&1; echo $? > %s/csh-probe; exec %s'" % (d, path))
    runner = ("cd \"$HOME\"; %s > %s/stdout 2> %s/stderr < /dev/null; "
              "echo $? > %s/exit" % (cmd, d, d, d))
    proc = subprocess.Popen(["/bin/sh", "-c", runner], stdin=subprocess.DEVNULL,
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                            start_new_session=True, close_fds=True)
    with open(os.path.join(d, "pgid"), "w") as h:
        h.write(str(proc.pid))
    return seq


def alive(mgr, seq):
    d = jobDir(mgr, seq)
    if os.path.exists(os.path.join(d, "exit")):
        return False
    try:
        with open(os.path.join(d, "pgid")) as h:
            os.killpg(int(h.read()), 0)
        return True
    except (OSError, ValueError):
        return False


def state(mgr, seq):
    d = jobDir(mgr, seq)
    if not os.path.isdir(d):
        return None
    if os.path.exists(os.path.join(d, "cancelled")):
        return "X"
    return "R" if alive(mgr, seq) else "C"


def cancel(mgr, seq):
    d = jobDir(mgr, seq)
    with open(os.path.join(d, "pgid")) as h:
        pg = int(h.read())
    open(os.path.join(d, "cancelled"), "w").close()
    try:
        os.killpg(pg, signal.SIGTERM)
    except OSError:
        return
    #  PBS and LSF follow SIGTERM with SIGKILL after a grace period.
    subprocess.Popen(["/bin/sh", "-c", "sleep 2; kill -KILL -%d 2>/dev/null" % pg],
                     stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                     stderr=subprocess.DEVNULL, start_new_session=True)


def readScript(args):
    """qsub and msub take a file, bsub reads stdin."""
    files = [a for a in args if not a.startswith("-")]
    if files:
        with open(files[-1]) as h:
            return h.read()
    return sys.stdin.read()


def queueOf(text):
    for line in text.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[0] in ("#PBS", "#BSUB", "#MSUB") and parts[1] == "-q":
            return parts[2]
    return ""


def main():
    name = os.path.basename(sys.argv[0])
    args = sys.argv[1:]
    mgr = mgrOf(name)
    os.makedirs(os.path.join(SPOOL, mgr), exist_ok=True)
    record(mgr, " ".join([name] + args))

    if name in ("qsub", "bsub", "msub"):
        text = readScript(args)
        queue = queueOf(text)
        seq = submit(mgr, text, queue)
        if mgr == "pbs":
            print("%d.%s" % (seq, SERVER))
        elif mgr == "lsf":
            print("Job <%d> is submitted to %s<%s>."
                  % (seq, "queue " if queue else "default queue ", queue or "normal"))
        else:
            prefix = "Moab." if os.environ.get("STUB_MOAB_PREFIX") else ""
            print("\n%s%d\n" % (prefix, seq))
        return 0

    if name in ("qdel", "bkill", "mjobctl"):
        ids = [a for a in args if not a.startswith("-")]
        if name == "mjobctl" and "-c" not in args:
            print("ERROR:    only -c is supported by the stand-in", file=sys.stderr)
            return 1
        seq = seqOf(ids[-1]) if ids else None
        if seq is None or state(mgr, seq) is None:
            msgs = {"pbs": ("qdel: Unknown Job Id %s" % (ids[-1] if ids else ""), 170),
                    "lsf": ("Job <%s>: No matching job found" % (ids[-1] if ids else ""), 255),
                    "moab": ("ERROR:    cannot locate job '%s'" % (ids[-1] if ids else ""), 1)}
            text, rc = msgs[mgr]
            print(text, file=sys.stderr)
            return rc
        if state(mgr, seq) == "R":
            cancel(mgr, seq)
        if mgr == "lsf":
            print("Job <%d> is being terminated" % seq)
        elif mgr == "moab":
            print("job '%d' cancelled" % seq)
        return 0

    if name in ("qstat", "bjobs", "checkjob"):
        ids = [a for a in args if not a.startswith("-")]
        seq = seqOf(ids[-1]) if ids else None
        st = state(mgr, seq) if seq is not None else None
        if not ids:
            return 0
        if st is None or (st in ("C", "X") and mgr != "moab"):
            #  Finished jobs age out of the real clients' output at once here.
            msgs = {"pbs": "qstat: Unknown Job Id %s" % ids[-1],
                    "lsf": "Job <%s> is not found" % ids[-1],
                    "moab": "ERROR:  cannot locate job '%s'" % ids[-1]}
            print(msgs[mgr], file=sys.stderr)
            return {"pbs": 153, "lsf": 255, "moab": 1}[mgr]
        if mgr == "pbs":
            queue = open(os.path.join(jobDir(mgr, seq), "queue")).read() or "normal"
            if "-f" in args:
                #  qstat -f, the form eccejobmonitor reads: "    job_state = R".
                print("Job Id: %s" % ids[-1])
                print("    Job_Name = submit")
                print("    Job_Owner = %s@%s" % (os.environ.get("USER", "user"), SERVER))
                print("    job_state = %s" % st)
                print("    queue = %s" % queue)
            else:
                print("Job id                    Name             User             Time Use S Queue")
                print("------------------------- ---------------- ---------------- -------- - -----")
                print("%-25s %-16s %-16s %8s %s %s" % (
                    ids[-1], "submit", os.environ.get("USER", "user"), "00:00:00", st, queue))
        elif mgr == "lsf":
            #  bjobs -w, the form eccejobmonitor reads: "... user  RUN  queue  from  exec ...".
            print("JOBID   USER    STAT  QUEUE      FROM_HOST   EXEC_HOST   JOB_NAME   SUBMIT_TIME")
            print("%-7d %-7s RUN   %-10s %-11s %-11s %-10s Sep 30 12:00" % (
                seq, os.environ.get("USER", "user"), "normal", SERVER, SERVER, "submit"))
        else:
            print("State: %s" % {"R": "Running", "C": "Completed", "X": "Removed"}[st])
        return 0

    return 2


if __name__ == "__main__":
    sys.exit(main())
