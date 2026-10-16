#!/usr/bin/env python3
"""
The Organizer's Progress box (#284), headless.

A local-data calculation (fixtures/calc-water-opt, made to look as a job
does while it runs and after it) is selected through the ECCE_TEST_ORGANIZER
hook; "progressbox" prints what the box draws and "snap" saves the window:

- running, from the snapshot stored with the calculation (ecce:progress,
  what a restarted Organizer has): optimisation step, criteria, plots,
  frequency bar, "updated n s ago";
- running, after a live ecce_url_progress snapshot ("progress <url> <file>",
  the path the broker message takes): newer lines, the same panel (no
  rebuild), more paints of the box, and no tree or summary update;
- completed: the plots as they were, the outcome lines below them;
- a calculation that was never launched: no box.

ECCE_PROGRESS_PNGS=<dir> keeps the window captures there.  Same installed
tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import re
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data",
                            "project.ecce-meta")
NS = "http://www.emsl.pnl.gov/ecce:"

STORED = """# seq=12 sent={sent:.3f} at={at}
opt step=4 energy=-76.4089123400 maxgrad=4.10e-04/3e-04 rmsgrad=8.00e-05/1e-04 maxstep=2.50e-03/4e-03 rmsstep=1.10e-03/2e-03 dE=-1.20e-05/5e-06
opt~ energy=-76.3871000000,-76.4052000000,-76.4086000000,-76.4089123400
scf cycle=7 energy=-76.4089123400 dE=-4.10e-08/1e-10 grad=3.20e-06/1e-07
scf~ dE=-2.10e-01,-3.30e-02,-1.20e-03,-6.40e-05,-2.20e-06,-4.10e-08
"""

LIVE = """# seq=30 sent={sent:.3f} at={at}
opt step=6 energy=-76.4089500000 maxgrad=9.00e-05/3e-04 rmsgrad=3.00e-05/1e-04 maxstep=4.00e-04/4e-03 rmsstep=2.00e-04/2e-03 dE=-1.00e-07/5e-06 converged=yes
opt~ energy=-76.3871000000,-76.4052000000,-76.4086000000,-76.4089123400,-76.4089480000,-76.4089500000
freq point=14/54 eta=75
scf cycle=9 energy=-76.4089412300 dE=-3.00e-09/1e-11 grad=4.00e-07/1e-08
scf~ dE=-1.10e-03,-4.00e-05,-2.00e-06,-1.30e-07,-8.00e-09,-3.00e-09
"""

DONE = """# seq=61 sent={sent:.3f} at={at}
opt step=6 energy=-76.4089500000 converged=yes
opt~ energy=-76.3871000000,-76.4052000000,-76.4086000000,-76.4089123400,-76.4089480000,-76.4089500000
freq point=54/54 done=yes real=3 imaginary=0 zpe=0.0213456789 eta=0
scf cycle=10 energy=-76.4089397700 converged=yes
"""

failures = []


def esc(text):
    return text.replace("\\", "\\\\").replace("\n", "\\n").replace("\t", "\\t")


def makeCalc(project, name, state, progress):
    calc = os.path.join(project, name)
    shutil.copytree(os.path.join(fixture.FIXTURES, "calc-water-opt"), calc,
                    symlinks=True)
    meta = os.path.join(calc, ".ecce-meta")
    lines = []
    for line in open(meta).read().splitlines():
        if line.startswith(".\t" + NS + "state\t"):
            line = ".\t%sstate\t\t%s" % (NS, state)
        lines.append(line)
    if progress:
        lines.append(".\t%sprogress\t\t%s" % (NS, esc(progress)))
    with open(meta, "w") as handle:
        handle.write("\n".join(lines) + "\n")
    return calc


def answers(log, command):
    return re.findall(r"ECCE_TEST_ORGANIZER: %s[^:]*: (.*)" % re.escape(command), log)


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display, pngs):
    data = tempfile.mkdtemp(prefix="ecce-progress-data-")
    work = tempfile.mkdtemp(prefix="ecce-progress-")
    try:
        project = os.path.join(data, "users", "local", "progress-test")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        now = time.time()
        stamp = {"sent": now - 20, "at": int(now - 20)}
        running = makeCalc(project, "water-running", "Running",
                           STORED.format(**stamp))
        done = makeCalc(project, "water-done", "Complete",
                        DONE.format(**stamp))
        fresh = makeCalc(project, "water-new", "Created", None)
        live = os.path.join(work, "live.txt")
        with open(live, "w") as handle:
            handle.write(LIVE.format(sent=now, at=int(now)))
        png = lambda n: os.path.join(pngs, n)
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as handle:
            handle.write("\n".join([
                "size 1100 1000",
                "summary file://" + running,
                "progressbox",
                "snap " + png("progress-running-stored.png"),
                "counts",
                "progress file://%s %s" % (running, live),
                "progressbox",
                "counts",
                "snap " + png("progress-running-live.png"),
                "summary file://" + done,
                "progressbox",
                "snap " + png("progress-completed.png"),
                "summary file://" + fresh,
                "progressbox",
            ]) + "\n")
        result = apps.run(display, "organizer", windowTimeout=60, settle=45,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + running,
                               "ECCE_LOCAL_DATA": data})
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return
        boxes = answers(log, "progressbox")
        counts = answers(log, "counts")
        if not check(len(boxes) == 4 and len(counts) == 2,
                     "every command answered (%d boxes, %d counts)" % (len(boxes), len(counts)),
                     run_tests._tail(log, 30)):
            return
        for b in boxes:
            print("      " + b)
        stored, after, completed, none = boxes

        def field(b, k):
            m = re.search(r"\b%s=(\S+)" % k, b)
            return m.group(1) if m else None

        check(field(stored, "shown") == "1", "running: the box is shown")
        check("Optimisation: step 4" in stored and "max force 4.1e-04 / 3e-04 \u2717" in stored
              and "RMS force 8.0e-05 / 1e-04 \u2713" in stored,
              "running (stored): step, criteria against targets", stored)
        check("plot energy per step (4 points)" in stored and
              "plot log |\u0394E| per cycle (6 points)" in stored,
              "running (stored): the energy and SCF plots", stored)
        check(re.search(r"updated (19|2\d) s ago", stored) is not None,
              "running (stored): updated about 20 s ago, from the job store's stamp", stored)
        check("Frequencies: point 14 of 54" in after and "bar 0.26" in after and
              re.search(r"about (7[0-5]|6\d) s left", after) is not None,
              "live: frequency point, bar and time left", after)
        check("Optimisation: step 6" in after and "converged" in after,
              "live: the newer optimisation step", after)
        check(field(after, "panel") == field(stored, "panel"),
              "live: the same summary panel (no rebuild)")
        check(int(field(after, "paints")) > int(field(stored, "paints")),
              "live: the box was repainted")
        check(counts[1].startswith("notices=0 applied=0"),
              "live: no tree or summary update for a progress message", counts[1])
        check("Optimisation converged in 6 steps" in completed and
              "Frequencies: 3 real, 0 imaginary" in completed and
              "ZPE 0.021346 hartree" in completed and
              "plot energy per step" in completed and
              completed.index("plot energy") < completed.index("summary: ") and
              "s left" not in completed and "updated" not in completed,
              "completed: the plots stay, the outcome lines follow them", completed)
        check(re.search(r"summary: Final energy -?\d+\.\d{6} Hartree$", completed)
              is not None,
              "completed: the last line is the final energy (#352)", completed)
        check(field(none, "shown") == "0", "never launched: no box", none)
        for n in ("progress-running-stored.png", "progress-running-live.png",
                  "progress-completed.png"):
            check(os.path.getsize(png(n)) > 1000 if os.path.exists(png(n)) else False,
                  "saved " + n)
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        keep = os.environ.get("ECCE_PROGRESS_PNGS")
        pngs = keep or tempfile.mkdtemp(prefix="ecce-progress-png-")
        os.makedirs(pngs, exist_ok=True)
        try:
            run(display, pngs)
        finally:
            if not keep:
                shutil.rmtree(pngs, ignore_errors=True)
        if failures:
            results.fail("progress", "progress box wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
