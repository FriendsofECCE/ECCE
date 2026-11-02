#!/usr/bin/env python3
"""A finished calculation whose structure has a cell opens with the cell drawn.

    tests/apps/viewer_cell_test.py              own Xvfb and services

The Viewer (a Builder on a calculation with results, read-only) has no
Periodic Builder pane open, and the cell used to be drawn only while that
pane was shown (#355).  The water fixture gets a lattice in its structure
file; "fragdump" reports whether the lattice exists and is drawn.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                 # noqa: E402
import fixture              # noqa: E402
import pbc_ux_test          # noqa: E402
import pbc_edit_test as pe  # noqa: E402
from builder_panes_test import _Results   # noqa: E402

LATTICE = "lattice_vectors: \n 6.0 0.0 0.0\n 0.0 6.0 0.0\n 0.0 0.0 6.0\n"


TRICLINIC = "lattice_vectors: \n 5.0 0.0 0.0\n 2.0 6.0 0.0\n 1.0 1.5 7.0\n"


def addTriclinic(calc):
    addCell(calc, TRICLINIC)


def addCell(calc, lattice=LATTICE):
    path = os.path.join(calc, "Parameters", "chemsys.mvm")
    text = open(path).read()
    assert "num_atoms:" in text
    open(path, "w").write(text.replace("num_atoms:", lattice + "num_atoms:", 1))


def readCell(dump):
    vecs, labels, corner, drawn, rep = [], [], [0.0] * 3, None, [1, 1, 1]
    for line in open(dump):
        w = line.split()
        if w[:1] == ["vector"]:
            vecs.append([float(v) for v in w[1:4]])
        elif w[:1] == ["latticecorner"]:
            corner = [float(v) for v in w[1:4]]
        elif w[:1] == ["latticelabel"]:
            labels.append([float(v) for v in w[2:5]])
        elif w[:1] == ["latticedrawn"]:
            drawn = [float(v) for v in w[1:7]]
        elif w[:1] == ["replication"]:
            rep = [int(v) for v in w[1:4]]
    return vecs, labels, corner, drawn, rep


def cellProblems(dump, tol=0.1, offset=0.05):
    """The drawn cell is the current toLines() cell, and the a/b/c labels sit
    at the ends of the base cell's edges (#723)."""
    vecs, labels, corner, drawn, rep = readCell(dump)
    if len(vecs) != 3 or len(labels) != 3 or not drawn:
        return ["%d vectors, %d labels, drawn %s in the dump"
                % (len(vecs), len(labels), drawn)]
    out = []
    lo = list(corner)
    hi = [corner[k] + sum(max(0.0, v[k]) for v in vecs) for k in range(3)]
    lo = [corner[k] + sum(min(0.0, v[k]) for v in vecs) for k in range(3)]
    for k in range(3):   # replication extends the box by (n-1) vectors
        for i, v in enumerate(vecs):
            ext = (rep[i] - 1) * v[k]
            hi[k] += max(0.0, ext)
            lo[k] += min(0.0, ext)
    for k in range(3):
        if abs(drawn[k] - lo[k]) > tol or abs(drawn[3 + k] - hi[k]) > tol:
            out.append("drawn cell %s..%s, current cell %s..%s"
                       % (drawn[:3], drawn[3:], lo, hi))
            break
    for n, p in enumerate(labels):
        end = [corner[k] + vecs[n][k] for k in range(3)]
        if any(abs(p[k] + offset - end[k]) > tol for k in range(3)):
            out.append("label %d at %s, edge end at %s" % (n, p, end))
    return out


def check(display, results):
    for name, mutate, want in (("viewer-cell", addCell, 1),
                               ("viewer-triclinic", addTriclinic, 1),
                               ("viewer-rep", addCell, 1),
                               ("viewer-nocell", None, 0)):
        url, error = pbc_ux_test.install(name, mutate)
        if error:
            results.fail("viewer cell", "cannot install fixture: %s" % error)
            return
        results.checks += 1
        auth = fixture.authFile(os.path.join(fixture.stateHome(), ".ECCE",
                                             "auth.pipe"), user=fixture.USER)
        work, problems, log = pbc_ux_test.runScene(
            display, name, ["fragdump opened", "panestate state Periodic_Builder"]
            + (["translate 1.7 -2.3 0.9", "fragdump moved"] if want else [])
            + (["pbcopen", "pbcpress replicate 2", "fragdump replicated"]
               if name == "viewer-rep" else []),
            url, ("-pipe", auth, "-context", url))
        for p in problems:
            results.fail("viewer cell " + name, p)
        dump = os.path.join(work, "opened.txt")
        if not os.path.exists(dump):
            results.fail("viewer cell " + name, "no structure dump")
            continue
        st = pbc_ux_test.readState(os.path.join(work, "state.txt"))
        pane = ((st or {}).get("tools") or {}).get("Periodic Builder")
        if not pane or pane["shown"]:
            results.fail("viewer cell " + name,
                         "the Periodic Builder pane is open or missing")
        counts = pe.readDump(dump)["counts"]
        if counts.get("lattice") != want:
            results.fail("viewer cell " + name, "lattice %s, expected %d"
                         % (counts.get("lattice"), want))
        if counts.get("latticeshown") != want:
            results.fail("viewer cell " + name,
                         "cell drawn %s, expected %d (Periodic Builder off)"
                         % (counts.get("latticeshown"), want))
        if want:
            last = "replicated" if name == "viewer-rep" else "moved"
            for tag in ("opened", "moved") + (("replicated",) if last == "replicated" else ()):
                path = os.path.join(work, tag + ".txt")
                if not os.path.exists(path):
                    results.fail("viewer cell " + name, "no %s dump" % tag)
                    continue
                for p in cellProblems(path):
                    results.fail("viewer cell %s (%s)" % (name, tag), p)
        fixture.remove(name)


def main():
    import isolate
    import xdisplay

    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    if not apps.installed("builder"):
        print("SKIP  no installed builder")
        return 77
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"

    results = _Results()
    xdisplay.SCREEN = "1366x768x24"
    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        try:
            if not all(apps.serviceState(display).values()):
                print("FAIL  services did not come up: %s" % log)
                return 1
            fixture.ensureRealUserAccount()
            restore = fixture.settleUpgradeNotices()
            try:
                check(display, results)
            finally:
                restore()
        finally:
            apps.stopServices(display)
    for failure in results.failures:
        print("FAIL  " + failure)
    if not results.failures:
        print("PASS  viewer cell")
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.exit(main())
