#!/usr/bin/env python3
"""Batch-queue support: submit script, submission, job id, completion, cancel.

Suites, over the queue managers ECCE ships in siteconfig/QueueManagers:

  stubs     PBS, LSF and Moab against STAND-IN clients (tests/queues/stubsched.py,
            installed as qsub/qdel/qstat, bsub/bkill/bjobs, msub/mjobctl/checkjob
            on the machines' queue manager path).  They run the submitted script
            in the background and print a job id in the real client's format.
            They prove ECCE's side of the protocol (script, submit command, id
            parsing, cancel command) and say nothing about the real schedulers.
  slurm, sge, htcondor
            the real scheduler of this machine; each is SKIPped when absent.
  golden    gensub alone, against tests/queues/golden/.

The live suites launch MOPAC and NWChem through the real Launch
(tests/launch/launchjob) on a machine registered under the queue manager, with
queue, node, processor, wall time, memory and account set as the launcher's queue
controls set them, and cancel a long job through RunMgmt::terminate, which is what
the Organizer's Kill calls.

The submit script of every manager is also generated directly by gensub for
fixed settings and compared with tests/queues/golden/, so the csh-to-sh port
of gensub can be checked against what csh produced.

    tests/queues/run_tests.py [--build build] [--suite local|golden|stubs|slurm|sge|htcondor|all]
                              [--transport unset|direct|both] [--manager pbs ...]
                              [--update-golden] [--keep] [-v]

"local" is golden plus stubs: everything that needs no scheduler.

Exit status 77 (CTest SKIP) when a prerequisite is missing; for a real scheduler
that includes its client failing or listing no queue.
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
    "sge": ("SGE", "#$", "sgetest"),
    "htcondor": ("HTCondor", "#CONDOR", "condortest"),
    "shell": ("Shell", None, "goldhost"),
}
STUB_MANAGERS = ("pbs", "lsf", "moab")
REAL_MANAGERS = ("slurm", "sge", "htcondor")
#  The queue a real scheduler of this host accepts for the "basic" profile.
LIVE_QUEUE = {"slurm": "debug", "sge": "all.q", "htcondor": "pool"}

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


def wallSeconds(wall):
    h, m, sec = (int(x) for x in wall.split(":"))
    return h * 3600 + m * 60 + sec


def expectedDirectives(mgr, p, jobname, rundir="/qtest/run"):
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
        out += ["#PBS -j oe", "#PBS -o pbs.out", "#PBS -S /bin/sh"]
    elif mgr == "lsf":
        add("#BSUB -J " + jobname)
        add("#BSUB -q %s" % p["queue"], p["queue"])
        add("#BSUB -n %d" % p["procs"])
        add("#BSUB -W %s" % p["hrmin"], p["hrmin"])
        add("#BSUB -M %s" % p["mem"], p["mem"])
        add("#BSUB -P %s" % p["account"], p["account"])
        out += ["#BSUB -o %s.out" % jobname, "#BSUB -e %s.err" % jobname,
                "#BSUB -L /bin/sh"]
    elif mgr == "moab":
        add("#MSUB -N " + jobname)
        add("#MSUB -q %s" % p["queue"], p["queue"])
        add("#MSUB -l nodes=%d:ppn=%d" % (p["nodes"], ppn))
        add("#MSUB -l walltime=%s" % p["wall"], p["wall"])
        add("#MSUB -l mem=%smb" % p["mem"], p["mem"])
        add("#MSUB -A %s" % p["account"], p["account"])
        out += ["#MSUB -j oe", "#MSUB -o moab.out"]
    elif mgr == "sge":
        add("#$ -N " + jobname)
        add("#$ -q %s" % p["queue"], p["queue"])
        add("#$ -pe smp %d" % p["procs"])
        add("#$ -l h_rt=%s" % p["wall"], p["wall"])
        add("#$ -l mem_free=%sM" % p["mem"], p["mem"])
        add("#$ -A %s" % p["account"], p["account"])
        out += ["#$ -S /bin/sh", "#$ -cwd", "#$ -v SHELL", "#$ -j y", "#$ -o sge.out"]
    elif mgr == "htcondor":
        out += ["#CONDOR executable = @SCRIPT@", "#CONDOR initialdir = " + rundir,
                "#CONDOR should_transfer_files = NO", "#CONDOR notification = never",
                '#CONDOR environment = "HOME=$ENV(HOME)"',
                "#CONDOR request_cpus = %d" % p["procs"]]
        add("#CONDOR request_memory = %s" % p["mem"], p["mem"])
        if p["wall"]:
            out.append("#CONDOR periodic_remove = (JobStatus == 2) && "
                       "((time() - EnteredCurrentStatus) > %d)" % wallSeconds(p["wall"]))
        add('#CONDOR +ProjectName = "%s"' % p["account"], p["account"])
        out += ["#CONDOR output = condor.out", "#CONDOR error = condor.err",
                "#CONDOR log = condor.log", "#CONDOR queue"]
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
        body = line[len(MANAGERS[mgr][1]):]
        if mgr == "htcondor":
            #  A description line is "key = value"; an empty value is the fault.
            if re.search(r"=\s*$|\$(?!ENV\()", body) or (body.strip() != "queue" and "=" not in body):
                problems.append("malformed directive: %r" % line)
        elif "$" in body or re.search(r"(=|\s-\w)\s*$", line) or "= " in line:
            problems.append("malformed directive: %r" % line)
    if not text.startswith("#!"):
        problems.append("no interpreter line")
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
        say("%-8s %-8s %-9s %-26s %-9s %6s  %s" % (
            "suite", "mgr", "transport", "job", "result", "secs", "note"))
        say("-" * 110)
        bad = 0
        for r in self.rows:
            say("%-8s %-8s %-9s %-26s %-9s %6.0f  %s" % (
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
        self.config("")
        self.env = dict(os.environ, ECCE_HOME=self.home, ECCE_REALUSERHOME=self.user)

    def config(self, extra):
        with open(os.path.join(self.user, ".ECCE", "CONFIG.goldhost"), "w") as h:
            h.write("NWChem: /opt/codes/nwchem\nMOPAC: /opt/codes/mopac\n"
                    "perlPath: /usr/bin\n" + extra)

    def script(self, mgr, code, p, host="goldhost", name="gold", rundir="/qtest/run"):
        name_c, infile, outfile = CODES[code]
        lines = ["-Q %s" % MANAGERS[mgr][0], "-H %s" % host, "-d localhost",
                 "-c %s" % name_c, "-n %d" % p["procs"], "-N %d" % p["nodes"],
                 "-r " + rundir, "-i " + infile, "-o " + outfile,
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
            for prob in notShell(text):
                rep.check(False, prob)
            if MANAGERS[mgr][1]:
                want = expectedDirectives(mgr, p, "gold")
                got = directiveLines(text, mgr)
                rep.check(got == want, "directives %s%s" % (
                    "as expected" if got == want else "differ: got %r want %r" % (got, want),
                    ""))
                for prob in wellFormed(text, mgr):
                    rep.check(False, prob)
            rep.done()
        snippetCases(g, rep, args)
        realParsers(g, rep, args)
    finally:
        g.close()


CSH_LEFTOVERS = re.compile(
    r"^\s*(?:setenv|foreach|endif|onintr|set\s+\w+\s*=)\b|\$status\b|\$\?\w|(?<![0-9])>&(?![0-9])|\$\w+:[htre]\b|/bin/t?csh")


def notShell(text):
    """Problems with a generated script as a POSIX sh script."""
    problems = []
    if text.splitlines()[0] != "#!/bin/sh":
        problems.append("interpreter line is %r, not #!/bin/sh" % text.splitlines()[0])
    for l in text.splitlines():
        if not l.startswith("#") and CSH_LEFTOVERS.search(l):
            problems.append("csh syntax left in the script: %r" % l)
    sh = shutil.which("dash") or "/bin/sh"
    res = subprocess.run([sh, "-n"], input=text.encode(), stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    if res.returncode != 0:
        problems.append("%s -n rejects the script: %s" % (os.path.basename(sh),
                        res.stdout.decode("utf-8", "replace").strip()[:200]))
    return problems


#  What a site's csh snippet becomes: (CONFIG text, substring expected in the
#  script, substring that must be gone); or (text, None, error fragment).
SNIPPETS = [
    ("setup {\nsetenv MLIB_NUMBER_OF_THREADS 1\nsetenv OMP_NUM_THREADS \"1\"\n}\n",
     "export MLIB_NUMBER_OF_THREADS=1\nexport OMP_NUM_THREADS=\"1\"", "setenv"),
    ("NWChemCommand {\nprun -n $totalprocs $nwchem $inFile >& $outFile\n}\n",
     "prun -n 1 $nwchem nwch.nw > nwch.nwout 2>&1", ">& "),
    ("NWChemCommand {\nmpirun -np 2 $nwchem $inFile >&! $outFile < /dev/null\n}\n",
     "mpirun -np 2 $nwchem nwch.nw > nwch.nwout 2>&1 < /dev/null", ">&!"),
    ("wrapup {\nset refund = \"refund.out\"\ncat $refund >> $outFile\n}\n",
     "refund=\"refund.out\"\ncat $refund", "set refund"),
    ("setup {\nif ($?SCRATCH) then\n  echo yes\nendif\n}\n",
     None, "setup, line 1"),
    ("setup {\nset procs = (a b)\nsource /etc/csh.login\nforeach f ($x)\nend\n}\n",
     None, "setup, line 3"),
    ("NWChemCommand {\nsetenv X a b\n}\n", None, "nwchemcommand, line 1"),
]


def snippetCases(g, rep, args):
    if args.manager and "shell" not in args.manager:
        return
    for i, (text, want, other) in enumerate(SNIPPETS):
        rep.row("golden", "shell", "-", "snippet.%d" % (i + 1))
        g.config(text)
        try:
            out = g.script("shell", "nwchem", PROFILES["basic"])
            if want is None:
                rep.check(False, "gensub accepted csh syntax: %r" % text)
            else:
                rep.check(want in out, "translated to %r" % want)
                rep.check(other not in out, "%r is gone" % other)
                for prob in notShell(out):
                    rep.check(False, prob)
        except RuntimeError as exc:
            msg = str(exc)
            rep.check(want is None and other in msg,
                      "gensub names the line: %s" % " ".join(msg.split())[:120])
        rep.done()
    g.config("")


# --- the schedulers ---------------------------------------------------------

def sh(argv, timeout=60):
    res = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         timeout=timeout)
    return res.returncode, res.stdout.decode("utf-8", "replace")


def pollFor(fn, seconds, every=2):
    """fn() until it returns something truthy, for at most `seconds`."""
    deadline = time.time() + seconds
    while True:
        got = fn()
        if got or time.time() > deadline:
            return got
        time.sleep(every)


class Sched(object):
    """One scheduler as the test sees it: job state and its own record of a job."""
    real = True

    def usable(self):
        """None when it can be used, else why not."""

    def state(self, jobid):
        """'R' running, 'Q' waiting, 'done', or None when it has never heard of it."""

    def record(self, jobid):
        """Text of the scheduler's own description of the job ('' if none)."""
        return ""

    def verify(self, rep, prof, name, jobdir, jobid, final):
        """Assert that the scheduler holds the settings the launcher gave."""

    def cancelled(self, rep, jobid):
        """Assert how this scheduler records a cancelled job."""

    def drop(self, jobid):
        """Remove the job if still there (cleanup after a failed test)."""


