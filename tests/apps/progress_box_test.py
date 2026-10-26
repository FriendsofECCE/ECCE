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
opt~ maxgrad=1.20e-02,3.10e-03,9.00e-04,4.10e-04
opt~ rmsgrad=5.00e-03,1.40e-03,2.10e-04,8.00e-05
scf cycle=7 energy=-76.4089123400 dE=-4.10e-08/1e-10 grad=3.20e-06/1e-07
scf~ dE=-2.10e-01,-3.30e-02,-1.20e-03,-6.40e-05,-2.20e-06,-4.10e-08
"""

# What the monitor makes of a real ORCA Opt+Freq (analytic Hessian, H2O) cut in
# the CP-SCF solver (tests/parsers/fixtures/orca/h2o_optfreq.out, line 2494):
# the optimisation is over and the Frequencies stage is already there.
HESS = """# seq=2 sent={sent:.3f} at={at}
opt step=4 energy=-74.965901192 dE=-0.0000025998/0.0000050000 rmsgrad=0.0000114783/0.0001000000 maxgrad=0.0000140004/0.0003000000 rmsstep=0.0000184919/0.0020000000 maxstep=0.0000226334/0.0040000000 converged=yes
opt~ rmsgrad=0.0164334166,0.0042057273,0.0009652234,0.0000114783
opt~ maxgrad=0.0273727398,0.0050080968,0.0011504486,0.0000140004
scf cycle=2 energy=-74.9659011921898468 dE=-2.40e-11/1.000e-08
scf~ dE=0.00e+00,-2.40e-11
freq kind=analytic point=3/9
"""

LIVE = """# seq=30 sent={sent:.3f} at={at}
opt step=6 energy=-76.4089500000 maxgrad=9.00e-05/3e-04 rmsgrad=3.00e-05/1e-04 maxstep=4.00e-04/4e-03 rmsstep=2.00e-04/2e-03 dE=-1.00e-07/5e-06 converged=yes
opt~ maxgrad=1.20e-02,3.10e-03,9.00e-04,4.10e-04,2.00e-04,9.00e-05
opt~ rmsgrad=5.00e-03,1.40e-03,2.10e-04,8.00e-05,5.00e-05,3.00e-05
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

# The last snapshot the monitor makes over a real ORCA relaxed scan (ethane
# H-C-C-H, HF/STO-3G, 5 points; tests/progress/fixtures/orca/scan.out).
SCAN = """# seq=4 sent={sent:.3f} at={at}
scan point=5/5
opt step=6 energy=-78.306179011 dE=-0.0000048721/0.0000050000 rmsgrad=0.0000986701/0.0001000000 maxgrad=0.0002091017/0.0003000000 rmsstep=0.0005809189/0.0020000000 maxstep=0.0011084890/0.0040000000 converged=yes
opt~ energy=-77.976865472,-78.037906627,-78.088807343,-78.142939814,-78.186026780,-78.219840446,-78.244953808,-78.260322404,-78.273842735,-78.289991238,-78.301966685,-78.305590245,-78.306092531,-78.306170728,-78.306178683,-78.287533444,-78.296960682,-78.301882132,-78.303878740,-78.303958175,-78.303962346,-78.282402350,-78.292240938,-78.297911973,-78.301398194,-78.301598994,-78.301604758,-78.284318355,-78.294198187,-78.300078089,-78.303780376,-78.303959186,-78.303964756,-78.290071304,-78.299244658,-78.304114959,-78.306111127,-78.306174138,-78.306179011
scf cycle=4 energy=-78.3061796133648329 dE=-2.14e-09/1.000e-08
scf~ dE=0.00e+00,-8.70e-08,-1.79e-08,-2.14e-09

"""

# A GROMACS dynamics run part way (#530): time axis out to the total.
GMX = """# seq=9 sent={sent:.3f} at={at}
md point=300/1000
gmx-epot step=300 time=0.6 epot=-6.5e+04 avg=-65100 sd=40 n=9
gmx-epot~ epot=-65300,-65200,-65050,-65100,-65150,-65000,-65120,-65090,-65110
gmx-temp step=300 time=0.6 temp=2.9e+02 avg=300.2 sd=3.1 n=9
gmx-temp~ temp=287,300,291,305,296,310,289,301,299
"""

