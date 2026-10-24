#!/usr/bin/env python3
"""The pre-parsed example calculations agree with a parse of the outputs (#448).

    tests/apps/examples_preparsed_test.py

Per file-output example set (general, organic, spectroscopy), two imports in a
fresh state each: one with
ECCE_NO_PREPARSED=1 (the output parsers run), one as shipped (the stored
copies under data/client/examples/<set>/<example>/stored are copied).  Every
file of every example calculation must be the same both ways, and every stored
property too, except dates, the user, the machine and job directories.  The
copying arm must really have copied all five, so a stale copy that quietly
falls back to the parse fails here, as does one that no longer matches.
Prints the time of both arms.  Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SKIP = 77
# set folder -> (project it is imported into, number of calculations)
SETS = {"general": ("Examples", 5), "organic": ("Examples_Organic", 17),
        "spectroscopy": ("Examples_Spectroscopy", 10)}

# Written per import: who ran it, when, and where (StoredCalcCopy's kSkip).
VOLATILE = {"owner", "creationdate", "createdWith", "currentVersion",
            "startdate", "completiondate", "job_path", "job_jobid",
            "job_clienthost", "job_clientid", "launch_user", "launch_machine",
            "launch_remoteShell", "launch_rundir", "version"}
# The progress record carries the clock of the run it came from.
# seq counts the monitor's messages, which depends on timing, not content.
CLOCK = re.compile(r"(seq|sent|at)=[0-9.]+")
IGNORED_FILES = re.compile(r"(^|/)(Thumbnail[^/]*|[^/]*\.lock)$")
DATE = re.compile(r"\b(Mon|Tue|Wed|Thu|Fri|Sat|Sun), \d\d \w{3} \d{4} "
                  r"\d\d:\d\d:\d\d GMT")


def runArm(out, noPreparsed, setId):
    env = dict(os.environ)
    env.pop("ECCE_NO_PREPARSED", None)
    if noPreparsed:
        env["ECCE_NO_PREPARSED"] = "1"
    project, count = SETS[setId]
    proc = subprocess.run([sys.executable, os.path.join(HERE, "examples_arm.py"),
                           out, setId, project, str(count)], env=env, capture_output=True, text=True)
    sys.stdout.write(proc.stdout[-1500:] if proc.returncode else "")
    m = re.search(r"^ARM (\S+) (\S+) (\d+) (\d+)$", proc.stdout, re.M)
    return proc.returncode, m, proc.stdout


def records(path):
    """{(resource, property): value} of a sidecar, minus the volatile ones."""
    found = {}
    with open(path, errors="replace") as handle:
        for line in handle:
            parts = line.rstrip("\n").split("\t", 3)
            if len(parts) != 4:
                continue
            name = parts[1].rsplit(":", 1)[-1]
            if name in VOLATILE:
                continue
            found[(parts[0], parts[1])] = CLOCK.sub(r"\1=<t>",
                                                    DATE.sub("<date>", parts[3]))
    return found


def files(root):
    found = set()
    for base, _dirs, names in os.walk(root):
        for n in names:
            rel = os.path.relpath(os.path.join(base, n), root)
            if not IGNORED_FILES.search(rel):
                found.add(rel)
    return found


def compare(parsed, copied):
    problems = []
    a, b = files(parsed), files(copied)
    for rel in sorted(a - b):
        problems.append("missing in the copy: " + rel)
    for rel in sorted(b - a):
        problems.append("only in the copy: " + rel)
    for rel in sorted(a & b):
        pa, pb = os.path.join(parsed, rel), os.path.join(copied, rel)
        if os.path.basename(rel) == ".ecce-meta":
            ra, rb = records(pa), records(pb)
            for key in sorted(set(ra) | set(rb)):
                if ra.get(key) != rb.get(key):
                    problems.append("%s %s: parse %r, copy %r" % (
                        rel, key[1].rsplit(":", 1)[-1],
                        (ra.get(key) or "-")[:60], (rb.get(key) or "-")[:60]))
        elif os.path.basename(rel) == "eccejobstorelog.ecce_run_log":
            continue    # a log of this run, with its own times
        else:
            with open(pa, "rb") as x, open(pb, "rb") as y:
                da, db = x.read(), y.read()
            if da != db and DATE.sub(b"", da) != DATE.sub(b"", db):
                problems.append("%s differs (%d and %d bytes)"
                                % (rel, len(da), len(db)))
    return problems


def checkSet(work, setId, failures):
    project, count = SETS[setId]
    arms = {}
    for label, no in (("parse", True), ("copy", False)):
        rc, m, out = runArm(os.path.join(work, setId, label), no, setId)
        if rc == SKIP:
            print(out.strip())
            return SKIP
        if rc != 0 or not m:
            print("FAIL  %s %s arm: exit %d\n%s" % (setId, label, rc, out[-1500:]))
            failures.append("%s %s arm" % (setId, label))
            return 1
        arms[label] = m
        print("%-12s %-5s logged %s s, last property file after %s s; "
              "%s copied, %s parsed" % (setId, label, m.group(1), m.group(2),
                                        m.group(3), m.group(4)))
    if arms["copy"].group(3) != str(count) or arms["parse"].group(3) != "0":
        failures.append("%s copied/parsed counts: copy arm %s/%s, parse arm "
                        "%s/%s" % (setId, arms["copy"].group(3),
                                   arms["copy"].group(4),
                                   arms["parse"].group(3),
                                   arms["parse"].group(4)))
    pdir = os.path.join(work, setId, "parse", project)
    cdir = os.path.join(work, setId, "copy", project)
    if sorted(os.listdir(pdir)) != sorted(os.listdir(cdir)):
        failures.append("%s calculations differ: %s and %s"
                        % (setId, sorted(os.listdir(pdir)),
                           sorted(os.listdir(cdir))))
        return 0
    for calc in sorted(os.listdir(pdir)):
        if calc.startswith("."):
            continue
        problems = compare(os.path.join(pdir, calc), os.path.join(cdir, calc))
        if problems:
            failures.append("%s/%s: %d differences" % (setId, calc, len(problems)))
            print("FAIL  %s/%s\n      %s" % (setId, calc, "\n      ".join(
                problems[:25])))
        else:
            print("ok    %s/%s: copy and parse agree (%d files)"
                  % (setId, calc, len(files(os.path.join(pdir, calc)))))
    return 0


def main():
    work = tempfile.mkdtemp(prefix="examples-preparsed-",
                            dir=os.path.expanduser("~/.cache"))
    failures = []
    try:
        for setId in (sys.argv[1:] or SETS):
            rc = checkSet(work, setId, failures)
            if rc == SKIP:
                return SKIP
            if rc:
                break
    finally:
        if os.environ.get("ECCE_TEST_KEEP"):
            print("kept " + work)
        else:
            shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