class SlurmSched(Sched):
    def usable(self):
        if not shutil.which("sinfo") or not shutil.which("sbatch"):
            return "no sbatch/sinfo"
        rc, out = sh(["sinfo", "-h", "-o", "%R"])
        if rc != 0 or not out.split():
            return "sinfo fails or lists no partition"
        if LIVE_QUEUE["slurm"] not in out.split():
            return "Slurm has no partition %s" % LIVE_QUEUE["slurm"]

    def job(self, jobid):
        rc, out = sh(["scontrol", "-o", "show", "job", str(jobid)])
        return dict(kv.split("=", 1) for kv in out.split() if "=" in kv)

    def state(self, jobid):
        rc, out = sh(["squeue", "-h", "-j", str(jobid), "-o", "%T"])
        q = out.strip() if rc == 0 else ""
        if q:
            return {"RUNNING": "R", "PENDING": "Q"}.get(q.split()[0], "done")
        return "done" if self.job(jobid) else None

    def record(self, jobid):
        rc, out = sh(["scontrol", "show", "job", str(jobid)])
        return out if rc == 0 else ""

    def verify(self, rep, prof, name, jobdir, jobid, final):
        j = self.job(jobid)
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
        rep.check(j.get("UserId", "").split("(")[0] == os.environ.get("USER", ""),
                  "job belongs to this user (scontrol: %s)" % j.get("UserId"))

    def cancelled(self, rep, jobid):
        js = pollFor(lambda: self.job(jobid).get("JobState") not in (None, "COMPLETING")
                     and self.job(jobid).get("JobState"), 20, 1)
        rep.check(js == "CANCELLED", "slurm JobState CANCELLED (%s)" % js)

    def drop(self, jobid):
        if self.state(jobid) in ("R", "Q"):
            sh(["scancel", str(jobid)])


