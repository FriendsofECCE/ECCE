#!/usr/bin/env python3
"""ai.ecceqm writes "raman true" from the Runtype Details tick box, for the
frequency run types only and never otherwise."""
import os, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
CASES = [("Vibration", "1", "raman true"),
         ("GeoVib", "1", "raman true"),
         ("Vibration", "0", None),
         ("Vibration", None, None),
         ("Energy", "1", None),
         ("Geometry", "1", None)]


def deck(runtype, raman):
    work = tempfile.mkdtemp(prefix="ecce-qmdeck-")
    try:
        shutil.copy(os.path.join(PARSERS, "ecceqm.tpl"), os.path.join(work, "t.tpl"))
        open(os.path.join(work, "w.frag"), "w").write(
            "# frag\nnum_atoms: 3\ntitle: w\natom_info: symbol\natom_list:\n"
            "O 0 0 0.1\nH 0 0.76 -0.47\nH 0 -0.76 -0.47\n")
        open(os.path.join(work, "w.basis"), "w").write("basis 6-31G*\nend\n")
        params = ["Category: SCF", "Theory: HF", "RunType: " + runtype, "Charge: 0",
                  "ChemSys.Multiplicity: 1"]
        if raman is not None:
            params.append("ES.Runtype.Vibration.UseRaman: " + raman)
        open(os.path.join(work, "w.param"), "w").write("\n".join(params) + "\n")
        p = subprocess.run(["perl", os.path.join(PARSERS, "ai.ecceqm"), "-p", "-f", "-b",
                            "-n", "w", "-t", "t.tpl"], cwd=work, capture_output=True,
                           text=True, env=dict(os.environ, ECCE_HOME=ROOT))
        if p.returncode:
            raise RuntimeError(p.stderr)
        return open(os.path.join(work, "t.tpl")).read()
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    fails = 0
    for runtype, raman, want in CASES:
        got = [l for l in deck(runtype, raman).splitlines() if l.startswith("raman")]
        ok = got == ([want] if want else [])
        print("%-4s %s UseRaman=%s -> %s" % ("ok" if ok else "FAIL", runtype, raman, got))
        fails += not ok
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
