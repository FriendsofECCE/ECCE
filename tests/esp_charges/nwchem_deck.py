#!/usr/bin/env python3
"""ai.nwchem appends `task esp` after the main task only when the Theory
Details box is ticked (SCF/DFT, not the ESP runtype itself).  With --run the
deck also goes through the real nwchem and nwchem.esp reads the .q file."""
import os, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")


def deck(extra, category="SCF", theory="RHF", runtype="Energy", work=None):
    own = work is None
    work = work or tempfile.mkdtemp(prefix="ecce-nwesp-")
    try:
        tpl = os.path.join(work, "w.nw")
        shutil.copy(os.path.join(PARSERS, "nwch.tpl"), tpl)
        open(os.path.join(work, "w.frag"), "w").write(
            "title: water\ntype: molecule\nnum_atoms: 3\natom_info: symbol cart\n"
            "atom_list:\nO 0.0 0.0 0.1173\nH 0.0 0.7572 -0.4692\nH 0.0 -0.7572 -0.4692\n")
        open(os.path.join(work, "w.basis"), "w").write(
            'basis "ao basis" cartesian print\n  H library "6-31G*"\n'
            '  O library "6-31G*"\nEND\n')
        params = ["title: water", "annotation: water", "parseFile: w.ep",
                  "Category: " + category, "Theory: " + theory,
                  "RunType: " + runtype, "Charge: 0", "Symmetry: C1",
                  "NumElectrons: 10", "ChemSys.Multiplicity: 1",
                  "BasisSet.Coordinates: Cartesian"] + extra
        open(os.path.join(work, "w.param"), "w").write("\n".join(params) + "\n")
        env = dict(os.environ, ECCE_HOME=ROOT)
        p = subprocess.run(["perl", os.path.join(PARSERS, "ai.nwchem"), "-n", "w",
                            "-p", "-f", "-b", "-t", tpl], cwd=work, env=env,
                           capture_output=True, text=True)
        if p.returncode:
            raise RuntimeError(p.stderr)
        return open(tpl).read()
    finally:
        if own:
            shutil.rmtree(work, ignore_errors=True)


def tasks(text):
    return [l.strip() for l in text.splitlines() if l.startswith("task ")]


def main(run):
    fails = 0
    cases = [([], "SCF", "RHF", "Energy", ["task scf energy"]),
             (["ES.Theory.NWChem.ESPCharges: 0"], "SCF", "RHF", "Energy",
              ["task scf energy"]),
             (["ES.Theory.NWChem.ESPCharges: 1"], "SCF", "RHF", "Energy",
              ["task scf energy", "task esp"]),
             (["ES.Theory.NWChem.ESPCharges: 1"], "SCF", "RHF", "Geometry",
              ["task scf optimize", "task scf gradient", "task esp"])]
    for extra, cat, th, rt, want in cases:
        got = tasks(deck(extra, cat, th, rt))
        ok = got == want
        print(("ok   " if ok else "FAIL ") + " / ".join(got))
        fails += not ok
    if run:
        work = tempfile.mkdtemp(prefix="ecce-nwesp-run-")
        try:
            text = deck(["ES.Theory.NWChem.ESPCharges: 1"], work=work)
            subprocess.run(["mpirun", "-np", "2", "nwchem", "w.nw"], cwd=work,
                           stdout=open(os.path.join(work, "w.out"), "w"),
                           stderr=subprocess.STDOUT)
            import glob; q = open(glob.glob(os.path.join(work, "*.q"))[0]).read()
            p = subprocess.run(["perl", os.path.join(PARSERS, "nwchem.esp")],
                               input=q, cwd=work, capture_output=True, text=True)
            print(p.stdout)
            ok = "key: ESPCHARGE" in p.stdout
            fails += not ok
        finally:
            shutil.rmtree(work, ignore_errors=True)
    return fails


if __name__ == "__main__":
    sys.exit(1 if main("--run" in sys.argv) else 0)