class SgeSched(Sched):
    def usable(self):
        if not shutil.which("qsub") or not shutil.which("qstat") or not shutil.which("qconf"):
            return "Grid Engine client commands not installed"
        rc, out = sh(["qstat", "-g", "c"])
        if rc != 0 or LIVE_QUEUE["sge"] not in out:
            return "no working Grid Engine queue %s (qstat -g c: %s)" % (
                LIVE_QUEUE["sge"], out.strip()[:60])

    def state(self, jobid):
        rc, out = sh(["qstat", "-u", "*"])
        for line in out.splitlines():
            f = line.split()
            if f and f[0] == str(jobid):
                return "R" if f[4] in ("r", "t", "Rr") else "Q"
        rc, out = sh(["qacct", "-j", str(jobid)])
        return "done" if rc == 0 else None

    def record(self, jobid):
        rc, out = sh(["qstat", "-j", str(jobid)])
        if rc == 0:
            return out
        rc, out = sh(["qacct", "-j", str(jobid)])
        return out if rc == 0 else ""

    def verify(self, rep, prof, name, jobdir, jobid, final):
        #  While it is queued or running qstat -j describes it; afterwards qacct does.
        text = self.record(jobid)
        if final:
            text = pollFor(lambda: sh(["qacct", "-j", str(jobid)])[1]
                           if sh(["qacct", "-j", str(jobid)])[0] == 0 else "", 40) or text
        for what, rx in (("queue " + prof["queue"], re.escape(prof["queue"])),
                         ("account " + prof["account"], re.escape(prof["account"])),
                         ("h_rt 30 minutes", r"h_rt=(1800|0:30:00|00:30:00)"),
                         ("mem_free 2000M", r"mem_free=(2000M|2000m|2\.0G|2G|2097152000)"),
                         ("parallel environment smp", r"\bsmp\b")):
            rep.check(re.search(rx, text) is not None,
                      "Grid Engine's record has %s" % what)
        if not final:
            say("      " + " | ".join(l.strip() for l in text.splitlines()
                                     if re.match(r"\s*(hard resource_list|account|hard_queue_list|"
                                                 r"parallel environment)", l)))

    def cancelled(self, rep, jobid):
        rc = pollFor(lambda: sh(["qacct", "-j", str(jobid)])[1]
                     if sh(["qacct", "-j", str(jobid)])[0] == 0 else "", 30)
        rep.check(bool(rc) and re.search(r"exit_status\s+(137|143)|failed\s+100", rc) is not None,
                  "qacct records the job as killed (exit_status 137, failed 100)")

    def drop(self, jobid):
        if self.state(jobid) in ("R", "Q"):
            sh(["qdel", str(jobid)])


