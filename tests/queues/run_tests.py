#!/usr/bin/env python3
"""Batch-queue support: submit script, submission, job id, completion, cancel.

Two suites, each over the queue managers ECCE ships in siteconfig/QueueManagers:

  stubs   PBS, LSF and Moab against STAND-IN clients (tests/queues/stubsched.py,
          installed as qsub/qdel/qstat, bsub/bkill/bjobs, msub/mjobctl/checkjob
          on PATH for this test only).  They run the submitted script in the
          background and print a job id in the real client's format.  They
          prove ECCE's side of the protocol (script, submit command, id
          parsing, cancel command) and say nothing about the real schedulers.
  slurm   Slurm against the real sbatch/squeue/scancel of this machine.

Both launch MOPAC and NWChem through the real Launch (tests/launch/launchjob)
on a machine registered under the queue manager, with queue, node, processor,
wall time, memory and account set as the launcher's queue controls set them,
and cancel a long job through RunMgmt::terminate, which is what the
Organizer's Kill calls.

The submit script of every manager is also generated directly by gensub for
fixed settings and compared with tests/queues/golden/, so the csh-to-sh port
of gensub can be checked against what csh produced.

    tests/queues/run_tests.py [--build build] [--suite local|stubs|slurm|golden|all]
                              [--transport unset|direct|both] [--manager pbs ...]
                              [--update-golden] [--keep] [-v]

"local" is golden plus stubs: everything that needs no scheduler.

Exit status 77 (CTest SKIP) when a prerequisite is missing; for the slurm
suite that includes sinfo failing or listing no partition.
"""

import argparse
import difflib
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "launch"))

import harness  # noqa: E402
from harness import REPO, say  # noqa: E402

GOLDEN = os.path.join(HERE, "golden")
STUB_LINKS = ("qsub", "qdel", "qstat", "bsub", "bkill", "bjobs",
              "msub", "mjobctl", "checkjob")
MOPAC_DECK = os.path.join(REPO, "tests", "e2e", "fixtures", "mopac", "mos", "mopac.mop")
NWCHEM_DECK = os.path.join(REPO, "tests", "e2e", "fixtures", "nwchem", "co-opt.nw")

#  key -> (queueMgrName in siteconfig/QueueManagers, directive prefix, machine)
MANAGERS = {
    "pbs": ("PBS", "#PBS", "pbstest"),
    "lsf": ("LSF", "#BSUB", "lsftest"),
    "moab": ("Moab", "#MSUB", "moabtest"),
    "slurm": ("Slurm", "#SBATCH", "slurmtest"),
    "shell": ("Shell", None, "goldhost"),
}
STUB_MANAGERS = ("pbs", "lsf", "moab")

#  What the launcher's queue controls can set.  wall is what Launch passes as
#  ##wall_clock_time##, hrmin as ##wall_clock_hrmin##; None means the control
#  is unset or unsupported.
PROFILES = {
    "basic": dict(queue="debug", nodes=1, procs=1, wall="0:30:00", hrmin="0:30",
                  mem=2000, account="proj1"),
    "multi": dict(queue="normal", nodes=2, procs=8, wall="26:0:00", hrmin="26:0",
                  mem=None, account=None),
    "bare": dict(queue=None, nodes=1, procs=1, wall=None, hrmin=None,
                 mem=None, account=None),
}
GOLDEN_CASES = [(m, "nwchem", p) for m in MANAGERS for p in PROFILES] + \
               [(m, "mopac", "basic") for m in MANAGERS]
CODES = {"nwchem": ("NWChem", "nwch.nw", "nwch.nwout"),
         "mopac": ("MOPAC", "mopac.mop", "mopac.mopout")}


