#!/usr/bin/env python3
"""
The Calculation Editor's constraints line (#456, #461).

The Builder's constraints and scan are stored with the calculation.  The
Settings box must show them for every run type and say whether this code's
input uses them or leaves them out (ECCE-QM leaves them out of GeoVib
and Opt-TS), and name a relaxed scan.  The CalcEd script command
"constraints" prints the line; ECCE_CONSTRAINTS_PNG_DIR, when set, gets
photographs of the editor for a scan and for an ignored combination.

Each calculation holds a fixed O-H bond and a scan of the H-O-H angle.  Same
installed tree, isolation and services as run_tests.py.  Exit 77 (skip)
without an install or Xvfb.
"""

import os
import re
import shutil
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402
import calced_no_basis_test as nobasis  # noqa: E402

PROJECT_META = os.path.join(REPO, "tools", "screenshots", "data", "project.ecce-meta")
FIX = os.path.join(HERE, "fixtures")
GCST = ("<GeomConstraintModel>\n"
        '<Constraint type="bond">\n  <Group>0 1</Group>\n  <Value>0.96</Value>\n</Constraint>\n'
        '<Constraint type="angle">\n  <Group>1 0 2</Group>\n  <Value>104.5</Value>\n'
        '  <Scan steps="4" step="5.0"/>\n</Constraint>\n'
        "</GeomConstraintModel>\n")
SCAN = "Relaxed scan: %s from 104.5 to 124.5 deg in 4 steps"
LINE = re.compile(r"CALCED: constraints count=(\d+) note=(.*)")
SHOWN = re.compile(r"CALCED: constraintsline shown=(\d) text=(.*)")

# label, fixture, [(run type, line the editor shows)]
USED = "1 constraint, used by this %s run | " + SCAN + ", used by this %s run"
NOGV = ("1 constraint, not used by a GeoVib run (frequencies need an unconstrained minimum) | "
        + SCAN + ", not used by a GeoVib run (frequencies need an unconstrained minimum)")
NOTS = ("1 constraint, not used by a transition-state search | "
        + SCAN + ", not used by a transition-state search")
CASES = [
    ("ECCE-QM", os.path.join(REPO, "tests", "ecceqm", "fixtures", "h2o-geometry"), [
        ("Energy", "1 constraint, not used by an Energy run | " + SCAN + ", not used by an Energy run"),
        ("Geometry", USED),
        ("GeoVib", NOGV),
        ("Transition State (Opt-TS)", NOTS),
    ]),
    ("ORCA", os.path.join(FIX, "calc-orca-water-ccpvdz"), [
        ("Geometry", USED), ("GeoVib", USED),
        ("Energy", "1 constraint, not used by an Energy run | " + SCAN + ", not used by an Energy run"),
    ]),
    ("Gaussian-16", os.path.join(FIX, "calc-polar-h2o-g16"), [
        ("Geometry", USED),
    ]),
]


def makeCalc(project, name, source):
    path = os.path.join(project, name)
    shutil.copytree(source, path, symlinks=True)
    with open(os.path.join(path, "Parameters", "GeomConstraintModel.gcst"), "w") as h:
        h.write(GCST)
    # Not run, so the run type may be changed.
    meta = os.path.join(path, ".ecce-meta")
    text = open(meta, errors="surrogateescape").read()
    text = re.sub(r"(ecce:state\t\t)[^\n]*", r"\g<1>Ready", text)
    text = re.sub(r"(ecce:runtype\t\t)[^\n]*", r"\g<1>Geometry", text)
    open(meta, "w", errors="surrogateescape").write(text)
    return path


def expected(template, runtype, coord):
    text = template
    if text.count("%s") == 3:     # USED: run type twice, coordinate once
        text = text % (runtype, coord, runtype)
    else:
        text = text % coord if "%s" in text else text
    return text