class CondorSched(Sched):
    def usable(self):
        if not shutil.which("condor_submit") or not shutil.which("condor_q"):
            return "HTCondor client commands not installed"
        rc, out = sh(["condor_status", "-total"])
        if rc != 0 or "Total" not in out:
            return "no HTCondor pool answers condor_status"

    def ad(self, jobid):
        rc, out = sh(["condor_q", "-long", str(jobid)])
        text = out if rc == 0 and "=" in out else ""
        if not text:
            rc, out = sh(["condor_history", "-limit", "1", "-long", str(jobid)])
            text = out if rc == 0 and "=" in out else ""
        return dict((k.strip(), v.strip()) for k, v in
                    (l.split(" = ", 1) for l in text.splitlines() if " = " in l))

    def state(self, jobid):
        rc, out = sh(["condor_q", "-format", "%d\\n", "JobStatus", str(jobid)])
        st = out.split()[0] if rc == 0 and out.split() and out.split()[0].isdigit() else ""
        if st:
            return {"2": "R", "6": "R", "1": "Q", "5": "Q", "7": "Q"}.get(st, "done")
        return "done" if self.ad(jobid) else None

    def record(self, jobid):
        return "\n".join("%s = %s" % kv for kv in self.ad(jobid).items())

    def verify(self, rep, prof, name, jobdir, jobid, final):
        ad = pollFor(lambda: self.ad(jobid), 30)
        rep.check(bool(ad), "HTCondor has a job ad for cluster %s" % jobid)
        if final:
            ad = pollFor(lambda: (lambda a: a if a.get("JobStatus") in ("3", "4") else None)(
                self.ad(jobid)), 40) or ad
        rep.check(ad.get("RequestCpus") == "1", "request_cpus 1 (ad: %s)" % ad.get("RequestCpus"))
        rep.check(ad.get("RequestMemory") == "2000",
                  "request_memory 2000 (ad: %s)" % ad.get("RequestMemory"))
        rep.check(ad.get("Iwd", "").strip('"').rstrip("/") == jobdir.rstrip("/"),
                  "initialdir is the run directory (ad: %s)" % ad.get("Iwd"))
        rep.check(ad.get("Cmd", "").strip('"').endswith("submit__" + name),
                  "the executable is the ECCE script (ad: %s)" % ad.get("Cmd"))
        rep.check(ad.get("ProjectName", "").strip('"') == prof["account"],
                  "ProjectName %s (ad: %s)" % (prof["account"], ad.get("ProjectName")))
        rep.check("1800" in ad.get("PeriodicRemove", ""),
                  "wall time limit of 1800 s in periodic_remove")
        rep.check(ad.get("Owner", "").strip('"') == os.environ.get("USER", ""),
                  "job belongs to this user (ad: %s)" % ad.get("Owner"))

    def cancelled(self, rep, jobid):
        ad = pollFor(lambda: (lambda a: a if a.get("JobStatus") == "3" else None)(self.ad(jobid)), 30)
        rep.check(bool(ad), "HTCondor JobStatus is 3 (removed)")

    def drop(self, jobid):
        if self.state(jobid) in ("R", "Q"):
            sh(["condor_rm", str(jobid)])