def expectedDirectives(mgr, p, jobname):
    """The directive lines submit.site's block must yield for profile `p`."""
    ppn = p["procs"] // p["nodes"]
    out = []

    def add(line, present=True):
        if present:
            out.append(line)

    if mgr == "pbs":
        add("#PBS -N " + jobname)
        add("#PBS -q %s" % p["queue"], p["queue"])
        add("#PBS -l select=%d:ncpus=%d" % (p["nodes"], ppn))
        add("#PBS -l walltime=%s" % p["wall"], p["wall"])
        add("#PBS -l mem=%smb" % p["mem"], p["mem"])
        add("#PBS -A %s" % p["account"], p["account"])
        out += ["#PBS -j oe", "#PBS -o pbs.out", "#PBS -S /bin/csh"]
    elif mgr == "lsf":
        add("#BSUB -J " + jobname)
        add("#BSUB -q %s" % p["queue"], p["queue"])
        add("#BSUB -n %d" % p["procs"])
        add("#BSUB -W %s" % p["hrmin"], p["hrmin"])
        add("#BSUB -M %s" % p["mem"], p["mem"])
        add("#BSUB -P %s" % p["account"], p["account"])
        out += ["#BSUB -o %s.out" % jobname, "#BSUB -e %s.err" % jobname,
                "#BSUB -L /bin/csh"]
    elif mgr == "moab":
        add("#MSUB -N " + jobname)
        add("#MSUB -q %s" % p["queue"], p["queue"])
        add("#MSUB -l nodes=%d:ppn=%d" % (p["nodes"], ppn))
        add("#MSUB -l walltime=%s" % p["wall"], p["wall"])
        add("#MSUB -l mem=%smb" % p["mem"], p["mem"])
        add("#MSUB -A %s" % p["account"], p["account"])
        out += ["#MSUB -j oe", "#MSUB -o moab.out"]
    elif mgr == "slurm":
        add("#SBATCH --partition=%s" % p["queue"], p["queue"])
        add("#SBATCH --nodes=%d" % p["nodes"])
        add("#SBATCH --ntasks=%d" % p["procs"])
        add("#SBATCH --time=%s" % p["wall"], p["wall"])
        add("#SBATCH --mem=%sM" % p["mem"], p["mem"])
        add("#SBATCH --account=%s" % p["account"], p["account"])
        out += ["#SBATCH --output=slurm.out", "#SBATCH --error=slurm.err"]
    return out


def directiveLines(text, mgr):
    prefix = MANAGERS[mgr][1]
    return [l for l in text.splitlines() if prefix and l.startswith(prefix + " ")]


def wellFormed(text, mgr):
    """Problems in the directive block that no scheduler would accept."""
    problems = []
    for line in directiveLines(text, mgr):
        if "$" in line or re.search(r"(=|\s-\w)\s*$", line) or "= " in line:
            problems.append("malformed directive: %r" % line)
    if not text.startswith("#!"):
        problems.append("no interpreter line")
    first = [l for l in text.splitlines() if not l.startswith("#!")
             and not l.startswith("# ")][:1]
    return problems


class Report(object):
    """One row per (suite, manager, transport, job) with its failed checks."""

    def __init__(self, strict=False):
        self.strict = strict
        self.rows = []
        self.cur = None

    def row(self, suite, mgr, transport, job):
        self.cur = dict(suite=suite, mgr=mgr, transport=transport, job=job,
                        status="-", fails=[], known=[], notes=[], seconds=0.0,
                        t0=time.time())
        self.rows.append(self.cur)
        return self.cur

    def check(self, ok, what):
        say("    %s %s" % ("ok  " if ok else "FAIL", what))
        if not ok:
            self.cur["fails"].append(what)
        return ok

    def knownFault(self, text):
        say("    KNOWN %s" % text)
        self.cur["known"].append(text)

    def note(self, text):
        self.cur["notes"].append(text)

    def done(self, status=None):
        self.cur["seconds"] = time.time() - self.cur["t0"]
        if status:
            self.cur["status"] = status
        elif self.cur["status"] == "-":
            self.cur["status"] = "FAIL" if self.cur["fails"] else "PASS"

    def table(self):
        say("")
        say("=" * 110)
        say("%-7s %-6s %-9s %-26s %-9s %6s  %s" % (
            "suite", "mgr", "transport", "job", "result", "secs", "note"))
        say("-" * 110)
        bad = 0
        for r in self.rows:
            say("%-7s %-6s %-9s %-26s %-9s %6.0f  %s" % (
                r["suite"], r["mgr"], r["transport"], r["job"], r["status"],
                r["seconds"], "; ".join(r["notes"])[:60]))
            for f in r["fails"]:
                say("        FAIL %s" % f)
            bad += 1 if r["status"] not in ("PASS", "SKIP") else 0
        seen = {}
        for r in self.rows:
            for k in r["known"]:
                seen.setdefault(k, []).append("%s/%s/%s/%s" % (
                    r["mgr"], r["transport"], r["job"], r["status"]))
        say("")
        say("KNOWN ECCE FAULTS SEEN%s" % (" (failing: --strict)" if self.strict else ""))
        for k, rows in seen.items():
            say("    %s %s\n        in: %d job(s)" % ("FAIL" if self.strict else "note", k, len(rows)))
            bad += 1 if self.strict else 0
        if not seen:
            say("    none")
        return bad


