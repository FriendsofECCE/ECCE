#!/usr/bin/env python3
"""Custom solvent (#642): ORCA and Gaussian 16 decks from the real generators,
and the SOLVMODEL parsers on the output lines each code prints for it."""
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


def deck(gen, tpl, basis, params):
    work = tempfile.mkdtemp(prefix="ecce-custsolv-")
    try:
        shutil.copy(os.path.join(PARSERS, tpl), os.path.join(work, "t.tpl"))
        open(os.path.join(work, "w.frag"), "w").write(H2O)
        open(os.path.join(work, "w.basis"), "w").write("useRouteCard %s\n" % basis)
        open(os.path.join(work, "w.param"), "w").write(
            "Category: SCF\nTheory: RHF\nRunType: Energy\nCharge: 0\n"
            "ChemSys.Multiplicity: 1\n" + params)
        p = subprocess.run(["perl", os.path.join(PARSERS, gen), "-p", "-f",
                            "-b", "-c", "-n", "w", "-t", "t.tpl"], cwd=work,
                           env=dict(os.environ, ECCE_HOME=ROOT),
                           capture_output=True, text=True)
        if p.returncode:
            return None, p.stderr
        return open(os.path.join(work, "t.tpl")).read(), ""
    finally:
        shutil.rmtree(work, ignore_errors=True)


def parse(script, text):
    return subprocess.run(["perl", os.path.join(PARSERS, script)], input=text,
                          capture_output=True, text=True).stdout


def main():
    orca = ("ES.Theory.SCF.UseSolvation: 1\nES.Theory.SCF.SolvationModel: %s\n"
            "ES.Theory.SCF.Solvent: %s\nES.Theory.SCF.Dielectric: 30.0\n"
            "ES.Theory.SCF.RefractiveIndex: 1.42\n")
    d, _ = deck("ai.orca", "orca.tpl", "def2-SVP", orca % ("CPCM", "Custom"))
    bang = [l for l in d.splitlines() if l.startswith("!")][0].split()
    check("CPCM" in bang and not any("CPCM(" in x for x in bang), "ORCA route has plain CPCM: " + " ".join(bang))
    check("%cpcm\n  epsilon 30.0\n  refrac 1.42\nend\n" in d, "ORCA %cpcm block with epsilon and refrac")
    d, _ = deck("ai.orca", "orca.tpl", "def2-SVP", orca % ("CPCM", "water"))
    check("CPCM(water)" in d and "epsilon" not in d, "named solvent unchanged")
    d, err = deck("ai.orca", "orca.tpl", "def2-SVP", orca % ("SMD", "Custom"))
    check(d is None and "CPCM" in err, "Custom with SMD is refused")

    g = ("ES.Theory.SCF.UseSCRF: 1\nES.Theory.SCF.SCRF: CPCM\n"
         "ES.Theory.SCF.Solvent: %s\nES.Theory.SCF.Dielectric: 30.0\n"
         "ES.Theory.SCF.RefractiveIndex: 1.42\n")
    d, _ = deck("ai.gauss16", "g16.tpl", "STO-3G", g % "Custom")
    check("SCRF=(CPCM,Solvent=Generic,Read)" in d, "G16 route Solvent=Generic,Read")
    check("\nEps=30.0\nEpsInf=2.016400\n\n" in d, "G16 input section Eps and EpsInf = n^2")
    # Gaussian ends the input at a second blank line, before the section.
    check("0.59\n\nEps=" in d, "one blank line between the geometry and Eps")
    d, _ = deck("ai.gauss16", "g16.tpl", "STO-3G", g % "Manual")
    check("Solvent=Generic,Read" in d and "Eps=30.0" in d, "old Manual means Custom")
    d, _ = deck("ai.gauss16", "g16.tpl", "STO-3G", g % "Water")
    check("Solvent=Water" in d and "Eps=" not in d, "named solvent unchanged")

    out = parse("orca.solvmodel",
                "  Epsilon    ...      30.0000\n  Refrac     ...       1.4200\n"
                "  Epsilon function type   ...  CPCM\nSolvent:     ... CUSTOM\n")
    check("CPCM, Custom (ε = 30.0, n = 1.42)" in out, "ORCA SOLVMODEL")
    out = parse("gaussian-16.solvmodel",
                " Model                : C-PCM.\n Solvent              : Generic,\n"
                "                        Eps                           =  30.000000\n"
                "                        Eps(infinity)                 =   2.016400\n")
    check("C-PCM, Custom (ε = 30.0, n = 1.42)" in out, "G16 SOLVMODEL")
    print("FAILED: %d" % fails if fails else "all passed")
    return 1 if fails else 0


sys.exit(main())