class StubSched(Sched):
    """The stand-ins of tests/queues/stubsched.py, through their spool."""
    real = False

    def __init__(self, mgr, bindir):
        self.mgr, self.bin = mgr, bindir

    def dir(self, jobid):
        seq = re.match(r"\d+", str(jobid))
        return os.path.join(self.bin, "..", "spool", self.mgr, seq.group(0)) if seq else None

    def state(self, jobid):
        d = self.dir(jobid)
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

    def cancelled(self, rep, jobid):
        rep.check(os.path.exists(os.path.join(self.dir(jobid), "cancelled")),
                  "the stand-in's cancel command (%s) ended it" % {
                      "pbs": "qdel", "lsf": "bkill", "moab": "mjobctl -c"}[self.mgr])


def schedFor(mgr, stubdir):
    if mgr in STUB_MANAGERS:
        return StubSched(mgr, stubdir)
    return {"slurm": SlurmSched, "sge": SgeSched, "htcondor": CondorSched}[mgr]()


def realParsers(g, rep, args):
    """The real schedulers' own parsers on the generated descriptions."""
    for mgr in REAL_MANAGERS:
        if args.manager and mgr not in args.manager:
            continue
        sched = schedFor(mgr, None)
        if sched.usable():
            continue
        for prof in ("basic", "bare"):
            p = dict(PROFILES[prof])
            if p["queue"]:
                p["queue"] = LIVE_QUEUE[mgr]
            rep.row("golden", mgr, "-", "real-parser.%s" % prof)
            name = "t-" + prof
            #  condor_submit checks that initialdir exists, and not under /tmp.
            run = tempfile.mkdtemp(prefix="ecce-rundir-", dir=os.path.expanduser("~/.cache"))
            text = g.script(mgr, "nwchem", p, host="goldhost", name=name, rundir=run)
            path = os.path.join(g.tmp, "submit__" + name)
            if mgr == "slurm":
                rc, out = sh(["sbatch", "--test-only", path])
                what = "sbatch --test-only"
            elif mgr == "sge":
                rc, out = sh(["qsub", "-verify", path])
                what = "qsub -verify"
            else:
                #  Through the submit command's own extraction of the description.
                sub = path + ".sub"
                with open(sub, "w") as h:
                    h.write("".join(l[len("#CONDOR "):] + "\n" for l in text.splitlines()
                                    if l.startswith("#CONDOR ")).replace("@SCRIPT@", path))
                rc, out = sh(["condor_submit", "-dry-run", "/dev/null", sub])
                what = "condor_submit -dry-run"
            rep.check(rc == 0, "%s accepts the generated description: %s" % (
                what, " ".join(out.split())[:110]))
            shutil.rmtree(run, ignore_errors=True)
            rep.done()
    if not args.manager or "htcondor" in args.manager:
        rep.row("golden", "htcondor", "-", "tmp-rundir")
        try:
            g.script("htcondor", "nwchem", PROFILES["basic"], rundir="/tmp/qtest")
            rep.check(False, "gensub accepted a run directory under /tmp")
        except RuntimeError as exc:
            rep.check("/tmp" in str(exc) and "home" in str(exc),
                      "gensub refuses a run directory under /tmp, saying why")
        rep.done()