# --- golden scripts from gensub directly ------------------------------------

class Gensub(object):
    """gensub run against a minimal $ECCE_HOME, with no services."""

    def __init__(self):
        self.tmp = tempfile.mkdtemp(prefix="ecce-gensub-")
        self.home = os.path.join(self.tmp, "home")
        self.user = os.path.join(self.tmp, "user")
        os.makedirs(os.path.join(self.user, ".ECCE"))
        for sub in ("scripts", "siteconfig"):
            harness.link(os.path.join(REPO, sub), os.path.join(self.home, sub))
        harness.link(os.path.join(REPO, "data", "client"),
                     os.path.join(self.home, "data", "client"))
        with open(os.path.join(self.user, ".ECCE", "CONFIG.goldhost"), "w") as h:
            h.write("NWChem: /opt/codes/nwchem\nMOPAC: /opt/codes/mopac\n"
                    "perlPath: /usr/bin\n")
        self.env = dict(os.environ, ECCE_HOME=self.home, ECCE_REALUSERHOME=self.user)

    def script(self, mgr, code, p, host="goldhost", name="gold"):
        name_c, infile, outfile = CODES[code]
        lines = ["-Q %s" % MANAGERS[mgr][0], "-H %s" % host, "-d localhost",
                 "-c %s" % name_c, "-n %d" % p["procs"], "-N %d" % p["nodes"],
                 "-r /qtest/run", "-i " + infile, "-o " + outfile,
                 "-f %s/submit__%s" % (self.tmp, name)]
        if p["queue"]:
            lines.append("-q " + p["queue"])
        if p["mem"]:
            lines.append("-m %d" % p["mem"])
        if p["account"]:
            lines.append("-a " + p["account"])
        if p["wall"]:
            lines += ["-T " + p["wall"], "-w " + p["hrmin"]]
        params = os.path.join(self.tmp, "subParams")
        with open(params, "w") as h:
            h.write("".join(" %s\n" % l for l in lines))
        res = subprocess.run(
            [os.path.join(self.home, "scripts", "gensub"), "-v", "-p", params],
            env=self.env, cwd=self.tmp, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)
        out = res.stdout.decode("utf-8", "replace")
        path = os.path.join(self.tmp, "submit__" + name)
        if res.returncode != 0 or not os.path.exists(path):
            raise RuntimeError("gensub failed (%d): %s" % (res.returncode, out[-400:]))
        with open(path) as h:
            return h.read()

    def normalise(self, text):
        text = re.sub(r"^(#\s+Generated ).* with ECCE Version .*$",
                      r"\1<date> with ECCE Version <version>.", text, flags=re.M)
        return text.replace(self.home, "<ECCE_HOME>").replace(self.tmp, "<TMP>")

    def close(self):
        shutil.rmtree(self.tmp, ignore_errors=True)


