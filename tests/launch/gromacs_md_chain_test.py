#!/usr/bin/env python3
"""A GROMACS study chain: where a task starts from, and when it may launch (#735).

Equilibrate has a structure attached on its Inputs page (named so that the
trailing-digit pick would prefer it) and Dynamics, resume on, has nothing
attached.

  * Launch Selected's plan refuses Dynamics ("waits for equilibrate") until
    Equilibrate is Complete; a single launch warns and stays cancellable;
  * after Equilibrate ran, Dynamics' staged conf.gro is Equilibrate's final
    .gro (md5), not the attachment.

    tests/launch/gromacs_md_chain_test.py [--build build-cmake] [--keep]

Exit status 77 (CTest SKIP) without gmx or the built binaries.
"""
import argparse
import glob
import hashlib
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402

FIXTURES = os.path.join(REPO, "tests", "gromacs_md", "fixtures")
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def md5(path):
    return hashlib.md5(open(path, "rb").read()).hexdigest()


def run_dir_of(rundir, marker):
    found = sorted(glob.glob(os.path.join(rundir, "**", marker), recursive=True),
                   key=os.path.getmtime)
    return os.path.dirname(found[-1]) if found else None


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    gmx = shutil.which("gmx")
    if not gmx:
        harness.skip("gmx is not installed")
    harness.prerequisites(build, ())
    s = harness.Session(build, "gmxmdchain", {"GROMACS": gmx}, local=True,
                        keep=args.keep)
    try:
        s.registerMachine("localhost", "Shell", {"GROMACS": gmx})
        if not check(s.services(True), "services came up"):
            return 1
        rc, out = s.driver("gromacsstudy", s.userUrl(), "chain")
        urls = [l for l in out.splitlines() if l.startswith("file://")]
        if not check(rc == 0 and len(urls) == 4, "a GROMACS study was created"):
            return 1
        study, opt, eq, dyn = [u.rstrip("/") + "/" for u in urls]
        rundir = os.path.join(s.state, "run")
        for u in (eq, dyn):
            rc, out = s.driver("mdsetup", u, "localhost", rundir, s.user(), "queue=normal")
            check(rc == 0, "launch settings stored for %s" % u.rstrip("/").rsplit("/", 1)[-1])
        attached = os.path.join(FIXTURES, "conf.gro")
        # a trailing number in the base name is what the old pick preferred
        rc, _ = s.driver("mdfile", eq, attached, "start_100.gro")
        check(rc == 0, "structure attached to Equilibrate")
        rc, _ = s.driver("mdfile", eq, os.path.join(FIXTURES, "topol.top"), "topol.gmxtop")
        check(rc == 0, "topology attached to Equilibrate")
        check(s.driver("mdmdp", eq)[0] == 0, "Equilibrate's .mdp written")
        check(s.driver("mdmdp", dyn, "resume")[0] == 0, "Dynamics' .mdp written, resume on")

        rc, out = s.driver("plan", eq)
        check(out.strip() == "ready", "Equilibrate may launch: %s" % out.strip())
        rc, out = s.driver("plan", dyn)
        check(out.strip() == "waits for equilibrate",
              "Launch Selected holds Dynamics back: %s" % out.strip())
        rc, out = s.driver("launch", dyn, extra={"LAUNCHJOB_WARNING": "cancel"})
        check(rc != 0 and "launch warning:" in out and "equilibrate" in out
              and s.state_of(dyn) == "ready",
              "a single launch warns, and cancelling leaves it ready")

        rc, out = s.launch(eq)
        check(rc == 0, "Equilibrate submitted")
        state = s.waitState(eq, 300)
        check(state in ("completed", "loaded"), "Equilibrate ran to %s" % state)
        etask = eq.replace("file://", "").rstrip("/")
        out_gro = os.path.join(etask, "Outputs", "ecceSession_ecceMdEquilibrate.gro")
        if not check(os.path.exists(out_gro), "Equilibrate's final structure was stored"):
            return 1
        d = run_dir_of(rundir, "ecceSession_ecceMdEquilibrate.gro")
        check(d and md5(os.path.join(d, "conf.gro")) == md5(attached),
              "Equilibrate started from its attached structure")
        check(md5(out_gro) != md5(attached), "and ended somewhere else")

        rc, out = s.driver("plan", dyn)
        check(out.strip() == "ready", "Dynamics may launch now: %s" % out.strip())
        rc, out = s.launch(dyn)
        check(rc == 0, "Dynamics submitted: %s" % out.strip().splitlines()[-2:])
        state = s.waitState(dyn, 300)
        check(state in ("completed", "loaded"), "Dynamics ran to %s" % state)
        d = run_dir_of(rundir, "ecceSession_ecceMdDynamics.gro")
        if check(d is not None, "Dynamics' run directory found"):
            started = md5(os.path.join(d, "conf.gro"))
            check(started == md5(out_gro),
                  "Dynamics started from Equilibrate's final structure")
            check(started != md5(attached), "not from the attached one")
    finally:
        s.stop()
    say("FAILED: %s" % failures if failures else "OK")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