# --- live jobs ----------------------------------------------------------

class Live(object):
    def __init__(self, s, rep, args, suite, stubdir=None):
        self.s, self.rep, self.args, self.suite = s, rep, args, suite
        self.stub = stubdir
        self.scheds = {}
        self.submitted = []          # (sched, id) to clean up
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

    def sched(self, mgr):
        if mgr not in self.scheds:
            self.scheds[mgr] = schedFor(mgr, self.stub)
        return self.scheds[mgr]

    def submittedScript(self, mgr, jobid, rundir, name):
        sched = self.sched(mgr)
        path = (os.path.join(sched.dir(jobid), "script") if not sched.real
                else os.path.join(rundir, "submit__" + name) if rundir else None)
        if path and os.path.exists(path):
            with open(path) as h:
                return h.read()
        return None

    def run(self, mgr, code, transport, kill=False):
        mname, prefix, machine = MANAGERS[mgr]
        rep, s = self.rep, self.s
        sched = self.sched(mgr)
        kind = "kill" if kill else "run"
        job = "%s-%s" % (code, kind)
        rep.row(self.suite, mgr, transport, job)
        say("--- %s %s %s (ECCE_TRANSPORT=%s)" % (mgr, code, kind, transport))
        s.transport = None if transport == "unset" else transport
        prof = dict(PROFILES["basic"])
        prof["queue"] = LIVE_QUEUE.get(mgr, prof["queue"])
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
                "yet returned 'Outputs' instead of the declared output name (c610bf6c's "
                "imported-output fallback); the job writes that name where the monitor "
                "expects the declared one, and MOPAC loses the properties only the live "
                "monitor extracts (TE, GEOMTRACE)")
        ran = [l.split(":", 1)[1].strip() for l in out.splitlines()
               if l.startswith("run directory:")]
        jobdir = ran[-1] if ran else None
        if not rep.check(rc == 0, "Launch submitted the job"):
            say("\n".join("      | " + l for l in out.strip().splitlines()[-8:]))
            rep.done()
            return
        jobid = s.driver("jobid", url)[1].strip().splitlines()[-1]
        self.submitted.append((sched, jobid))
        rep.note("id " + jobid)
        idre = {"pbs": r"\d+\.stubserver"}.get(mgr, r"\d+")
        rep.check(re.fullmatch(idre, jobid) is not None,
                  "parsed job id %r has the form %s" % (jobid, idre))
        known = pollFor(lambda: sched.state(jobid), 15, 1)
        rep.check(known is not None, "the scheduler knows job %s" % jobid)

        if not sched.real:
            #  Proof that it was the stand-in, not a scheduler of this host, that ran.
            cmd = {"pbs": "qsub", "lsf": "bsub", "moab": "msub"}[mgr]
            log = os.path.join(self.stub, "..", "spool", mgr, "commands.log")
            text = open(log).read() if os.path.exists(log) else ""
            spooled = self.submittedScript(mgr, jobid, jobdir, name) or ""
            rep.check(re.search(r"^%s\b" % cmd, text, re.M) is not None and name in spooled,
                      "the stand-in %s recorded this submission" % cmd)

        if sched.real:
            sched.verify(rep, prof, name, jobdir, jobid, final=False)
            say("      scheduler state: %s" % sched.state(jobid))

        script = self.submittedScript(mgr, jobid, jobdir, name)
        if rep.check(script is not None, "the submitted script was captured"):
            if prefix:
                want = expectedDirectives(mgr, prof, name[:14], jobdir)
                got = directiveLines(script, mgr)
                rep.check(got == want, "script directives as expected" + (
                    "" if got == want else ": got %r want %r" % (got, want)))
                for prob in wellFormed(script, mgr):
                    rep.check(False, prob)
            if mgr == "htcondor" and jobdir:
                sub = os.path.join(jobdir, "submit__%s.sub" % name)
                rep.check(os.path.exists(sub), "the submit command wrote %s" % os.path.basename(sub))
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
                deadline = time.time() + (0 if outbug and code == "mopac" else 40)
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
            after = sched.state(jobid)
            rep.check(after in ("done", None), "the scheduler no longer runs the job (%s)" % after)
            if sched.real and state == "completed":
                #  The scheduler's own account of how it ended.
                sched.verify(rep, prof, name, jobdir, jobid, final=True)
        if rep.cur["fails"]:
            self.preserve(name, jobdir)
        rep.done()

    def preserve(self, name, jobdir):
        """Keep a failed job's run directory and monitor logs past the next run."""
        dest = os.path.join(self.s.state, "failed", name)
        shutil.rmtree(dest, ignore_errors=True)
        os.makedirs(dest)
        if jobdir and os.path.isdir(jobdir):
            shutil.copytree(jobdir, os.path.join(dest, "rundir"), symlinks=True,
                            ignore_dangling_symlinks=True)
        for d in glob.glob(os.path.join(self.s.state, "tmp", "*", "jobs", "*" + name + "*")):
            shutil.copytree(d, os.path.join(dest, "ecce-" + os.path.basename(d)),
                            symlinks=True, ignore_dangling_symlinks=True)
        say("      kept for inspection: " + dest)

    def cancel(self, mgr, url, jobid, jobdir):
        rep, s = self.rep, self.s
        sched = self.sched(mgr)
        #  The job is held in the wrapper's sleep; wait until the scheduler runs it.
        running = pollFor(lambda: sched.state(jobid) == "R", 120, 1)
        rep.check(running, "the job is running under the scheduler")
        time.sleep(3)
        rc, out = s.driver("kill", url)
        msg = out.strip().splitlines()[-1] if out.strip() else ""
        rep.check(rc == 0 and "has been issued" in msg, "RunMgmt::terminate: %s" % msg)
        ended = pollFor(lambda: sched.state(jobid) != "R", 40, 1)
        rep.check(ended, "the scheduler job ended after the cancel command (%s)" % sched.state(jobid))
        sched.cancelled(rep, jobid)
        time.sleep(2)
        left = subprocess.run(["pgrep", "-f", self.wrapper], stdout=subprocess.PIPE)
        rep.check(left.returncode != 0,
                  "no compute process left (%s)" % left.stdout.decode().split())
        state = s.waitState(url, 60, want=("killed", "completed", "failed", "unsuccessful"))
        rep.check(state == "killed", "calculation state is killed (last: %s)" % state)

    def cleanup(self):
        for sched, jobid in self.submitted:
            if sched.real:
                sched.drop(jobid)


