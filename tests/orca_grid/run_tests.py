#!/usr/bin/env python3
"""ORCA integration grid as its own setting (#644): decks from the real ai.orca."""
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
        g = lambda v: "ES.Theory.DFT.Grid: %s\n" % v
        r = route(work, g("Default"))
        check(not any("DEFGRID" in x for x in r), "Default writes no keyword: " + " ".join(r))
        r = route(work, "")
        check(not any("DEFGRID" in x for x in r), "no setting stored writes no keyword")
        for n in "123":
            r = route(work, g("DefGrid" + n))
            check(r.count("DEFGRID" + n) == 1 and "B3LYP" in r, "DefGrid%s -> DEFGRID%s once: %s" % (n, n, " ".join(r)))
        r = route(work, g("DefGrid3") + "ES.Theory.SCF.Dispersion: D3BJ\n")
        check("DEFGRID3" in r and "D3BJ" in r, "grid and dispersion are separate keywords")
        r = route(work, g("DefGrid3"), cat="MP2", theory="RMP2")
        check(not any("DEFGRID" in x for x in r), "not written for MP2")
        r = route(work, g("DefGrid3"), cat="SCF", theory="RHF")
        check(not any("DEFGRID" in x for x in r), "not written for HF")
        # Legacy: the grid (and dispersion) live in the functional name.
        for name, base, kws in (("PBE0 DEFGRID3", "PBE0", ["DEFGRID3"]),
                                ("PBE0 DEFGRID3 D3BJ", "PBE0", ["DEFGRID3", "D3BJ"]),
                                ("B3LYP-D3 DEFGRID2", "B3LYP", ["DEFGRID2", "D3ZERO"])):
            r = route(work, "", func=name)
            check(sorted(r) == sorted(["!", "RKS", base, "def2-SVP"] + kws),
                  "legacy %r -> %s + %s: %s" % (name, base, kws, " ".join(r)))
        r = route(work, g("DefGrid1"), func="PBE0 DEFGRID3")
        check(r.count("DEFGRID1") == 1 and "DEFGRID3" not in r, "the setting wins over a legacy name")
        # Three words: an unsplit name would echo the same keywords, so the
        # stored setting must replace the name's grid.
        r = route(work, g("DefGrid1"), func="PBE0 DEFGRID3 D3BJ")
        check(sorted(r) == sorted(["!", "RKS", "PBE0", "def2-SVP", "DEFGRID1", "D3BJ"]),
              "the setting wins over a three-word legacy name: " + " ".join(r))
        r = route(work, "", func="B97-D3 DEFGRID2")
        check(sorted(r) == sorted(["!", "RKS", "B97-D3", "def2-SVP", "DEFGRID2"]),
              "B97-D3 keeps its D3 behind a grid: " + " ".join(r))
        r = route(work, "", func="B97-D3")
        check("B97-D3" in r and not any("DEFGRID" in x for x in r), "B97-D3 is still a functional")
    print("FAILED: %d" % fails if fails else "all passed")
    return 1 if fails else 0


sys.exit(main())
