#!/usr/bin/env python3
"""ai.mopac writes the ESP keyword only when the Theory Details box is
ticked."""
import os, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")


def deck(extra):
    if not any(e.startswith("RunType:") for e in extra):
        extra = ["RunType: Energy"] + extra
    work = tempfile.mkdtemp(prefix="ecce-mopesp-")
    try:
        shutil.copy(os.path.join(PARSERS, "mopac.tpl"), os.path.join(work, "t.tpl"))
        open(os.path.join(work, "w.frag"), "w").write(
            "# frag\nnum_atoms: 3\ntitle: water\natom_info: symbol\natom_list:\n"
            "O 0.0 0.0 0.1173\nH 0.0 0.7572 -0.4692\nH 0.0 -0.7572 -0.4692\n")
        params = ["Category: SE", "Theory: RPM7", "Charge: 0",
                  "ChemSys.Multiplicity: 1"] + extra
        open(os.path.join(work, "w.param"), "w").write("\n".join(params) + "\n")
        env = dict(os.environ, ECCE_HOME=ROOT)
        p = subprocess.run(["perl", os.path.join(PARSERS, "ai.mopac"), "-p", "-f",
                            "-n", "w", "-t", "t.tpl"], cwd=work, env=env,
                           capture_output=True, text=True)
        if p.returncode:
            raise RuntimeError(p.stderr)
        return open(os.path.join(work, "t.tpl")).read()
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    fails = 0
    for extra, want in (([], False), (["ES.Theory.SCF.ESPCharges: 0"], False),
                        (["ES.Theory.SCF.ESPCharges: 1"], True)):
        first = deck(extra).splitlines()[0].split()
        ok = ("ESP" in first) == want
        print(("ok   " if ok else "FAIL ") + " ".join(first))
        fails += not ok
    return fails


if __name__ == "__main__":
    sys.exit(1 if main() else 0)
