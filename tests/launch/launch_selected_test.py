#!/usr/bin/env python3
"""Organizer > Run Mgmt > Launch Selected (#492): several ready calculations
are submitted in one go with the launch settings each stores.

Three CP2K calculations (the code is /bin/true) and one with no machine are
made in a local data folder; the Organizer hook `launchselected URL...` runs
the batch without its dialogs, and `launchselectedshot PNG URL...` saves the
summary dialog.  Every complete calculation must leave "ready" through the
real Launch, and the one without settings must be skipped.

    tests/launch/launch_selected_test.py [--build DIR] [--keep] [--shot PNG]

Exit status 77 (CTest SKIP) without Xvfb or the built binaries.
"""

import argparse
import os
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


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--shot", default="")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "launchsel", {"CP2K": "/bin/true"},
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
    s.registerMachine("localhost", "Shell", {"CP2K": "/bin/true"})
    #  Launch on (#492): "small" cuts cores and wall time and has no "batch"
    #  queue (a Shell manager lists no queues, so Slurm, staged: the submit
    #  stops after the settings are stored); "orcaonly" does not offer CP2K.
    s.registerMachine("small", "Slurm", {"CP2K": "/bin/true"}, queues=("normal",),
                      limits={"normal": (1, 12, 60)}, size="16:1",
                      config={"userSubmit": "true"})
    s.registerMachine("orcaonly", "Shell", {"ORCA": "/bin/true"})
    runsmall = os.path.join(s.state, "run-small")
    runproj = os.path.join(s.state, "run-project")
    with open(os.path.join(s.state, ".ECCE", "MachPrefs"), "w") as h:
        for m, rd in (("small", runsmall), ("orcaonly", runsmall)):
            h.write("%s:\t%s|ssh|normal|%s|%s|acct-1|1|1|60|0|0|1\n"
                    % (m, s.user(), rd, rd))
    with open(os.path.join(s.state, ".ECCE", "MachProfiles"), "w") as h:
        h.write("profile\tsmall\tProject|acct-2|%s|%s|normal\n" % (runproj, runproj))
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    os.makedirs(work, exist_ok=True)
    os.makedirs(rundir, exist_ok=True)
    deck = os.path.join(work, "a.cp2kin")
    open(deck, "w").write("&GLOBAL\n  RUN_TYPE ENERGY\n&END GLOBAL\n")

    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        urls = []
        for n in range(3):
            url, out = s.create("sel%d" % n, "cp2k_es", deck, "cp2k.cp2kin",
                                rundir, extra=("procs=1", "queue=normal",
                                               "nodes=1", "wall=0 1:0"))
            if not check(url is not None, "calculation %d created" % n):
                say(out)
                return 1
            urls.append(url)
        #  A direct (Shell) machine takes no wall time: not skipped for lack of one.
        nowall, out = s.create("nowall", "cp2k_es", deck, "cp2k.cp2kin",
                               rundir, extra=("procs=1", "queue=normal",
                                              "nodes=1"))
        check(nowall is not None, "calculation without a wall time created")
        #  No machine stored: listed, skipped.
        bad, out = s.create("nomachine", "cp2k_es", deck, "cp2k.cp2kin",
                            rundir, machine="", extra=("procs=1",))
        say("  nomachine: %s" % (bad or out.strip()[-120:]))
        #  A run directory that cannot be made: the submit fails (#516).
        badrun = os.path.join(s.state, "notadir")
        open(badrun, "w").write("a file, not a directory\n")
        failing, out = s.create("failsub", "cp2k_es", deck, "cp2k.cp2kin",
                                os.path.join(badrun, "sub"),
                                extra=("procs=1", "queue=normal", "nodes=1"))
        check(failing is not None, "calculation with an unusable run directory created")
        #  Stored for another machine: a queue it lacks, more cores than it has.
        cut, out = s.create("cut", "cp2k_es", deck, "cp2k.cp2kin", rundir,
                            extra=("procs=48", "queue=batch", "nodes=2",
                                   "wall=0 4:0", "mem=8192"))
        check(cut is not None, "calculation to retarget created")
        asyncs = []
        for n in range(3):
            u, out = s.create("async%d" % n, "cp2k_es", deck, "cp2k.cp2kin",
                              rundir, extra=("procs=1", "queue=normal",
                                             "nodes=1", "wall=0 1:0"))
            check(u is not None, "calculation async%d created" % n)
            asyncs.append(u)
        states = [s.state_of(u) for u in urls]
        check(all(x == "ready" for x in states), "all start ready: %s" % states)

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")],
            env=dict(genv, ECCE_TEST_ORGANIZER=cmds),
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        def send(line, timeout=300):
            seen = len(open(olog, errors="replace").read().splitlines())
            with open(cmds, "a") as h:
                h.write(line + "\n")
            want = "ECCE_TEST_ORGANIZER: %s: " % line
            end = time.time() + timeout
            while time.time() < end:
                if org.poll() is not None:
                    return "the Organizer ended (%s)" % org.returncode
                for t in open(olog, errors="replace").read().splitlines()[seen:]:
                    if t.startswith(want):
                        return t[len(want):]
                time.sleep(0.5)
            return "no answer"

        shot = args.shot or os.path.join(s.state, "summary.png")
        every = urls + ([failing] if failing else []) + ([nowall] if nowall else []) + ([bad] if bad else [])
        #  Several table rows selected, focus still on the tree: all are used.
        #  Each calculation sits in its own project, so the rows of the user
        #  folder's table (the projects) stand in for the rows of one table.
        rows = [u.rsplit("/", 1)[0] for u in urls]
        ans = send("launchselectionrows %s" % " ".join(rows))
        check(ans.startswith("ok %d " % len(rows)) and
              all(u in ans for u in rows),
              "all table rows are the Launch Selected set (%s)" % ans[:80])
        ans = send("launchselectedshot %s %s" % (shot, " ".join(every)))
        check(ans == "not launched" and os.path.getsize(shot) > 1000,
              "summary dialog saved (%s)" % ans)
        check(all(s.state_of(u) == "ready" for u in urls),
              "the summary alone submits nothing")

        #  Launch on: summary for another machine, then the real thing.
        shot2 = os.path.splitext(shot)[0] + "-on.png"
        ans = send("launchselectedshot %s on=small %s %s" % (shot2, cut, " ".join(every)))
        check(ans == "not launched" and os.path.getsize(shot2) > 1000,
              "summary dialog for another machine saved (%s)" % ans)
        ans = send("launchselected on=orcaonly %s" % urls[0])
        say("  answer: %s" % ans)
        check("sel0: skipped: code CP2K is not registered on orcaonly" in ans,
              "a code the machine does not offer is skipped with the reason")
        check(s.state_of(urls[0]) == "ready", "the skipped calculation stays ready")
        ans = send("launchselected on=small profile=Project %s" % cut)
        say("  answer: %s" % ans)
        check("cut: failed: machine small is set to staged launches" in ans,
              "the launch on the other machine is tried (a staged machine stops it)")
        for want in ("machine localhost \u2192 small", "queue batch \u2192 normal",
                     "48 \u2192 12 cores", "nodes 2 \u2192 1",
                     "wall time 4:00 h \u2192 1:00 h", "profile Default \u2192 Project"):
            check(want in ans, "changes list: %s" % want)
        rc, out = s.driver("launchdata", cut)
        stored = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
        say("  stored: %s" % stored)
        check(stored.get("machine") == "small" and stored.get("queue") == "normal" and
              stored.get("procs") == "12" and stored.get("profile") == "Project" and
              stored.get("rundir") == runproj and stored.get("memory") == "8192",
              "the machine and cut settings are stored with the calculation")

        ans = send("launchselected %s" % " ".join(every))
        say("  answer: %s" % ans)
        check(all("[" not in part for part in ans.split("|") if part.startswith("sel")),
              "As set up lists no changes")
        check(ans.startswith("ok "), "batch answered ok")
        for n in range(3):
            check(("sel%d: submitted" % n) in ans, "sel%d reported submitted" % n)
        if nowall:
            check("nowall: submitted" in ans, "no wall time on a Shell machine is not skipped")
        if bad:
            check("nomachine: skipped" in ans, "the calculation without settings is skipped")
        if failing:
            check("failsub: submitted" not in ans, "the unusable run directory is not submitted")
            check(s.state_of(failing) == "ready", "the failed submit leaves it ready")
            reason = s.reason_of(failing)
            say("  stored reason: %s" % reason)
            check(reason != "", "the failure reason is stored in runStatusReason")
            log = s.run_log(failing)
            check("Job submission failed" in log,
                  "the run log holds the failure")
            #  The cause is fixed; the next submit goes through and clears it.
            os.remove(badrun)
            os.makedirs(badrun)
            ans = send("launchselected %s" % failing)
            check("failsub: submitted" in ans, "relaunch submitted (%s)" % ans[:100])
            check(s.reason_of(failing) == "", "a successful launch clears the reason")
        rc, out = s.driver("launchdata", urls[1])
        check("machine=localhost" in out and "queue=normal" in out and "procs=1\n" in out,
              "As set up leaves the stored settings as they were")
        for n, u in enumerate(urls):
            st = s.waitState(u, seconds=120,
                             want=harness.FINAL_STATES + ("submitted", "running"))
            check(st not in ("ready", "created", ""), "sel%d left ready: %s" % (n, st))
        #  #640: the Organizer answers other commands while a batch submits.
        if all(asyncs):
            ans = send("launchselectedasync %s" % " ".join(asyncs))
            check(ans == "started", "async batch started (%s)" % ans)
            t0 = time.time()
            ans = send("launchselectionrows %s" % " ".join(rows), timeout=20)
            check(ans.startswith("ok "), "another command is answered during the batch "
                  "(%.1f s): %s" % (time.time() - t0, ans[:40]))
            end = time.time() + 120
            while time.time() < end:
                ans = send("launchselectedstatus", timeout=20)
                if ans.startswith("idle"):
                    break
                time.sleep(0.5)
            say("  status: %s" % ans)
            check(ans.startswith("idle ") and all(
                  ("async%d: submitted" % n) in ans for n in range(3)),
                  "the async batch ends with all submitted")
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
