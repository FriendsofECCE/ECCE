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
        # RI mode and the old checkbox must survive both direct generation
        # and migration to an explicit None choice.
        modekey = "ES.Theory.SCF.RIMode: %s\n"
        oldkey = "ES.Theory.SCF.UseRIJCOSX: %s\n"
        for cat, theory in (("DFT", "RDFT"), ("SCF", "RHF")):
            cases = [("", "None"), (oldkey % "0", "None"),
                     (oldkey % "1", "RIJCOSX")]
            for mode in ("None", "RIJCOSX", "RIJONX"):
                cases.extend([(modekey % mode, mode),
                              ((oldkey % "1") + (modekey % mode), mode)])
            for extra, expected in cases:
                r = route(work, extra, cat=cat, theory=theory)
                ri = [x for x in r if x in ("RIJCOSX", "RIJONX")]
                check(ri == ([] if expected == "None" else [expected]) and
                      r.count("def2/J") == (expected != "None"),
                      "%s RI %r -> %s" % (cat, extra.strip(), " ".join(r)))
            for mode in ("RIJCOSX", "RIJONX"):
                r = route(work, (modekey % mode) +
                          "ES.Theory.SCF.RIJCOSXAuxBasis: def2/JK\n",
                          cat=cat, theory=theory)
                check(mode in r and "def2/JK" in r and "def2/J" not in r,
                      "%s %s uses the selected auxiliary basis" % (cat, mode))
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
        # -3c composites (#772): the keyword alone, no basis, no D3/D4.
        for comp in ("r2SCAN-3c", "B97-3c", "PBEh-3c", "wB97X-3c"):
            r = route(work, d("D4"), func=comp)
            check(comp in r and "def2-SVP" not in r and "D4" not in r and "D3BJ" not in r
                  and "D3ZERO" not in r, "%s alone: %s" % (comp, " ".join(r)))
        r = route(work, "", func="HF-3c")
        check("HF-3c" in r and "RKS" not in r and "def2-SVP" not in r, "HF-3c alone: " + " ".join(r))
    print("FAILED: %d" % fails if fails else "all passed")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