def liveSuite(args, rep, suite, managers, stubdir=None):
    build = os.path.abspath(args.build)
    harness.prerequisites(build, ("nwchem", "mopac"))
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
                          queues=(LIVE_QUEUE.get(m, "normal"),),
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
    root = os.environ.get("ECCE_TEST_STUBS") or os.path.join(
        os.path.expanduser("~"), ".cache", "ecce-queue-stubs")
    shutil.rmtree(root, ignore_errors=True)
    bindir = os.path.join(root, "bin")
    os.makedirs(bindir)
    for name in STUB_LINKS:
        os.symlink(os.path.join(HERE, "stubsched.py"), os.path.join(bindir, name))
    say("STAND-IN schedulers (tests/queues/stubsched.py, NOT the real PBS/LSF/Moab) "
        "on PATH: " + bindir)
    spool = os.path.join(root, "spool")
    os.makedirs(spool, exist_ok=True)
    if args.no_csh:
        open(os.path.join(spool, "no-csh"), "w").close()
        say("jobs run with csh and tcsh made unusable (bwrap)")
    saved = os.environ["PATH"]
    os.environ["PATH"] = bindir + ":" + saved
    try:
        liveSuite(args, rep, "stubs", managers, stubdir=bindir)
        if args.no_csh:
            probes = glob.glob(os.path.join(spool, "*", "*", "csh-probe"))
            rep.row("stubs", "-", "-", "no-csh.probe")
            rep.check(probes and all(open(f).read().strip() != "0" for f in probes),
                      "csh was unusable in all %d stand-in jobs" % len(probes))
            rep.done()
    finally:
        os.environ["PATH"] = saved


