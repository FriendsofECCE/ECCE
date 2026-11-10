#!/usr/bin/env python3
"""ecce-launch --move (#830): a Submitted calculation whose Slurm job is
still pending moves to another machine under its own name.

Two Slurm machines (m1, m2) are stand-ins: sbatch records and prints an id,
squeue lists the jobs with the state kept in a file, scancel records the
cancel, sacct reports PENDING (the scripts never run). A calculation with a
PENDING job on m1 is moved to m2: the fake m1 records the cancel, the
calculation is Submitted on m2 with a new job id, keeps its name and its run
log gains "moved from m1 to m2, job N cancelled". One whose job is RUNNING
is refused and left as it was. --move without on= is a usage error.

    tests/launch/ecce_launch_move_test.py [--build DIR] [--keep]

Exit status 77 (CTest SKIP) without Xvfb or the built binaries.
"""

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402

failures = []

SBATCH = """#!/bin/sh
# STAND-IN for Slurm's sbatch: records the script, never runs it.
spool=%(spool)s
n=$(cat $spool/counter 2>/dev/null || echo %(first)d)
echo $((n + 1)) > $spool/counter
cp "$1" $spool/$n.script 2>/dev/null
echo PD > $spool/$n.st
echo "Submitted batch job $n"
"""
SQUEUE = """#!/bin/sh
# STAND-IN: plain squeue lists the jobs; any option answers nothing.
spool=%(spool)s
[ $# -gt 0 ] && exit 0
echo "  JOBID PARTITION     NAME     USER ST       TIME  NODES NODELIST(REASON)"
for f in $spool/*.st; do
  [ -f "$f" ] || continue
  id=$(basename $f .st)
  printf "  %%s    normal   submit  nobody %%s       0:00      1 (Resources)\\n" $id $(cat $f)
done
"""
SCANCEL = """#!/bin/sh
# STAND-IN: records the cancel and drops the job from the queue.
spool=%(spool)s
echo "scancel $*" >> $spool/cancel.log
for id in "$@"; do mv $spool/$id.st $spool/$id.cancelled 2>/dev/null; done
exit 0
"""
SACCT = """#!/bin/sh
# STAND-IN: a queued job is PENDING, a cancelled one CANCELLED.
spool=%(spool)s
case "$*" in *End) echo ''; exit 0;; esac
for a in "$@"; do case "$a" in [0-9]*) id=$a;; esac; done
if [ -f $spool/$id.cancelled ]; then echo "CANCELLED by 1000"; else echo PENDING; fi
"""


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def fakeSlurm(root, first):
    spool = os.path.join(root, "spool")
    bindir = os.path.join(root, "bin")
    os.makedirs(spool)
    os.makedirs(bindir)
    for name, body in (("sbatch", SBATCH), ("squeue", SQUEUE),
                       ("scancel", SCANCEL), ("sacct", SACCT)):
        path = os.path.join(bindir, name)
        with open(path, "w") as h:
            h.write(body % {"spool": spool, "first": first})
        os.chmod(path, 0o755)
    return bindir, spool


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    exe = os.path.join(build, "ecce-launch")
    if not os.access(exe, os.X_OK):
        say("FAIL: ecce-launch is not built in %s" % build)
        return 1
    import xdisplay

    s = harness.Session(build, "ecce-launch-move", {"CP2K": "/bin/true"},
                        local=True, keep=args.keep)
    harness.link(os.path.join(build, "organizer"),
                 os.path.join(s.home, "bin", "organizer"))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    bin1, spool1 = fakeSlurm(os.path.join(s.state, "slurm-m1"), 1000)
    bin2, spool2 = fakeSlurm(os.path.join(s.state, "slurm-m2"), 2000)
    s.registerMachine("m1", "Slurm", {"CP2K": "/bin/true"}, config={"qmgrPath": bin1})
    s.registerMachine("m2", "Slurm", {"CP2K": "/bin/true"}, config={"qmgrPath": bin2})
    with open(os.path.join(s.state, ".ECCE", "MachPrefs"), "w") as h:
        for m in ("m1", "m2"):
            rd = os.path.join(s.state, "run-" + m)
            os.makedirs(rd, exist_ok=True)
            h.write("%s:\t%s|ssh|normal|%s|%s||1|1|60|0|0|1\n" % (m, s.user(), rd, rd))
    work = os.path.join(s.state, "decks")
    os.makedirs(work, exist_ok=True)
    deck = os.path.join(work, "a.cp2kin")
    open(deck, "w").write("&GLOBAL\n  RUN_TYPE ENERGY\n&END GLOBAL\n")
    rundir = os.path.join(s.state, "run-m1")

    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        urls, ids = {}, {}
        for name in ("pend", "started"):
            url, out = s.create(name, "cp2k_es", deck, "cp2k.cp2kin", rundir,
                                machine="m1",
                                extra=("procs=1", "queue=normal", "nodes=1", "wall=0 1:0"))
            if not check(url is not None, "calculation %s created" % name):
                say(out)
                return 1
            urls[name] = url
            rc, out = s.launch(url)
            ids[name] = s.driver("meta", url, "job_jobid")[1].strip()
            check(rc == 0 and ids[name].isdigit() and s.state_of(url) == "submitted",
                  "%s is Submitted on m1 as job %s" % (name, ids[name]))
        #  The stand-in queue now shows "started" as running.
        with open(os.path.join(spool1, ids["started"] + ".st"), "w") as h:
            h.write("R\n")

        def run(*argv):
            r = subprocess.run([exe] + list(argv), env=s.env(),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=300)
            return r.returncode, r.stdout.decode("utf-8", "replace"), \
                r.stderr.decode("utf-8", "replace")

        rc, out, err = run("--move", urls["pend"])
        check(rc == 64 and "--move needs on=MACHINE" in err,
              "--move without on=: usage, exit 64 (rc=%s)" % rc)

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")], env=genv,
            cwd=os.path.join(s.home, "bin"),
            stdout=open(os.path.join(s.state, "organizer.log"), "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        #  A RUNNING job is refused and nothing changes.
        end = time.time() + 120
        while time.time() < end:
            rc, out, err = run("--move", "on=m2", urls["started"])
            if rc != 2:
                break
            time.sleep(3)
        say(out)
        check(rc == 1 and re.search(r"^started: not moved: .*has started", out, re.M),
              "a running job is refused (rc=%s)" % rc)
        check(s.state_of(urls["started"]) == "submitted" and
              s.driver("meta", urls["started"], "job_jobid")[1].strip() == ids["started"],
              "the running calculation is unchanged")
        check(not os.path.exists(os.path.join(spool1, "cancel.log")),
              "nothing was cancelled for the running job")

        #  A pending job moves.
        rc, out, err = run("--move", "on=m2", urls["pend"])
        say(out)
        check(rc == 0 and re.search(r"^pend: job id 2\d+$", out, re.M),
              "pending job: 'pend: job id N' from m2 (rc=%s)" % rc)
        cancels = ""
        try:
            cancels = open(os.path.join(spool1, "cancel.log")).read()
        except OSError:
            pass
        check(cancels.split() == ["scancel", ids["pend"]],
              "m1 recorded the cancel of job %s (%r)" % (ids["pend"], cancels))
        newid = s.driver("meta", urls["pend"], "job_jobid")[1].strip()
        check(newid.isdigit() and newid != ids["pend"] and int(newid) >= 2000,
              "new job id %s on m2, not %s" % (newid, ids["pend"]))
        check(s.state_of(urls["pend"]) == "submitted", "pend is Submitted again")
        rc, ld = s.driver("launchdata", urls["pend"])
        stored = dict(l.split("=", 1) for l in ld.splitlines() if "=" in l)
        check(stored.get("machine") == "m2", "pend is stored on m2 (%s)" % stored.get("machine"))
        check(urls["pend"].endswith("/pend") and
              os.path.isdir(urls["pend"][len("file://"):]), "same name and folder")
        #  The new job's monitor moves the old log to ".last"; the line is in one of them.
        log = s.run_log(urls["pend"])
        for extra in glob.glob(os.path.join(urls["pend"][len("file://"):], "Outputs",
                                            "eccejobstorelog.ecce_run_log*")):
            log += open(extra, errors="replace").read()
        check("moved from m1 to m2, job %s cancelled" % ids["pend"] in log,
              "the run log has the move line")
    finally:
        if org and org.poll() is None:
            org.terminate()
        if display:
            display.__exit__(None, None, None)
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)

    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
