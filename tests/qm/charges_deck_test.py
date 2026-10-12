#!/usr/bin/env python3
"""ai.ecceqm writes the "charges" line from the Theory Details tick boxes:
only the ticked schemes, and no line at all when none is ticked."""
import os, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
CASES = [({}, None),
         ({"Loewdin": 1}, "charges loewdin"),
         ({"Loewdin": 1, "CHELPG": 0, "MK": 1}, "charges loewdin mk"),
         ({"Loewdin": 1, "CHELPG": 1, "MK": 1}, "charges loewdin chelpg mk")]


def deck(ticks):
    work = tempfile.mkdtemp(prefix="ecce-qmdeck-")
    try:
        shutil.copy(os.path.join(PARSERS, "ecceqm.tpl"), os.path.join(work, "t.tpl"))
        open(os.path.join(work, "w.frag"), "w").write(
            "# frag\nnum_atoms: 3\ntitle: w\natom_info: symbol\natom_list:\n"
            "O 0 0 0.1\nH 0 0.76 -0.47\nH 0 -0.76 -0.47\n")
        open(os.path.join(work, "w.basis"), "w").write("basis 6-31G*\nend\n")
        params = ["Category: SCF", "Theory: HF", "RunType: Energy", "Charge: 0",
                  "ChemSys.Multiplicity: 1"]
        params += ["ES.Theory.ECCEQM.%s: %d" % kv for kv in ticks.items()]
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
    for ticks, want in CASES:
        got = [l for l in deck(ticks).splitlines() if l.startswith("charges")]
        ok = got == ([want] if want else [])
        print("%-4s %s -> %s" % ("ok" if ok else "FAIL", ticks, got))
        fails += not ok
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
