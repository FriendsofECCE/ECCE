#!/usr/bin/env python3
"""A cancelled job leaves no program running in its run directory (#855).

Slurm with proctrack/linuxproc finds a job's processes by walking the tree
from the job shell: TERM to all of them, then KILL to what is still in the
tree after KillWait.  A program that outlives the TERM while its job shell
dies is reparented to init and the KILL misses it.  No services, no ECCE
state: a temp tree.

  a. the gensub job script (MOPAC stand-in that ignores TERM) under that
     emulated cancel: the program must be gone afterwards
  b. the same with a program that obeys TERM: the script writes 302
  c. scripts/ecce-rundir-sweep ends an escaped TERM-ignoring process in the
     run directory, spares the job monitor and its child and a program on a
     terminal there, says "none" when
     nothing is left, and "remote NODE" for a Slurm job that ran elsewhere

    tests/launch/terminate_leftover_test.py
"""

import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "queues"))
import run_tests as q   # noqa: E402

SWEEP = os.path.join(REPO, "scripts", "ecce-rundir-sweep")
failures = []
started = []


def check(ok, what):
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)


def alive(pid):
    try:
        with open("/proc/%d/stat" % pid) as h:
            return h.read().split(")")[-1].split()[0] != "Z"
    except OSError:
        return False


def children():
    tree = {}
    for p in os.listdir("/proc"):
        if not p.isdigit():
            continue
        try:
            with open("/proc/%s/status" % p) as h:
                for line in h:
                    if line.startswith("PPid:"):
                        tree.setdefault(int(line.split()[1]), []).append(int(p))
        except OSError:
            pass
    return tree


def descendants(pid):
    tree, out, todo = children(), [], [pid]
    while todo:
        p = todo.pop()
        out.append(p)
        todo += tree.get(p, [])
    return [p for p in out if alive(p)]


def waitFile(path, seconds=20):
    end = time.time() + seconds
    while time.time() < end:
        if os.path.exists(path) and os.path.getsize(path) > 0:
            return True
        time.sleep(0.1)
    return False


def linuxprocCancel(g, tmp, ignoreTerm):
    """Runs the job script and cancels it as slurmd with linuxproc does.
    Returns (program pid alive afterwards, status file text)."""
    run = tempfile.mkdtemp(prefix="run-", dir=tmp)
    prog = os.path.join(tmp, "mopac-%s" % ("deaf" if ignoreTerm else "obeys"))
    with open(prog, "w") as h:
        h.write("#!/bin/sh\n%secho $$ > %s/prog.pid\nexec sleep 120\n"
                % ("trap '' TERM\n" if ignoreTerm else "", run))
    os.chmod(prog, 0o755)
    g.config("MOPAC: %s\n" % prog)
    script = g.script("slurm", "mopac", q.PROFILES["basic"], rundir=run)
    path = os.path.join(tmp, "job-%s.sh" % os.path.basename(run))
    with open(path, "w") as h:
        h.write(script)
    open(os.path.join(run, "mopac.mop"), "w").write("PM7\n\n\nH 0 0 0\n")
    job = subprocess.Popen(["sh", path], cwd=run, start_new_session=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    started.append(job.pid)
    if not waitFile(os.path.join(run, "prog.pid")):
        return None, "program did not start"
    pid = int(open(os.path.join(run, "prog.pid")).read())
    started.append(pid)
    for p in descendants(job.pid):
        os.kill(p, signal.SIGTERM)
    time.sleep(1.5)   # KillWait
    for p in descendants(job.pid):
        os.kill(p, signal.SIGKILL)
    time.sleep(0.5)
    job.wait(timeout=10)
    status = os.path.join(run, ".ecce.status")
    text = open(status).read().strip() if os.path.exists(status) else ""
    return alive(pid), text


def spawn(cmd, cwd, **kw):
    p = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL, **kw)
    started.append(p.pid)
    return p


def sweep(run, mgr="Shell", env=None, grace="2"):
    res = subprocess.run(["sh", SWEEP, run, mgr, "4711", grace], env=env,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         timeout=60)
    return res.stdout.decode().strip()


def sweepCases(tmp):
    run = tempfile.mkdtemp(prefix="sweep-", dir=tmp)
    sub = os.path.join(run, "MRMD")
    os.makedirs(sub)
    # Escaped (own session, like a program reparented to init), deaf to TERM.
    deaf = spawn(["sh", "-c", "trap '' TERM; exec sleep 120"], sub,
                 start_new_session=True)
    obeys = spawn(["sleep", "120"], run, start_new_session=True)
    mon = os.path.join(tmp, "eccejobmonitor")
    with open(mon, "w") as h:
        h.write("sleep 120 &\nwait\n")
    monitor = spawn(["sh", mon], run, start_new_session=True)
    # A program on a terminal there (a user's shell, a pager) is not the job.
    term = spawn(["script", "-qfc", "sleep 120", "/dev/null"], run,
                 start_new_session=True, stdin=subprocess.DEVNULL) \
        if shutil.which("script") else None
    time.sleep(0.5)
    monChild = [p for p in descendants(monitor.pid) if p != monitor.pid]
    t0 = time.time()
    out = sweep(run)
    took = time.time() - t0
    check(out.startswith("killed ") and str(deaf.pid) in out
          and str(obeys.pid) in out, "sweep: names what it ended (%r)" % out)
    deaf.poll(), obeys.poll()
    time.sleep(0.2)
    check(not alive(deaf.pid) and not alive(obeys.pid),
          "sweep: TERM-deaf and plain programs are gone")
    check(took >= 2, "sweep: KILL only after the grace (%.1f s)" % took)
    check(alive(monitor.pid) and monChild and all(alive(p) for p in monChild),
          "sweep: the job monitor and its child are spared")
    check(sweep(run) == "none", "sweep: 'none' when nothing is left")

    bin_ = os.path.join(tmp, "bin")
    os.makedirs(bin_, exist_ok=True)
    with open(os.path.join(bin_, "sacct"), "w") as h:
        h.write("#!/bin/sh\necho node-elsewhere-17\n")
    os.chmod(os.path.join(bin_, "sacct"), 0o755)
    env = dict(os.environ, PATH=bin_ + os.pathsep + os.environ["PATH"])
    check(sweep(run, "Slurm", env) == "remote node-elsewhere-17",
          "sweep: a Slurm job on another node is reported as not visible")
    check(sweep(os.path.join(tmp, "nope")).startswith("unchecked "),
          "sweep: a missing run directory is 'unchecked'")
    if term:
        onTty = [p for p in descendants(term.pid) if p != term.pid]
        check(onTty and all(alive(p) for p in onTty),
              "sweep: a program on a terminal in the run directory is spared")
        term.kill()
        for p in onTty:
            started.append(p)
    monitor.kill()


def main():
    if not sys.platform.startswith("linux"):
        print("SKIP: needs /proc")
        return 77
    g = q.Gensub()
    tmp = tempfile.mkdtemp(prefix="ecce-855-")
    try:
        survived, status = linuxprocCancel(g, tmp, True)
        check(survived is False,
              "job script: a TERM-deaf program is in the tree for the KILL (%s)"
              % ("still running" if survived else status or "gone"))
        survived, status = linuxprocCancel(g, tmp, False)
        check(survived is False and status == "302",
              "job script: a cancelled run ends with status 302 (%r)" % status)
        sweepCases(tmp)
    finally:
        for p in started:
            try:
                os.kill(p, signal.SIGKILL)
            except OSError:
                pass
        g.close()
        subprocess.run(["rm", "-rf", tmp])
    print("FAILED: %s" % "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
