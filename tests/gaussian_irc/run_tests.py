#!/usr/bin/env python3
"""Gaussian 16 IRC route (#688): keywords appear only for non-default settings."""
import os, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
H2O = ("# frag\nnum_atoms: 3\ntitle: h2o\natom_info: symbol\natom_list:\n"
       "O 0.0 0.0 0.0\nH 0.0 0.76 0.59\nH 0.0 -0.76 0.59\n")
fails = 0


def check(ok, what):
    global fails
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    fails += not ok


def route(extra):
    work = tempfile.mkdtemp(prefix="ecce-irc-")
    try:
        shutil.copy(os.path.join(PARSERS, "g16.tpl"), os.path.join(work, "t.tpl"))
        open(os.path.join(work, "w.frag"), "w").write(H2O)
        open(os.path.join(work, "w.basis"), "w").write("useRouteCard STO-3G\n")
        open(os.path.join(work, "w.param"), "w").write(
            "Category: SCF\nTheory: RHF\nRunType: IRC\nCharge: 0\n"
            "ChemSys.Multiplicity: 1\n" + extra)
        p = subprocess.run(["perl", os.path.join(PARSERS, "ai.gauss16"), "-p", "-f",
                            "-b", "-c", "-n", "w", "-t", "t.tpl"], cwd=work,
                           env=dict(os.environ, ECCE_HOME=ROOT),
                           capture_output=True, text=True)
        if p.returncode:
            print(p.stdout + p.stderr)
            return ""
        return open(os.path.join(work, "t.tpl")).read()
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    d = route("")
    check("IRC=(calcfc) " in d or "IRC=(calcfc)\n" in d, "default: IRC=(calcfc)")
    check("MaxPoints" not in d and "StepSize" not in d, "default: no extra keywords")
    d = route("ES.Runtype.IRC.MaximumStepsValue: 12\nES.Runtype.IRC.StepSizeValue: 5\n")
    check("IRC=(calcfc,MaxPoints=12,StepSize=5)" in d, "IRC=(calcfc,MaxPoints=12,StepSize=5)")
    d = route("ES.Runtype.IRC.Direction: Reverse\n")
    check("IRC=(calcfc,Reverse)" in d, "IRC=(calcfc,Reverse)")
    d = route("ES.Runtype.IRC.Direction: Forward\nES.Runtype.IRC.MaximumStepsValue: 10\n")
    check("IRC=(calcfc,Forward)" in d, "IRC=(calcfc,Forward), 10 points is the default")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
