"""Deck generation helpers shared by run_tests.py (no Gaussian/ORCA needed)."""
import os, shutil, subprocess, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")


def deck(code, params="", runtype="Geometry"):
    """code is 'gauss16' or 'orca'; returns the generated input deck."""
    tpl = {"gauss16": "g16.tpl", "orca": "orca.tpl"}[code]
    work = tempfile.mkdtemp(prefix="ecce-opthess-")
    try:
        shutil.copy(os.path.join(PARSERS, tpl), os.path.join(work, "t.tpl"))
        open(os.path.join(work, "w.frag"), "w").write(
            "# frag\nnum_atoms: 3\ntitle: h2o\natom_info: symbol\natom_list:\n"
            "O 0.0 0.0 0.1\nH 0.0 0.75 -0.5\nH 0.0 -0.75 -0.5\n")
        open(os.path.join(work, "w.basis"), "w").write("useRouteCard STO-3G\n")
        open(os.path.join(work, "w.param"), "w").write(
            "Category: SCF\nTheory: RHF\nRunType: %s\nCharge: 0\n"
            "ChemSys.Multiplicity: 1\n%s" % (runtype, params))
        env = dict(os.environ, ECCE_HOME=ROOT)
        p = subprocess.run(["perl", os.path.join(PARSERS, "ai." + code), "-p", "-f",
                            "-b", "-c", "-n", "w", "-t", "t.tpl"], cwd=work, env=env,
                           capture_output=True, text=True)
        if p.returncode:
            raise RuntimeError(p.stderr)
        return open(os.path.join(work, "t.tpl")).read()
    finally:
        shutil.rmtree(work, ignore_errors=True)