# The last snapshot over a real ORCA NEB-TS run (HCN -> HNC, HF/3-21G;
# tests/progress/fixtures/orca/nebts.out).
NEB = """# seq=4 sent={sent:.3f} at={at}
neb iter=15 erel=0.097436 maxforce=0.004704/2.00e-02 rmsforce=0.001211/1.00e-02 maxcforce=0.000999/2.00e-03 rmscforce=0.000530/1.00e-03
neb~ erel=0.107317,0.103988,0.101228,0.097268,0.097091,0.097177,0.097133,0.097187,0.097460,0.097507,0.097237,0.097321,0.097344,0.097343,0.097375,0.097436
opt step=3 energy=-92.246042678 dE=0.0000001218/0.0000050000 rmsgrad=0.0000237583/0.0001000000 maxgrad=0.0000415214/0.0003000000 rmsstep=0.0001137107/0.0020000000 maxstep=0.0001768984/0.0040000000 converged=yes
opt~ energy=-92.246046065,-92.246042800,-92.246042678
scf cycle=3 energy=-92.2460426775835600 dE=-1.94e-09/1.000e-08
scf~ dE=0.00e+00,-7.27e-09,-1.94e-09

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


def checkLayout(tag, b):
    """Sections run SCF, Optimisation, Scan, Frequencies; each plot is beside
    its section's text or directly under it, and nothing overlaps."""
    titles = [t for t in re.findall(r"\| (?:\(grey\) )?(SCF|Optimisation|NEB|Scan|Frequencies): ", b)]
    seen = []
    for t in titles:
        if t not in seen:
            seen.append(t)
    want = [t for t in ("SCF", "Optimisation", "NEB", "Scan", "Frequencies") if t in seen]
    check(seen == want, "%s: sections in the order SCF, Optimisation, NEB, Scan, "
          "Frequencies (%s)" % (tag, ", ".join(seen)), b)
    prevBottom = 0
    for g in re.finditer(r"\| geom (\w+) text (\d+) (\d+) plot (\d+) (\d+) (\d+) (\d+)", b):
        title = g.group(1)
        y0, textEnd, px, py, pw, ph = [int(g.group(i)) for i in range(2, 8)]
        beside = py == y0 and px > 0
        under = py == textEnd and px == 0
        check(beside or under, "%s: the %s plot is beside its text or directly "
              "under it (%s)" % (tag, title, "beside" if beside else "under"),
              g.group(0))
        check(y0 >= prevBottom, "%s: the %s section starts below the previous "
              "plot" % (tag, title), g.group(0))
        prevBottom = max(textEnd, py + ph)
    caps = {"SCF": ["SCF: |\u0394E| per cycle (Hartree)"],
            "Optimisation": ["Optimisation: energy per step, all points",
                             "Optimisation: max and RMS force per step (Hartree/bohr), all points"]}
    for title, options in caps.items():
        if re.search(r"\| geom %s " % title, b):
            check(any(("plot " + cap) in b for cap in options),
                  "%s: caption of the %s plot" % (tag, title), b)


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
        scan = makeCalc(project, "water-scan", "Complete", DONE.format(**stamp))
        with open(os.path.join(scan, "Props", "SCANENERGY"), "w") as handle:
            handle.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                         '<vector name="SCANENERGY" rowLabel="Scan Point" '
                         'rowLabels="1-A 2-A 3-A" rows="3" units="Hartree">'
                         '-76.40000   -76.41000   -76.40500</vector>\n')
        scanrun = makeCalc(project, "ethane-scan", "Running",
                           SCAN.format(**stamp))
        nebrun = makeCalc(project, "hcn-neb", "Running", NEB.format(**stamp))
        gmxrun = makeCalc(project, "gmx-md", "Running", GMX.format(**stamp))
        hessrun = makeCalc(project, "orca-hessian", "Running", HESS.format(**stamp))
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
                "summary file://" + scan,
                "progressbox",
                "summary file://" + scanrun,
                "size 1024 600",
                "progressbox",
                "showbox",
                "snap " + png("progress-scan-1024x600.png"),
                "size 640 600",
                "progressbox",
                "showbox",
                "snap " + png("progress-scan-640x600.png"),
                "size 1366 768",
                "progressbox",
                "showbox",
                "snap " + png("progress-scan-1366x768.png"),
                "size 1920 1080",
                "progressbox",
                "showbox",
                "snap " + png("progress-scan-1920x1080.png"),
                "summary file://" + nebrun,
                "size 1024 700",
                "progressbox",
                "showbox",
                "snap " + png("progress-neb-1024x700.png"),
                "summary file://" + gmxrun,
                "showbox",
                "snap " + png("progress-gmx-md.png"),
                "summary file://" + hessrun,
                "progressbox",
                "snap " + png("progress-orca-hessian.png"),
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
        if not check(len(boxes) == 11 and len(counts) == 2,
                     "every command answered (%d boxes, %d counts)" % (len(boxes), len(counts)),
                     run_tests._tail(log, 30)):
            return
        for b in boxes:
            print("      " + b)
        stored, after, completed, none, scanned, scan1, narrow, scan2, scan3, neb, hess = boxes

        def field(b, k):
            m = re.search(r"\b%s=(\S+)" % k, b)
            return m.group(1) if m else None

        check("Frequencies: analytic Hessian, 3 of 9 perturbations" in hess and
              "Optimisation: step 4" in hess and "converged" in hess and
              "s left" not in hess,
              "ORCA analytic Opt+Freq: the Frequencies stage is shown before the end", hess)
        check(field(stored, "shown") == "1", "running: the box is shown")
        check("Optimisation: step 4" in stored and "max force 4.1e-04 (limit 3e-04) \u2717" in stored
              and "RMS force 8.0e-05 (limit 1e-04) \u2713" in stored,
              "running (stored): step, criteria against targets", stored)
        check("plot Optimisation: max and RMS force per step (Hartree/bohr), all points (4 points)" in stored and
              "plot SCF: |\u0394E| per cycle (Hartree) (6 points)" in stored,
              "running (stored): the force and SCF plots", stored)
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
              "plot Optimisation: energy per step, all points" in completed and
              completed.index("plot Optimisation") < completed.index("summary: ") and
              "s left" not in completed and "updated" not in completed,
              "completed: the plots stay, the outcome lines follow them", completed)
        check(re.search(r"summary: Final energy -?\d+\.\d{6} Hartree$", completed)
              is not None,
              "completed: the last line is the final energy (#352)", completed)
        energy = re.search(r"summary: Final energy (\S+) Hartree$", completed)
        check(energy is not None and field(completed, "copyvalue") == energy.group(1),
              "completed: Copy energy gives the value shown (#408)", completed)
        check(field(completed, "energyselectable") == "1",
              "completed: the final energy can be selected with the mouse (#408)", completed)
        check(scanned.endswith("summary: Final energy (last scan point) -76.405 Hartree"),
              "relaxed scan: the final energy is the last scan point's", scanned)
        for tag, b in (("1024x600", scan1), ("1366x768", scan2), ("1920x1080", scan3)):
            check("Scan: point 5 of 5" in b and "bar 1.00" in b and
                  "plot Optimisation: energy per step, all points (39 points)" in b and
                  "plot SCF: |\u0394E| per cycle (Hartree) (4 points)" in b,
                  "relaxed scan (%s): point 5 of 5, every optimisation step "
                  "of all 5 points in the energy plot" % tag, b)
        for tag, b in (("stored", stored), ("live", after), ("completed", completed),
                       ("scan 1024x600", scan1), ("scan 640x600", narrow),
                       ("scan 1366x768", scan2), ("scan 1920x1080", scan3)):
            checkLayout(tag, b)
        check("NEB: iteration 15" in neb and
              "E(highest image) - E(reactant) (Eh) 0.097436" in neb and
              "max force, climbing image 1.0e-03 (limit 2e-03) \u2713" in neb and
              "plot NEB: E(highest image) - E(reactant) (Eh) per iteration (16 points)" in neb,
              "NEB-TS: iteration, energy, forces against limits, 16-point plot", neb)
        checkLayout("neb 1024x700", neb)
        check(field(none, "shown") == "0", "never launched: no box", none)
        for n in ("progress-running-stored.png", "progress-running-live.png",
                  "progress-completed.png", "progress-scan-1024x600.png",
                  "progress-scan-640x600.png", "progress-scan-1366x768.png",
                  "progress-scan-1920x1080.png", "progress-neb-1024x700.png"):
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
