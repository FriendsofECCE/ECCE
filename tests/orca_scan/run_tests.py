#!/usr/bin/env python3
"""ORCA relaxed scan (#393): a scanned constraint must reach the deck as
'Scan D i j k l = start, end, n+1 end' in %geom (0-based atoms, same points as
Gaussian's 'S n step'), a plain constraint must stay a Constraint, and a real
ORCA 6 run of a generated deck must parse into SCANCOORD/SCANENERGY with one
point per step and the coordinate moving by the step.  Exit 77 (CTest SKIP)
without ORCA 6 for the run part.
"""
import glob, os, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
CON = '<Constraint type="%s">\n  <Group>%s</Group>\n  <Value>%s</Value>\n%s</Constraint>\n'
fails = 0


def check(ok, what):
    global fails
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    fails += not ok


def deck(constraint, work):
    shutil.copy(os.path.join(PARSERS, "orca.tpl"), os.path.join(work, "t.tpl"))
    open(os.path.join(work, "w.frag"), "w").write(
        "# frag\nnum_atoms: 4\ntitle: h2o2\natom_info: symbol\natom_list:\n"
        "O 0.0 0.73 0.0\nO 0.0 -0.73 0.0\nH 0.85 0.9 0.3\nH -0.85 -0.9 0.3\n")
    open(os.path.join(work, "w.basis"), "w").write("useRouteCard 3-21G\n")
    open(os.path.join(work, "w.con"), "w").write(
        "<GeomConstraintModel>\n" + constraint + "</GeomConstraintModel>\n")
    open(os.path.join(work, "w.param"), "w").write(
        "Category: SCF\nTheory: RHF\nRunType: Geometry\nCharge: 0\n"
        "ChemSys.Multiplicity: 1\nES.Theory.UseSymmetry: 1\n")
    env = dict(os.environ, ECCE_HOME=ROOT)
    p = subprocess.run(["perl", os.path.join(PARSERS, "ai.orca"), "-p", "-f", "-b",
                        "-c", "-n", "w", "-t", "t.tpl"], cwd=work, env=env,
                       capture_output=True, text=True)
    if p.returncode:
        raise RuntimeError(p.stderr)
    return open(os.path.join(work, "t.tpl")).read()


def main():
    work = tempfile.mkdtemp(prefix="ecce-oscan-")
    try:
        cases = [
            (CON % ("torsion", "2 0 1 3", "141.11993", '  <Scan steps="6" step="30.0"/>\n'),
             "Scan D 2 0 1 3 = 141.11993, 321.11993, 7 end", "Constraints"),
            (CON % ("bond", "0 1", "1.46", '  <Scan steps="10" step="-0.05"/>\n'),
             "Scan B 0 1 = 1.46, 0.96, 11 end", "Constraints"),
            (CON % ("torsion", "2 0 1 3", "141.11993", ""),
             "{ D 2 0 1 3 141.11993 C }", "Scan"),
        ]
        for con, want, absent in cases:
            d = deck(con, work)
            check(want in d and (absent not in d or absent == "Constraints"
                                 and "{ " not in d), want)
            if "Scan" in want:
                check("Constraints" not in d, "no Constraints block beside the scan")
                check("UseSym" not in d, "no UseSym in a scan (ORCA aborts when the point group changes)")
            else:
                check("UseSym" in d, "UseSym kept for a plain constrained optimisation")
        # Real run of the first case.
        orcas = [p for p in sorted(glob.glob("/opt/orca/orca_6*/orca")) if os.access(p, os.X_OK)]
        if not orcas:
            print("no ORCA 6: run skipped")
            return 1 if fails else 77
        d = deck(cases[0][0], work)
        open(os.path.join(work, "orca.orcain"), "w").write(d)
        subprocess.run([orcas[0], "orca.orcain"], cwd=work, timeout=900,
                       stdout=open(os.path.join(work, "orca.orcaout"), "w"),
                       stderr=subprocess.STDOUT)
        out = open(os.path.join(work, "orca.orcaout"), errors="replace").read()
        check("ORCA TERMINATED NORMALLY" in out, "ORCA ran the generated deck")
        i = out.rfind("RELAXED SURFACE SCAN RESULTS")
        j = out.find("Calculated Surface using the SCF energy", i)
        check(i > 0 and j > i, "results table present")
        p = subprocess.run(["perl", os.path.join(PARSERS, "orca.scan"), "SCANCOORD", "Geometry"],
                           cwd=work, input=out[i:j], capture_output=True, text=True)
        recs = {}
        for blk in p.stdout.split("END\n"):
            if "key:" in blk:
                L = blk.split("\n")
                recs[L[0].split()[1]] = (L[L.index("values: ") + 1:L.index("units:")], L[L.index("units:") + 1])
        c = [float(x) for x in recs["SCANCOORD"][0]]
        e = [float(x) for x in recs["SCANENERGY"][0]]
        print("  coords", c, "\n  energies", e)
        check(len(c) == 7 and len(e) == 7, "7 points (6 steps)")
        check(all(abs((b - a) - 30.0) < 1e-3 for a, b in zip(c, c[1:])), "coordinate moves by 30 degrees")
        check(all(-200 < x < -100 for x in e), "energies present")
        check(recs["SCANCOORD"][1] == "Degree", "unit Degree")
    finally:
        shutil.rmtree(work, True)
    print("FAILED" if fails else "PASSED")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
