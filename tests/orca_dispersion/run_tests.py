#!/usr/bin/env python3
"""ORCA dispersion as its own setting (#593): decks from the real ai.orca."""
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


def route(work, extra, func="B3LYP", cat="DFT", theory="RDFT"):
    shutil.copy(os.path.join(PARSERS, "orca.tpl"), os.path.join(work, "t.tpl"))
    open(os.path.join(work, "w.frag"), "w").write(H2O)
    open(os.path.join(work, "w.basis"), "w").write("useRouteCard def2-SVP\n")
    open(os.path.join(work, "w.param"), "w").write(
        "Category: %s\nTheory: %s\nRunType: Energy\nCharge: 0\n"
        "ChemSys.Multiplicity: 1\nES.Theory.DFT.XCFunctionals: %s\n%s"
        % (cat, theory, func, extra))
    p = subprocess.run(["perl", os.path.join(PARSERS, "ai.orca"), "-p", "-f",
                        "-b", "-c", "-n", "w", "-t", "t.tpl"], cwd=work,
                       env=dict(os.environ, ECCE_HOME=ROOT),
                       capture_output=True, text=True)
    if p.returncode:
        raise RuntimeError(p.stderr)
    return [l for l in open(os.path.join(work, "t.tpl")).read().splitlines()
            if l.startswith("!")][0].split()


def main():
    with tempfile.TemporaryDirectory() as work:
        d = lambda v: "ES.Theory.SCF.Dispersion: %s\n" % v
        r = route(work, d("None"))
        check("D3BJ" not in r and "D4" not in r and "D3ZERO" not in r, "None writes no keyword")
        for choice, kw in (("D3BJ", "D3BJ"), ("D3(0)", "D3ZERO"), ("D4", "D4")):
            r = route(work, d(choice))
            check(r.count(kw) == 1 and "RKS" in r and "B3LYP" in r, "DFT %s -> %s once: %s" % (choice, kw, " ".join(r)))
        r = route(work, d("D3BJ"), cat="SCF", theory="RHF")
        check("HF" in r and r.count("D3BJ") == 1, "HF takes D3BJ: " + " ".join(r))
        r = route(work, d("D4"), cat="MP2", theory="RMP2")
        check("D4" not in r, "not written for MP2")
        # Legacy: the dispersion lives in the functional name.
        for name, base, kw in (("PBE0 D3BJ", "PBE0", "D3BJ"), ("B3LYP-D3", "B3LYP", "D3ZERO"),
                               ("PBE0 D4", "PBE0", "D4")):
            r = route(work, "", func=name)
            check(base in r and r.count(kw) == 1 and name not in " ".join(r), "legacy %r -> %s + %s" % (name, base, kw))
        r = route(work, d("D3BJ"), func="PBE0 D3BJ")
        check(r.count("D3BJ") == 1, "legacy name plus the setting is not doubled")
        r = route(work, "", func="wB97X-D3")
        check("wB97X-D3" in r and "D3ZERO" not in r, "wB97X-D3 is a functional, not split")
        r = route(work, "", func="B97-D3")
        check("B97-D3" in r and "D3ZERO" not in r, "B97-D3 is a functional, not split")
    print("FAILED: %d" % fails if fails else "all passed")
    return 1 if fails else 0


sys.exit(main())
