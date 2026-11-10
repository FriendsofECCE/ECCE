#!/usr/bin/env python3
"""#924: the Organizer's start-up sweep (launchjob sweeptmp) removes job
folders and ECCE?????? temp files older than three days, and keeps fresh
ones, folders a live process still uses, and anything not named as
TempStorage names its folders and files."""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OLD = time.time() - 4 * 24 * 3600
failures = []


def check(ok, what):
    print(("ok   " if ok else "FAIL ") + what)
    if not ok:
        failures.append(what)


def make(path, old, isdir=True):
    if isdir:
        os.makedirs(path)
        with open(os.path.join(path, "eccejobstore.log"), "w") as h:
            h.write("log\n")
        os.utime(os.path.join(path, "eccejobstore.log"), (OLD, OLD) if old else None)
    else:
        with open(path, "w") as h:
            h.write("x\n")
    if old:
        os.utime(path, (OLD, OLD))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", required=True)
    opts = ap.parse_args()
    exe = os.path.join(os.path.abspath(opts.build), "launchjob")
    if not os.access(exe, os.X_OK):
        print("launchjob is not built")
        return 77
    top = tempfile.mkdtemp(prefix="ecce-tmpsweep-")
    busy = None
    try:
        env = dict(os.environ, ECCE_HOME=REPO, ECCE_TMPDIR=os.path.join(top, "tmp"),
                   HOME=os.path.join(top, "home"), ECCE_REALUSER="sweeptest",
                   ECCE_REALUSERHOME=os.path.join(top, "home"))
        os.makedirs(env["HOME"])
        root = os.path.join(top, "tmp", "ecce_sweeptest")
        jobs = os.path.join(root, "jobs")
        os.makedirs(jobs)
        outside = os.path.join(top, "user-output.out")
        make(outside, True, isdir=False)
        make(os.path.join(jobs, "h2o__Ab12Cd"), True)
        link = os.path.join(jobs, "h2o__Ab12Cd", "h2o.out")
        os.symlink(outside, link)
        # The link and the entry it adds would make the folder young.
        os.utime(link, (OLD, OLD), follow_symlinks=False)
        os.utime(os.path.join(jobs, "h2o__Ab12Cd"), (OLD, OLD))
        make(os.path.join(jobs, "dynamics__Lk78Mn"), True)
        make(os.path.join(jobs, "fresh__Xy34Zw"), False)
        make(os.path.join(jobs, "busy__Qw56Er"), True)
        make(os.path.join(jobs, "Examples"), True)
        make(os.path.join(root, "ECCEab12cd"), True, isdir=False)
        make(os.path.join(root, "ECCEzz99yy"), False, isdir=False)
        make(os.path.join(root, "ECCEverifyab12cd"), True, isdir=False)
        if sys.platform.startswith("linux"):
            busy = subprocess.Popen(["sleep", "120"], cwd=os.path.join(jobs, "busy__Qw56Er"))
            os.utime(os.path.join(jobs, "busy__Qw56Er"), (OLD, OLD))

        run = subprocess.run([exe, "sweeptmp"], env=env, capture_output=True,
                             text=True, timeout=120)
        print(run.stdout + run.stderr)
        check(run.returncode == 0, "launchjob sweeptmp ran (rc=%d)" % run.returncode)
        check(not os.path.exists(os.path.join(jobs, "h2o__Ab12Cd")),
              "an old job folder is removed")
        check(not os.path.exists(os.path.join(jobs, "dynamics__Lk78Mn")),
              "an old import folder is removed")
        check(os.path.exists(outside), "the file an import folder linked to is kept")
        check(os.path.isdir(os.path.join(jobs, "fresh__Xy34Zw")),
              "a fresh job folder is kept")
        check(os.path.isdir(os.path.join(jobs, "Examples")),
              "a folder not named by TempStorage is kept")
        if busy:
            check(os.path.isdir(os.path.join(jobs, "busy__Qw56Er")),
                  "an old folder a live process works in is kept")
        check(not os.path.exists(os.path.join(root, "ECCEab12cd")),
              "an old ECCE?????? temp file is removed")
        check(os.path.exists(os.path.join(root, "ECCEzz99yy")),
              "a fresh ECCE?????? temp file is kept")
        check(os.path.exists(os.path.join(root, "ECCEverifyab12cd")),
              "an old file with another name is kept")
        check(run.stdout.strip().splitlines()[-1:] == ["3"] if run.stdout.strip() else False,
              "three entries removed (%s)" % run.stdout.strip())
    finally:
        if busy:
            busy.kill()
            busy.wait()
        shutil.rmtree(top, ignore_errors=True)
    print("FAILED (%d)" % len(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