def photographer(display, pngDir, names, stop):
    """Take the editor when the script asks (`shot NAME`), then release it."""
    state = fixture.stateDir()
    while not stop.is_set():
        for name in list(names):
            ready = os.path.join(state, name + ".ready")
            if os.path.exists(ready):
                time.sleep(1)
                wins = [w for w, t in display.windows() if t.endswith("Editor")]
                if wins:
                    subprocess.run(["import", "-display", display.name, "-window",
                                    wins[0], os.path.join(pngDir, name + ".png")],
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                names.remove(name)
                open(os.path.join(state, name + ".go"), "w").close()
        time.sleep(0.5)


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "constraintsline-localdata")
        shutil.rmtree(folder, ignore_errors=True)
        project = os.path.join(folder, "users", "local", "p")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        os.environ["ECCE_LOCAL_DATA"] = folder
        pngDir = os.environ.get("ECCE_CONSTRAINTS_PNG_DIR")
        restorePrefs = fixture.settleUpgradeNotices()
        stop = threading.Event()
        shots = []
        if pngDir:
            os.makedirs(pngDir, exist_ok=True)
        try:
            for label, source, runtypes in CASES:
                calc = makeCalc(project, label, source)
                steps = ["wait 9000", "constraints"]
                for runtype, _ in runtypes:
                    steps += ["runtype " + runtype, "wait 3000", "constraints"]
                    shot = None
                    if pngDir and (label, runtype) == ("ORCA", "Geometry"):
                        shot = "constraints-scan-orca-geometry"
                    elif pngDir and (label, runtype) == ("ECCE-QM", "GeoVib"):
                        shot = "constraints-ignored-ecceqm-geovib"
                    if shot:
                        shots.append(shot)
                        steps += ["wait 1500", "shot " + shot]
                steps.append("quit")
                if shots and not stop.is_set() and not any(
                        t.name == "photo" for t in threading.enumerate()):
                    threading.Thread(target=photographer, name="photo", daemon=True,
                                     args=(display, pngDir, shots, stop)).start()
                log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT", steps,
                                   "file://" + calc + "/")
                lines = LINE.findall(log)
                shown = SHOWN.findall(log)
                sym.check(len(lines) == len(runtypes) + 1,
                          "%s: the editor reported the line %d times (got %d)"
                          % (label, len(runtypes) + 1, len(lines)),
                          run_tests._tail(log, 30))
                if len(lines) != len(runtypes) + 1:
                    continue
                coord = None
                m = re.search(r"Relaxed scan: (\S+) from", lines[0][1])
                coord = m.group(1) if m else "?"
                sym.check(m is not None and lines[0][0] == "2",
                          "%s: the scan is named on open (%s) and both constraints "
                          "counted" % (label, lines[0][1]))
                for (runtype, template), (count, note) in zip(runtypes, lines[1:]):
                    want = expected(template, runtype, coord)
                    print("%s %s: %s" % (label, runtype, note))
                    sym.check(note.strip() == want,
                              "%s, %s: the line says %r (got %r)"
                              % (label, runtype, want, note.strip()))
                # Opened on a Geometry run the line is there from the start.
                if shown:
                    sym.check(shown[0][0] == "1" and "Relaxed scan" in shown[0][1],
                              "%s: the Settings line is shown on open (%r)" % (label, shown[0][1][:60]))
                # The box shows the line only where the run type applies it (#502).
                for (runtype, template), (vis, text) in zip(
                        [(None, None)] + runtypes, shown):
                    if runtype is None:
                        continue
                    sym.check(("Relaxed scan" in text) == (template == USED) and
                              (vis == "1") == bool(text),
                              "%s, %s: the Settings line is %s (shown=%s)"
                              % (label, runtype,
                                 "shown" if template == USED else "hidden", vis))
            #  The Notes row holds the constraint line and why the calculation
            #  is not ready, in one place (#461).
            other = os.path.join(fixture.stateDir(), "constraintsline-nobasis")
            problem = sym.makeCalculations(other)
            sym.check(not problem, "calculations for the not-ready case made", problem)
            calc = os.path.join(other, "users", "local", nobasis.CALC)
            nobasis.dropBasisProperty(calc)
            with open(os.path.join(calc, "Parameters", "GeomConstraintModel.gcst"), "w") as h:
                h.write(GCST)
            os.environ["ECCE_LOCAL_DATA"] = other
            steps = ["wait 9000", "symmetry"]
            if pngDir:
                shots.append("notes-not-ready-g16-energy")
                if not any(t.name == "photo" for t in threading.enumerate()):
                    threading.Thread(target=photographer, name="photo", daemon=True,
                                     args=(display, pngDir, shots, stop)).start()
                steps += ["wait 1500", "shot notes-not-ready-g16-energy"]
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT", steps + ["quit"],
                               "file://" + calc + "/")
            m = re.search(r"CALCED: symmetry box=\d+ stored=\d+ note=(.*)", log)
            notes = re.sub(r"\s*\|\s*", " ", m.group(1)) if m else ""   # wrapped rows
            # An Energy run does not apply the constraints, so only the reason (#502).
            sym.check("Not ready: choose a basis set" in notes
                      and "Relaxed scan" not in notes,
                      "the Notes row says why it is not ready, no constraints line "
                      "for an Energy run (got %r)" % notes,
                      run_tests._tail(log, 30))
        finally:
            stop.set()
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if sym.failures:
            results.fail("calced-constraints-line", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
