#!/usr/bin/env python3
"""One-atom ORCA Geometry/GeoVib (#623): decks from the real ai.orca and, if ORCA is installed, a real run."""
import glob, os, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
FRAG = "# frag\nnum_atoms: %d\ntitle: x\natom_info: symbol\natom_list:\n%s"
ATOMS = {1: "O 0.0 0.0 0.0\n", 2: "O 0.0 0.0 0.0\nO 0.0 0.0 1.2\n"}
fails = 0


def check(ok, what):
    global fails
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    fails += not ok


def deck(work, runtype, natoms, mult):
    shutil.copy(os.path.join(PARSERS, "orca.tpl"), os.path.join(work, "t.tpl"))
    open(os.path.join(work, "w.frag"), "w").write(FRAG % (natoms, ATOMS[natoms]))
    open(os.path.join(work, "w.basis"), "w").write("useRouteCard def2-SVP\n")
    open(os.path.join(work, "w.param"), "w").write(
        "Category: DFT\nTheory: UDFT\nRunType: %s\nCharge: 0\nChemSys.Multiplicity: %d\n"
        "ES.Theory.DFT.XCFunctionals: B3LYP\nES.Runtype.GeomOpt.ConvergenceGradient: Tight\n"
        "ES.Runtype.GeomOpt.MaximumStepsValue: 7\n" % (runtype, mult))
    p = subprocess.run(["perl", os.path.join(PARSERS, "ai.orca"), "-p", "-f", "-b", "-c",
                        "-n", "w", "-t", "t.tpl"], cwd=work,
                       env=dict(os.environ, ECCE_HOME=ROOT), capture_output=True, text=True)
    if p.returncode:
        raise RuntimeError(p.stderr)
    return open(os.path.join(work, "t.tpl")).read()


def main():
    orca = (glob.glob("/opt/orca/orca_6_1_1*/orca") or [None])[0]
    for rt, want_freq in (("Geometry", False), ("GeoVib", True)):
        with tempfile.TemporaryDirectory() as work:
            text = deck(work, rt, 1, 3)
            bang = [l for l in text.splitlines() if l.startswith("!")][0].split()
            print(rt, "one atom:", " ".join(bang))
            check(not any("OPT" in x.upper() for x in bang), "no Opt keyword")
            check(("FREQ" in [x.upper() for x in bang]) == want_freq, "Freq kept only for GeoVib")
            check("# one atom: no geometry optimisation" in text, "comment in the deck")
            check("%geom" not in text, "no %geom block")
            if orca:
                open(os.path.join(work, "w.inp"), "w").write(text)
                subprocess.run([orca, "w.inp"], cwd=work, stdout=open(os.path.join(work, "w.out"), "w"))
                out = open(os.path.join(work, "w.out")).read()
                check("ORCA TERMINATED NORMALLY" in out, "ORCA terminated normally")
                check("FINAL SINGLE POINT ENERGY" in out, "energy printed")
                check("did not converge" not in out and "THE OPTIMIZATION" not in out,
                      "nothing gensub or orca.desc could read as a failed optimisation")
                check(("Final Gibbs free energy" in out) == want_freq, "Gibbs energy only with Freq")
    for rt in ("Geometry", "GeoVib"):
        with tempfile.TemporaryDirectory() as work:
            text = deck(work, rt, 2, 3)
            bang = [l for l in text.splitlines() if l.startswith("!")][0].split()
            check("Opt" in bang and "one atom" not in text and "%geom" in text,
                  rt + " on two atoms is unchanged: " + " ".join(bang))
    print("FAILED: %d" % fails if fails else "all passed")
    return 1 if fails else 0


sys.exit(main())