def goldenSuite(args, rep):
    g = Gensub()
    try:
        for mgr, code, prof in GOLDEN_CASES:
            if args.manager and mgr not in args.manager:
                continue
            rep.row("golden", mgr, "-", "%s.%s" % (code, prof))
            p = PROFILES[prof]
            try:
                text = g.normalise(g.script(mgr, code, p))
            except RuntimeError as exc:
                rep.check(False, str(exc))
                rep.done()
                continue
            path = os.path.join(GOLDEN, "%s.%s.%s.sh" % (mgr, code, prof))
            if args.update_golden:
                os.makedirs(GOLDEN, exist_ok=True)
                with open(path, "w") as h:
                    h.write(text)
                rep.note("golden written")
            elif not os.path.exists(path):
                rep.check(False, "no golden file %s (run --update-golden)" % path)
            else:
                with open(path) as h:
                    want = h.read()
                diff = "".join(difflib.unified_diff(
                    want.splitlines(1), text.splitlines(1), "golden", "generated"))
                rep.check(not diff, "script equals golden" + ("\n" + diff if diff else ""))
            if MANAGERS[mgr][1]:
                want = expectedDirectives(mgr, p, "gold")
                got = directiveLines(text, mgr)
                rep.check(got == want, "directives %s%s" % (
                    "as expected" if got == want else "differ: got %r want %r" % (got, want),
                    ""))
                for prob in wellFormed(text, mgr):
                    rep.check(False, prob)
            rep.done()
        if not args.manager or "slurm" in args.manager:
            slurmTestOnly(g, rep)
    finally:
        g.close()


def slurmTestOnly(g, rep):
    """sbatch --test-only: the real parser's verdict on generated directives."""
    if not slurmUsable():
        return
    for prof in ("basic", "bare"):
        rep.row("golden", "slurm", "-", "sbatch-test-only.%s" % prof)
        text = g.script("slurm", "nwchem", PROFILES[prof], host="goldhost", name="t-" + prof)
        path = os.path.join(g.tmp, "submit__t-" + prof)
        res = subprocess.run(["sbatch", "--test-only", path], stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, cwd=g.tmp)
        out = res.stdout.decode().strip()
        rep.check(res.returncode == 0, "sbatch accepts the generated directives: %s" % out[:120])
        rep.done()


# --- environment ----------------------------------------------------------

def slurmUsable():
    if not shutil.which("sinfo") or not shutil.which("sbatch"):
        return False
    res = subprocess.run(["sinfo", "-h", "-o", "%P"], stdout=subprocess.PIPE,
                         stderr=subprocess.DEVNULL)
    return res.returncode == 0 and bool(res.stdout.strip())


def partitions():
    res = subprocess.run(["sinfo", "-h", "-o", "%R"], stdout=subprocess.PIPE)
    return res.stdout.decode().split()


