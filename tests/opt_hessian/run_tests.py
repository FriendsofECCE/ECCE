#!/usr/bin/env python3
"""Initial force constants for geometry optimisations and TS searches (#537).

Generates decks only (no Gaussian or ORCA needed): the default deck must be
byte-for-byte what it was before the controls existed, each option must add
exactly its keyword, and a TS search must not carry two sources of force
constants.  Also checks that the input checker warns about ReadFC without a
checkpoint."""
import os, subprocess, sys, tempfile

from gen import deck, ROOT

H = "ES.Runtype.GeomOpt.InitialHessian: %s\n"
R = "ES.Runtype.GeomOpt.RecalcFC: %s\n"
TS = "SearchFor: Transition State\n"
fails = 0


def route(d):
    return [l for l in d.splitlines() if l.startswith("#")][0]


def oroute(d):
    return [l for l in d.splitlines() if l.startswith("!")][0]


def geom_block(d):
    out, on = [], False
    for l in d.splitlines():
        if l.startswith("%geom"):
            on = True
        if on:
            out.append(l)
            if l == "end":
                break
    return "\n".join(out)


def check(ok, what, got=""):
    global fails
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        print("       got: %s" % got)
        fails += 1


def g16():
    print("Gaussian 16")
    opt = lambda p, rt="Geometry": route(deck("gauss16", p, rt)).split("Opt=")[1].split(")")[0] + ")"
    print("   default            ", route(deck("gauss16")))
    check(opt("") == "()", "default: Opt=()", opt(""))
    check(opt(H % "Valence Force Field") == "()", "Valence Force Field writes nothing")
    check(opt(H % "Calculate") == "(CalcFC )", "Calculate -> CalcFC", opt(H % "Calculate"))
    check(opt(H % "Calculate at every step") == "(CalcAll )", "CalcAll", opt(H % "Calculate at every step"))
    check(opt(H % "Read from checkpoint") == "(ReadFC )", "ReadFC", opt(H % "Read from checkpoint"))
    check(opt(R % "5") == "(RecalcFC=5 )", "RecalcFC=5", opt(R % "5"))
    check(opt(R % "0") == "()", "RecalcFC 0 writes nothing")
    x = opt("ES.Runtype.GeomOpt.MaximumStepsValue: 30\n" + H % "Calculate" + R % "4")
    check(x == "(MaxCycle=30 CalcFC RecalcFC=4 )", "MaxCycle + CalcFC + RecalcFC", x)
    # TS: the chosen source replaces calcfc, so the default asks for CalcFC once.
    print("   TS default         ", route(deck("gauss16", TS + H % "Calculate")))
    check(opt(TS + H % "Calculate") == "(TS noeigen CalcFC )", "TS default, CalcFC once",
          opt(TS + H % "Calculate"))
    x = opt(TS + H % "Calculate at every step")
    check(x == "(TS noeigen CalcAll )" and "calcfc" not in x.lower(), "TS + CalcAll, no calcfc", x)
    x = opt(TS + H % "Read from checkpoint" + R % "3")
    check(x == "(TS noeigen ReadFC RecalcFC=3 )", "TS + ReadFC + RecalcFC", x)


def orca():
    print("ORCA")
    d = deck("orca")
    check("Calc_Hess" not in d and "Recalc_Hess" not in d, "default: no hessian keywords")
    d = deck("orca", H % "Default")
    check("Calc_Hess" not in d, "Default writes nothing")
    g = geom_block(deck("orca", H % "Calculate"))
    check(g == "%geom\n  Calc_Hess true\nend", "Calculate -> Calc_Hess true", g)
    g = geom_block(deck("orca", R % "5"))
    check(g == "%geom\n  Recalc_Hess 5\nend", "Recalc_Hess 5", g)
    g = geom_block(deck("orca", "ES.Runtype.GeomOpt.MaximumStepsValue: 40\n" + H % "Calculate" + R % "5"))
    check(g == "%geom\n  MaxIter 40\n  Calc_Hess true\n  Recalc_Hess 5\nend", "MaxIter + both", g)
    check("Recalc_Hess" not in deck("orca", R % "0"), "Recalc 0 writes nothing")
    # #557: Find Minimum / Transition State and reading a Hessian file.
    base = deck("orca")
    check(deck("orca", "SearchFor: Minimum\n") == base, "Minimum: deck unchanged")
    check(deck("orca", H % "Read from previous calculation") == base,
          "Read without an attached file: deck unchanged")
    x = oroute(deck("orca", TS + H % "Calculate"))
    check("OptTS" in x.split() and "Opt" not in x.split(), "TS -> ! OptTS", x)
    x = oroute(deck("orca", TS, "GeoVib"))
    check("OptTS" in x.split() and "Freq" in x.split(), "TS GeoVib -> OptTS Freq", x)
    F = "ES.Runtype.GeomOpt.HessianFile: inhess.hess\n"
    g = geom_block(deck("orca", H % "Read from previous calculation" + F))
    check(g == '%geom\n  inhess read\n  InHessName "inhess.hess"\nend',
          "Read -> inhess read + InHessName", g)
    check("inhess" not in deck("orca", H % "Calculate" + F).lower(),
          "a file without the Read choice is ignored")
    # A workflow water step: the Hessian file is declared as an auxiliary
    # input, which is what makes Launch stage it into the run directory.
    edml = open(os.path.join(ROOT, "data", "client", "cap", "ORCA.edml")).read()
    check('<Input   type="auxiliary" mimetype="chemical/x-orca-hess">inhess.hess</Input>' in edml,
          "ORCA.edml declares inhess.hess as an auxiliary input")
    g = geom_block(deck("orca", H % "Read from previous calculation" + F +
                        "ES.Runtype.GeomOpt.MaximumStepsValue: 100\n"))
    check('inhess read' in g and 'InHessName "inhess.hess"' in g and "MaxIter 100" in g,
          "Read + MaxIter: both in %geom", g)


def checker():
    print("Input checker")
    base = ("%%Chk=g16.chk\n%s#P rhf/STO-3G Opt=(ReadFC )\n\ntitle\n\n0 1\n"
            "O 0.0 0.0 0.1\nH 0.0 0.75 -0.5\nH 0.0 -0.75 -0.5\n\n")
    for old, want in (("", True), ("%OldChk=guess.chkin\n", False)):
        with tempfile.NamedTemporaryFile("w", suffix=".g16in", delete=False) as f:
            f.write(base % old)
        p = subprocess.run(["perl", os.path.join(ROOT, "scripts", "parsers", "verifyinput"),
                            "-f", f.name, "-c", "Gaussian-16"], capture_output=True, text=True,
                           env=dict(os.environ, ECCE_HOME=ROOT))
        os.unlink(f.name)
        warn = [l for l in p.stdout.splitlines() if "ReadFC" in l]
        bad = [l for l in p.stdout.splitlines() if l.startswith("BAD") or "|keywords|" in l]
        check(bool(warn) == want and not bad,
              "ReadFC %s %%OldChk: %s" % ("without" if want else "with",
                                          "UNSURE warning" if want else "no warning"),
              p.stdout)


if __name__ == "__main__":
    g16(); orca(); checker()
    sys.exit(1 if fails else 0)
