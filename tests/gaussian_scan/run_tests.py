#!/usr/bin/env python3
"""A scanned constraint (#393) must reach the Gaussian 16 deck as
'D i j k l S <n> <step>' (no value: Gaussian reads a value as a fixed
coordinate and drops the scan) with Opt=ModRedundant; a plain constraint
keeps its 'F'.  Generates decks only; no Gaussian needed."""
import os, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")

CON = """<Constraint type="%s">
  <Group>%s</Group>
  <Value>%s</Value>
%s</Constraint>
"""


def deck(constraint):
    work = tempfile.mkdtemp(prefix="ecce-gscan-")
    try:
        shutil.copy(os.path.join(PARSERS, "g16.tpl"), os.path.join(work, "t.tpl"))
        open(os.path.join(work, "w.frag"), "w").write(
            "# frag\nnum_atoms: 4\ntitle: h2o2\natom_info: symbol\natom_list:\n"
            "O 0.0 0.73 0.0\nO 0.0 -0.73 0.0\nH 0.85 0.9 0.3\nH -0.85 -0.9 0.3\n")
        open(os.path.join(work, "w.basis"), "w").write("useRouteCard STO-3G\n")
        open(os.path.join(work, "w.con"), "w").write(
            "<GeomConstraintModel>\n" + constraint + "</GeomConstraintModel>\n")
        open(os.path.join(work, "w.param"), "w").write(
            "Category: SCF\nTheory: RHF\nRunType: Geometry\nCharge: 0\n"
            "ChemSys.Multiplicity: 1\n")
        env = dict(os.environ, ECCE_HOME=ROOT)
        p = subprocess.run(["perl", os.path.join(PARSERS, "ai.gauss16"), "-p", "-f",
                            "-b", "-c", "-n", "w", "-t", "t.tpl"], cwd=work, env=env,
                           capture_output=True, text=True)
        if p.returncode:
            raise RuntimeError(p.stderr)
        return open(os.path.join(work, "t.tpl")).read()
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    fails = 0
    cases = [
        (CON % ("torsion", "2 0 1 3", "141.11993", '  <Scan steps="6" step="30.0"/>\n'),
         "D 3 1 2 4 S 6 30.0"),
        (CON % ("bond", "0 1", "1.46", '  <Scan steps="10" step="-0.05"/>\n'),
         "B 1 2 S 10 -0.05"),
        (CON % ("torsion", "2 0 1 3", "141.11993", ""), "D 3 1 2 4 141.11993 F"),
    ]
    for con, want in cases:
        d = deck(con)
        ok = want in d and "ModRedundant" in d
        print("%-4s %s" % ("ok" if ok else "FAIL", want))
        if not ok:
            print(d)
        fails += not ok
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