def realSuite(mgr):
    def suite(args, rep):
        if args.manager and mgr not in args.manager:
            return
        why = schedFor(mgr, None).usable()
        if why:
            harness.skip("%s: %s" % (mgr, why))
        liveSuite(args, rep, mgr, [mgr])
    return suite


SUITES = [("golden", goldenSuite), ("stubs", stubsSuite)] + \
         [(m, realSuite(m)) for m in REAL_MANAGERS]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build"))
    ap.add_argument("--suite", default="all",
                    choices=["all", "local", "golden", "stubs"] + list(REAL_MANAGERS))
    ap.add_argument("--transport", default="both",
                    choices=("unset", "direct", "ssh", "both"))
    ap.add_argument("--manager", action="append", choices=sorted(MANAGERS))
    ap.add_argument("--code", action="append", choices=("mopac", "nwchem"),
                    help="only this code (repeatable)")
    ap.add_argument("--no-kill", action="store_true", help="skip the cancel jobs")
    ap.add_argument("--repeat", type=int, default=1, help="repeat each live job N times")
    ap.add_argument("--update-golden", action="store_true")
    ap.add_argument("--no-csh", action="store_true",
                    help="stand-in scheduler jobs run with csh/tcsh unusable")
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
    for suite, fn in SUITES:
        real = suite in REAL_MANAGERS
        if args.suite not in ("all", suite) and not (args.suite == "local" and not real):
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
