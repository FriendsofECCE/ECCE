#!/usr/bin/env python3
"""
Excluded calculations (#617), headless.

A stored calculation is marked excluded with a reason (ecce:excluded in its
.ecce-meta).  The ECCE_TEST_ORGANIZER hooks "exclude"/"include"/"treetip" run
what ecce-annotate --exclude/--include do; the GUI does not show the mark
at all (#805), so the tree item stays plain throughout and neither the project
table nor the summary has a row style or an Excluded line.  The
CLI (ecce-annotate --exclude/--include, no session) must write the same
property, ecce-si-export must skip an excluded calculation unless given
--include-excluded and carry an "excluded" column, and Launch Ready Steps
(workflowready) must not offer an excluded step.  Exit 77 (skip) without an
install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data", "project.ecce-meta")
CLI = os.path.join(ROOT, "scripts", "ecce_si_export.py")
SRC = os.path.join(fixture.FIXTURES, "calc-water-opt")
REASON = "SCF converged to neutral Na"
failures = []


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + str(detail)) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def meta(path):
    p = os.path.join(path, ".ecce-meta")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-excl-data-")
    work = tempfile.mkdtemp(prefix="ecce-excl-")
    try:
        root = os.path.join(data, "users", "local")
        project = os.path.join(root, "excl")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "water")
        shutil.copytree(SRC, calc, symlinks=True)
        other = os.path.join(project, "water2")
        shutil.copytree(SRC, other, symlinks=True)
        u, wf = "file://" + calc, "file://" + project + "/Workflow"
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as h:
            h.write("treetip %s\n" % u)
            h.write("exclude %s %s\n" % (u, REASON))
            h.write("treetip %s\n" % u)
            h.write("summary file://%s\n" % project)
            h.write("summary %s\n" % u)
            h.write("include %s\n" % u)
            h.write("summary file://%s\n" % project)
            h.write("treetip %s\n" % u)
            # workflow: a step that is excluded is not ready
            h.write("new file://%s workflow\n" % project)
            h.write("workflowcopy %s file://%s/water2 src\n" % (wf, project))
            h.write("workflowstep %s orca_es a %s/src\n" % (wf, wf))
            h.write("workflowstep %s orca_es b %s/src\n" % (wf, wf))
            h.write("workflowready %s\n" % wf)
            h.write("exclude %s/b not wanted\n" % wf)
            h.write("workflowready %s\n" % wf)
        result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + project,
                               "ECCE_LOCAL_DATA": data})
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return

        def ans(word):
            return re.findall(r"ECCE_TEST_ORGANIZER: %s .*?: (.*)$" % word, log, re.M)
        tips = ans("treetip")
        if not check(len(tips) == 3, "the hooks answered", log[-1500:]):
            return
        check(all("strike=0" in t and "grey=0" in t for t in tips),
              "the tree item is never struck through or grey", tips)
        # The meta holds the reason after the exclude and nothing after include.
        check("excluded" not in meta(calc) or not re.search(r"excluded\t\t\S", meta(calc)),
              "include cleared the stored reason")

        ready = ans("workflowready")
        check(len(ready) == 2 and " a" in ready[0] and " b" in ready[0],
              "both steps ready before the exclude", ready)
        check(len(ready) == 2 and " a" in ready[1] and not re.search(r"\bb\b", ready[1]),
              "an excluded step is not ready", ready)
        check("not wanted" in meta(project + "/Workflow/b") or
              "not wanted" in open(os.path.join(project, "Workflow", "b", ".ecce-meta")).read(),
              "the reason is in the step's .ecce-meta")

        # CLI, no session: same property, written through the data layer.
        home = tempfile.mkdtemp(prefix="ecce-excl-home-")
        env = dict(os.environ, ECCE_HOME=apps.INSTALL, ECCE_LOCAL_DATA=data,
                   ECCE_REALUSERHOME=home, HOME=home,
                   ECCE_SESSION_ID="0123456789abcdef")
        cli = os.path.join(apps.INSTALL, "bin", "ecce-annotate")
        try:
            r = subprocess.run([cli, "--exclude", calc, REASON], env=env,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
            check(r.returncode == 0 and b": excluded" in r.stdout and
                  REASON in meta(calc), "ecce-annotate --exclude stores the reason",
                  (r.returncode, r.stdout, r.stderr))
            out = os.path.join(work, "a.xyz")
            subprocess.run([sys.executable, CLI, calc, other, "-o", out, "--table", "csv"],
                           check=True)
            xyz = open(out).read()
            csv = open(os.path.join(work, "a.csv")).read().splitlines()
            check("water2" in xyz and "water  " not in xyz and len(csv) == 2 and
                  csv[0].endswith(",excluded"), "ecce-si-export skips the excluded one",
                  (xyz[:200], csv))
            subprocess.run([sys.executable, CLI, calc, other, "-o", out, "--table", "csv",
                            "--include-excluded"], check=True)
            csv = open(os.path.join(work, "a.csv")).read().splitlines()
            check(len(csv) == 3 and csv[1].endswith(REASON.replace(" ", " ")) or
                  REASON in csv[1] or REASON in csv[2],
                  "--include-excluded lists it with its reason", csv)
            r = subprocess.run([cli, "--include", calc], env=env,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
            check(r.returncode == 0 and b": included" in r.stdout and
                  REASON not in meta(calc), "ecce-annotate --include clears it",
                  (r.returncode, r.stdout, r.stderr))
        finally:
            shutil.rmtree(home, ignore_errors=True)
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("exclude", "exclude wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
