#!/usr/bin/env python3
"""qm_scan_deck: ai.ecceqm turns the Builder's scan constraint into the
engine's `scan` block (#450), and ecce-qm reads the deck it wrote.

    scan_deck_test.py [ECCE_QM_EXE]

The constraint file counts atoms from 0 and gives the scan as steps and step
size; the engine counts from 1 and takes from, to and the number of points
(n steps are n+1 points, as for ORCA).  A plain constraint stays under
`constraints`, a scan is written for a Geometry run only.
"""
import os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
CON = '<Constraint type="%s">\n  <Group>%s</Group>\n  <Value>%s</Value>\n%s</Constraint>\n'
fails = 0


def check(ok, what):
    global fails
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    fails += not ok


def deck(constraint, runtype, work):
    shutil.copy(os.path.join(PARSERS, "ecceqm.tpl"), os.path.join(work, "t.tpl"))
    open(os.path.join(work, "w.frag"), "w").write(
        "# frag\nnum_atoms: 4\ntitle: h2o2\natom_info: symbol\natom_list:\n"
        "O 0.0 0.73 0.0\nO 0.0 -0.73 0.0\nH 0.85 0.9 0.3\nH -0.85 -0.9 0.3\n")
    open(os.path.join(work, "w.basis"), "w").write("basis STO-3G\n")
    open(os.path.join(work, "w.con"), "w").write(
        "<GeomConstraintModel>\n" + constraint + "</GeomConstraintModel>\n")
    open(os.path.join(work, "w.param"), "w").write(
        "Category: SCF\nTheory: HF\nRunType: %s\nCharge: 0\nChemSys.Multiplicity: 1\n" % runtype)
    p = subprocess.run(["perl", os.path.join(PARSERS, "ai.ecceqm"), "-p", "-f", "-b",
                        "-c", "-n", "w", "-t", "t.tpl"], cwd=work, capture_output=True,
                       text=True, env=dict(os.environ, ECCE_HOME=ROOT))
    if p.returncode:
        raise RuntimeError(p.stderr)
    return open(os.path.join(work, "t.tpl")).read()


def main():
    exe = sys.argv[1] if len(sys.argv) > 1 else None
    work = tempfile.mkdtemp(prefix="ecce-qmscan-")
    try:
        torsion = CON % ("torsion", "2 0 1 3", "141.11993", '  <Scan steps="2" step="30.0"/>\n')
        bond = CON % ("bond", "0 1", "1.46", '  <Scan steps="4" step="-0.05"/>\n')
        fixed = CON % ("bond", "0 1", "1.45", "")
        d = deck(torsion, "Geometry", work)
        check("scan\n  torsion 3 1 2 4 141.11993 201.11993 3\nend" in d,
              "a torsion scan: atoms from 1, 3 points for 2 steps")
        check("constraints" not in d, "no constraints block beside it")
        d = deck(bond, "Geometry", work)
        check("scan\n  bond 1 2 1.46 1.26 5\nend" in d, "a bond scan with a negative step")
        d = deck(fixed + torsion, "Geometry", work)
        check("constraints\n  bond 1 2 1.45\nend" in d and "  torsion 3 1 2 4 141.11993 201.11993 3" in d,
              "a held bond and a scan are both written")
        check("scan" not in deck(torsion, "Energy", work), "no scan for an Energy run")
        check("scan" not in deck(torsion, "Gradient", work), "no scan for a Gradient run")
        try:
            deck(torsion + bond, "Geometry", work)
            check(False, "two scans are refused")
        except RuntimeError as e:
            check("one coordinate" in str(e), "two scans are refused")
        if exe:
            d = deck(torsion, "Geometry", work)
            open(os.path.join(work, "run.qm"), "w").write(d)
            p = subprocess.run([exe, os.path.join(work, "run.qm")], capture_output=True, text=True)
            table = p.stdout.split("begin scan_table")[-1]   # rewritten after every point
            rows = re.findall(r"^\d+ (\S+) (\S+) (yes|no)$", table, re.M)
            check(p.returncode == 0 and len(rows) == 3 and all(r[2] == "yes" for r in rows),
                  "ecce-qm runs the deck: 3 converged points (%s)" % (p.stderr[-200:] or "ok"))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
