#!/usr/bin/env python3
"""ORCA route line for numerical frequencies (#1058): with Frequencies Method
Numerical the line carries NumFreq alone, because ORCA 6.1.1 takes the analytic
path whenever Freq is also there (B3LYP Raman then stops at the input check).
Analytic keeps Freq.  Generator only, no ORCA needed.

    tests/parsers/orca_numfreq_route_test.py
"""
import os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(REPO, "scripts", "parsers")


def route(runtype, method):
    work = tempfile.mkdtemp(prefix="ecce-orca-numfreq-")
    try:
        shutil.copy(os.path.join(PARSERS, "orca.tpl"), os.path.join(work, "t.tpl"))
        with open(os.path.join(work, "w.frag"), "w") as f:
            f.write("# frag\nnum_atoms: 3\ntitle: water\natom_info: symbol\natom_list:\n"
                    "O 0.0 0.0 0.117\nH 0.0 0.757 -0.467\nH 0.0 -0.757 -0.467\n")
        with open(os.path.join(work, "w.basis"), "w") as f:
            f.write("useRouteCard def2-SVP\n")
        with open(os.path.join(work, "w.param"), "w") as f:
            f.write("Category: DFT\nTheory: RDFT\nRunType: %s\nCharge: 0\n"
                    "ChemSys.Multiplicity: 1\nES.Theory.DFT.XCFunctionals: B3LYP\n"
                    "ES.Runtype.Vibration.UseRaman: 1\n"
                    "ES.Runtype.Vibration.Method: %s\n" % (runtype, method))
        p = subprocess.run(["perl", os.path.join(PARSERS, "ai.orca"), "-p", "-f", "-b",
                            "-n", "w", "-t", "t.tpl"], cwd=work, capture_output=True,
                           text=True, env=dict(os.environ, ECCE_HOME=REPO))
        deck = open(os.path.join(work, "t.tpl")).read()   # filled in place
        lines = [l for l in deck.splitlines() if l.startswith("!")]
        return " ".join(lines[0].split()) if lines else "(no route line: %s)" % p.stderr[-300:]
    finally:
        shutil.rmtree(work, ignore_errors=True)


failures = 0
for runtype, method, want, notwant in (
        ("GeoVib", "Numerical", r"\bOpt NumFreq\b", r"\bFreq\b"),
        ("Vibration", "Numerical", r"\bNumFreq\b", r"\bFreq\b"),
        ("GeoVib", "Analytic", r"\bOpt Freq\b", r"NumFreq"),
        ("Vibration", "Analytic", r"\bFreq\b", r"NumFreq")):
    r = route(runtype, method)
    ok = re.search(want, r) and not re.search(notwant, r)
    print("  %s %-9s %-9s %s" % ("ok  " if ok else "FAIL", runtype, method, r))
    failures += not ok
print("PASS" if not failures else "FAIL %d" % failures)
sys.exit(1 if failures else 0)