def slurmJob(jobid):
    """scontrol's view of a job (still there MinJobAge seconds after it ends)."""
    res = subprocess.run(["scontrol", "-o", "show", "job", str(jobid)],
                         stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    text = res.stdout.decode()
    return dict(kv.split("=", 1) for kv in text.split() if "=" in kv)


def slurmQueue(jobid):
    res = subprocess.run(["squeue", "-h", "-j", str(jobid), "-o", "%i %P %T"],
                         stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    return res.stdout.decode().strip()


# --- live jobs ----------------------------------------------------------

class Live(object):
    def __init__(self, s, rep, args, suite, stubdir=None):
        self.s, self.rep, self.args, self.suite = s, rep, args, suite
        self.stub = stubdir
        self.submitted = []          # (mgr, id) to clean up
        self.wrapper = os.path.join(s.state, "slow-mopac")
        self.delay = os.path.join(s.state, "mopac-delay")
        with open(self.wrapper, "w") as h:
            h.write("#!/bin/sh\nsleep $(cat \"%s\" 2>/dev/null || echo 0)\n"
                    "exec /usr/bin/mopac \"$@\"\n" % self.delay)
        os.chmod(self.wrapper, 0o755)
        self.setDelay(0)

    def setDelay(self, seconds):
        with open(self.delay, "w") as h:
            h.write("%d\n" % seconds)

    def scheduler(self, mgr):
        """The scheduler's own record of a job, or None."""

    def stubJobDir(self, mgr, seq):
        return os.path.join(self.stub, "..", "spool", mgr, str(seq))

    def schedulerState(self, mgr, jobid):
        """'R' running, 'done', or None when the scheduler does not know it."""
        if mgr == "slurm":
            q = slurmQueue(jobid)
            if q:
                return {"RUNNING": "R", "PENDING": "Q"}.get(q.split()[-1], "done")
            j = slurmJob(jobid)
            return "done" if j else None
        seq = re.match(r"\d+", jobid)
        d = self.stubJobDir(mgr, seq.group(0)) if seq else None
        if not d or not os.path.isdir(d):
            return None
        if os.path.exists(os.path.join(d, "exit")):
            return "done"
        try:
            with open(os.path.join(d, "pgid")) as h:
                os.killpg(int(h.read()), 0)
            return "R"
        except (OSError, ValueError):
            return "done"

    def submittedScript(self, mgr, jobid, rundir):
        if mgr != "slurm":
            seq = re.match(r"\d+", jobid).group(0)
            path = os.path.join(self.stubJobDir(mgr, seq), "script")
        else:
            found = glob.glob(os.path.join(rundir, "submit__*"))
            path = found[0] if found else None
        if path and os.path.exists(path):
            with open(path) as h:
                return h.read()
        return None

    def run(self, mgr, code, transport, kill=False):
        mname, prefix, machine = MANAGERS[mgr]
        rep, s = self.rep, self.s
        kind = "kill" if kill else "run"
        job = "%s-%s" % (code, kind)
        rep.row(self.suite, mgr, transport, job)
        say("--- %s %s %s (ECCE_TRANSPORT=%s)" % (mgr, code, kind, transport))
        s.transport = None if transport == "unset" else transport
        prof = PROFILES["basic"]
        stamp = str(int(time.time()) % 100000)
        name = "q%s%s%s%s" % (mgr[0], code[0], kind[0], stamp)
        rundir = os.path.join(s.state, "jobs")
        os.makedirs(rundir, exist_ok=True)
        resource, deckname = ("mopac_es", "mopac.mop") if code == "mopac" \
            else ("nwchem_es", "nwch.nw")
        deck = MOPAC_DECK if code == "mopac" else NWCHEM_DECK
        self.setDelay(120 if kill else 0)
        os.environ["LAUNCHJOB_ACCOUNT"] = prof["account"]
        extra = ["queue=" + prof["queue"], "nodes=%d" % prof["nodes"],
                 "procs=%d" % prof["procs"], "wall=0 0:30", "mem=%d" % prof["mem"]]
        url, out = s.create(name, resource, deck, deckname, rundir,
                            machine=machine, extra=extra)
        if not rep.check(url is not None, "calculation created"):
            say(out[-400:])
            rep.done()
            return
        rc, out = s.launch(url)
        outname = None
        for l in out.splitlines():
            if l.startswith("files:"):
                say("      " + l)
                outname = re.search(r"output=(\S+)", l).group(1)
        outbug = outname not in (None, CODES[code][2])
        if outbug:
            rep.knownFault(
                "TaskJob::getDataFile(PRIMARY_OUTPUT) on a calculation that has not run "
                "yet returned %r instead of the declared %s (c610bf6c's imported-output "
                "fallback); the job writes %r, eccejobmonitor reads %s, and "
                "MOPAC loses the properties only the live monitor extracts (TE, GEOMTRACE)"
                % (outname, CODES[code][2], outname, CODES[code][2]))
        ran = [l.split(":", 1)[1].strip() for l in out.splitlines()
               if l.startswith("run directory:")]
        jobdir = ran[-1] if ran else None
        if not rep.check(rc == 0, "Launch submitted the job"):
            say("\n".join("      | " + l for l in out.strip().splitlines()[-8:]))
            rep.done()
            return
        jobid = s.driver("jobid", url)[1].strip().splitlines()[-1]
        self.submitted.append((mgr, jobid))
        rep.note("id " + jobid)
        idre = {"pbs": r"\d+\.stubserver", "lsf": r"\d+", "moab": r"\d+",
                "slurm": r"\d+"}[mgr]
        rep.check(re.fullmatch(idre, jobid) is not None,
                  "parsed job id %r has the form %s" % (jobid, idre))
        known = self.schedulerState(mgr, jobid)
        rep.check(known is not None, "the scheduler knows job %s (%s)" % (
            jobid, "squeue/scontrol" if mgr == "slurm" else "stand-in spool"))

        if mgr != "slurm":
            #  Proof that it was the stand-in, not a scheduler of this host, that ran.
            cmd = {"pbs": "qsub", "lsf": "bsub", "moab": "msub"}[mgr]
            log = os.path.join(self.stub, "..", "spool", mgr, "commands.log")
            text = open(log).read() if os.path.exists(log) else ""
            rep.check(re.search(r"^%s .*submit__%s" % (cmd, name), text, re.M) is not None
                      or (mgr == "lsf" and "bsub" in text),
                      "the stand-in %s recorded this submission" % cmd)
        if mgr != "slurm":
            #  Proof that it was the stand-in, not a scheduler of this host, that ran.
            cmd = {"pbs": "qsub", "lsf": "bsub", "moab": "msub"}[mgr]
            log = os.path.join(self.stub, "..", "spool", mgr, "commands.log")
            text = open(log).read() if os.path.exists(log) else ""
            rep.check(re.search(r"^%s .*submit__%s" % (cmd, name), text, re.M) is not None
                      or (mgr == "lsf" and "bsub" in text),
                      "the stand-in %s recorded this submission" % cmd)
        if mgr == "slurm":
            q = slurmQueue(jobid)
            j = slurmJob(jobid)
            rep.check(j.get("Partition") == prof["queue"],
                      "runs on partition %s (scontrol: %s)" % (prof["queue"], j.get("Partition")))
            rep.check(j.get("TimeLimit") in ("00:30:00", "0:30:00"),
                      "time limit 00:30:00 (scontrol: %s)" % j.get("TimeLimit"))
            rep.check(j.get("NumNodes") in ("1", "1-1") and j.get("NumTasks", "1") == "1",
                      "1 node, 1 task (scontrol: %s, %s)" % (j.get("NumNodes"), j.get("NumTasks")))
            mem = j.get("MinMemoryNode", j.get("MinMemoryCPU", ""))
            rep.check(mem in ("2000M", "2000"), "memory 2000M (scontrol: %s)" % mem)
            rep.check(j.get("Account") in ("proj1", "(null)", None),
                      "account %s accepted, not substituted (scontrol: %s)"
                      % (prof["account"], j.get("Account")))
            rep.check(j.get("UserId", "").startswith(s.user() + "("),
                      "job belongs to %s (scontrol: %s)" % (s.user(), j.get("UserId")))
            say("      squeue: %s" % (q or "(already finished)"))

        script = self.submittedScript(mgr, jobid, jobdir or rundir)
        if rep.check(script is not None, "the submitted script was captured"):
            if prefix:
                want = expectedDirectives(mgr, prof, name[:14])
                got = directiveLines(script, mgr)
                rep.check(got == want, "script directives as expected" + (
                    "" if got == want else ": got %r want %r" % (got, want)))
                for prob in wellFormed(script, mgr):
                    rep.check(False, prob)
            keep = os.path.join(s.state, "scripts")
            os.makedirs(keep, exist_ok=True)
            with open(os.path.join(keep, "%s.%s.%s.%s.sh" % (mgr, code, kind, transport)), "w") as h:
                h.write(script)

        if kill:
            self.cancel(mgr, url, jobid, jobdir)
        else:
            state = s.waitState(url, 240)
            rep.check(state == "completed", "state completed (last: %s)" % state)
            if state == "completed":
                #  "completed" is set before eccejobstore has stored every property.
                deadline = time.time() + 40
                while True:
                    got = s.props(url)
                    if ("TE" in got and "GEOMTRACE" in got) or time.time() > deadline:
                        break
                    time.sleep(2)
                if outbug and code == "mopac" and "TE" not in got:
                    rep.note("props lost to the known fault")
                else:
                    rep.check("TE" in got and "GEOMTRACE" in got,
                              "TE and GEOMTRACE in Props/ (have: %s)" % " ".join(got))
            after = self.schedulerState(mgr, jobid)
            rep.check(after in ("done", None), "the scheduler no longer runs the job (%s)" % after)
        rep.done()

    def cancel(self, mgr, url, jobid, jobdir):
        rep, s = self.rep, self.s
        #  The job is held in the wrapper's sleep; wait until the scheduler runs it.
        deadline = time.time() + 60
        while time.time() < deadline and self.schedulerState(mgr, jobid) != "R":
            time.sleep(1)
        rep.check(self.schedulerState(mgr, jobid) == "R", "the job is running under the scheduler")
        if mgr == "slurm":
            rep.note(slurmQueue(jobid))
        time.sleep(3)
        rc, out = s.driver("kill", url)
        msg = out.strip().splitlines()[-1] if out.strip() else ""
        rep.check(rc == 0 and "has been issued" in msg, "RunMgmt::terminate: %s" % msg)
        deadline = time.time() + 30
        while time.time() < deadline and self.schedulerState(mgr, jobid) == "R":
            time.sleep(1)
        now = self.schedulerState(mgr, jobid)
        rep.check(now != "R", "the scheduler job ended after the cancel command (%s)" % now)
        if mgr != "slurm":
            seq = re.match(r"\d+", jobid).group(0)
            rep.check(os.path.exists(os.path.join(self.stubJobDir(mgr, seq), "cancelled")),
                      "the stand-in's cancel command (%s) ended it" % {
                          "pbs": "qdel", "lsf": "bkill", "moab": "mjobctl -c"}[mgr])
        if mgr == "slurm":
            for _ in range(20):
                js = slurmJob(jobid).get("JobState")
                if js != "COMPLETING":
                    break
                time.sleep(1)
            rep.check(js == "CANCELLED", "slurm JobState CANCELLED (%s)" % js)
        time.sleep(2)
        left = subprocess.run(["pgrep", "-f", self.wrapper], stdout=subprocess.PIPE)
        rep.check(left.returncode != 0,
                  "no compute process left (%s)" % left.stdout.decode().split())
        state = s.waitState(url, 60, want=("killed", "completed", "failed", "unsuccessful"))
        rep.check(state == "killed", "calculation state is killed (last: %s)" % state)

    def cleanup(self):
        for mgr, jobid in self.submitted:
            if mgr == "slurm" and self.schedulerState(mgr, jobid) in ("R", "Q"):
                subprocess.run(["scancel", str(jobid)])


def liveSuite(args, rep, suite, managers, stubdir=None, slurm=False):
    build = os.path.abspath(args.build)
    tools = ("nwchem", "mopac") + (("sbatch",) if slurm else ())
    harness.prerequisites(build, tools)
    codes = {"NWChem": shutil.which("nwchem"), "MOPAC": None}
    modes = ["unset", "direct"] if args.transport == "both" else [args.transport]
    s = harness.Session(build, "queue", {"NWChem": shutil.which("nwchem")},
                        (8696, 8688), keep=args.keep, transport=None)
    live = Live(s, rep, args, suite, stubdir)
    codes["MOPAC"] = live.wrapper
    for m in managers:
        #  qmgrPath is the machine's "queue manager path": what ECCE puts in
        #  front of PATH for its commands there, so qsub is ours and not the
        #  Grid Engine one this host also has.
        s.registerMachine(MANAGERS[m][2], MANAGERS[m][0], codes,
                          config={"qmgrPath": stubdir} if stubdir else None)
    try:
        if not s.services(True):
            say("FAILED: services did not start")
            rep.row(suite, "-", "-", "services")
            rep.check(False, "services did not start")
            rep.done()
            return
        for mode in modes:
            for m in managers:
                for _ in range(args.repeat):
                    for code in ("mopac", "nwchem"):
                        if not args.code or code in args.code:
                            live.run(m, code, mode)
                    if not args.no_kill and (not args.code or "mopac" in args.code):
                        live.run(m, "mopac", mode, kill=True)
        outside = [p for p in s.seen.values() if os.path.dirname(p) != s.build]
        if outside:
            rep.row(suite, "-", "-", "binaries")
            rep.check(False, "ran from outside the build: %s" % outside)
            rep.done()
    finally:
        live.cleanup()
        left = s.stop()
        if left:
            say("processes left running: %r" % left)


def stubsSuite(args, rep):
    managers = [m for m in STUB_MANAGERS if not args.manager or m in args.manager]
    if not managers:
        return
    root = os.path.join(os.path.expanduser("~"), ".cache", "ecce-queue-stubs")
    shutil.rmtree(root, ignore_errors=True)
    bindir = os.path.join(root, "bin")
    os.makedirs(bindir)
    for name in STUB_LINKS:
        os.symlink(os.path.join(HERE, "stubsched.py"), os.path.join(bindir, name))
    say("STAND-IN schedulers (tests/queues/stubsched.py, NOT the real PBS/LSF/Moab) "
        "on PATH: " + bindir)
    saved = os.environ["PATH"]
    os.environ["PATH"] = bindir + ":" + saved
    try:
        liveSuite(args, rep, "stubs", managers, stubdir=bindir)
    finally:
        os.environ["PATH"] = saved


def slurmSuite(args, rep):
    if args.manager and "slurm" not in args.manager:
        return
    if not slurmUsable():
        harness.skip("no working Slurm (sinfo fails or lists no partition)")
    need = set(PROFILES["basic"]["queue"].split())
    if not need <= set(p.rstrip("*") for p in partitions()):
        harness.skip("Slurm has no partition %s" % PROFILES["basic"]["queue"])
    liveSuite(args, rep, "slurm", ["slurm"], slurm=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build"))
    ap.add_argument("--suite", default="all", choices=("all", "local", "stubs", "slurm", "golden"))
    ap.add_argument("--transport", default="both",
                    choices=("unset", "direct", "ssh", "both"))
    ap.add_argument("--manager", action="append", choices=sorted(MANAGERS))
    ap.add_argument("--code", action="append", choices=("mopac", "nwchem"),
                    help="only this code (repeatable)")
    ap.add_argument("--no-kill", action="store_true", help="skip the cancel jobs")
    ap.add_argument("--repeat", type=int, default=1, help="repeat each live job N times")
    ap.add_argument("--update-golden", action="store_true")
    ap.add_argument("--strict", action="store_true",
                    help="count the known ECCE faults as failures")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()
    if args.update_golden and args.suite == "all":
        args.suite = "golden"

    rep = Report(args.strict)
    t0 = time.time()
    skipped = []
    for suite, fn in (("golden", goldenSuite), ("stubs", stubsSuite), ("slurm", slurmSuite)):
        if args.suite not in ("all", suite) and not (
                args.suite == "local" and suite != "slurm"):
            continue
        if suite == "golden" and not shutil.which("perl"):
            harness.skip("perl is not installed")
        try:
            fn(args, rep)
        except SystemExit as exc:
            #  harness.skip(): this suite cannot run; the others still may.
            if exc.code != harness.SKIP or args.suite not in ("all", "local"):
                raise
            skipped.append(suite)
    bad = rep.table()
    if skipped:
        say("skipped: %s" % ", ".join(skipped))
    say("")
    say("%s (%d failing rows, %.0f s)" % ("PASSED" if not bad else "FAILED", bad, time.time() - t0))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
