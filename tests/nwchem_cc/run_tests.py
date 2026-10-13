#!/usr/bin/env python3
"""#297: NWChem MP2 and coupled-cluster decks.

Generates decks with the real scripts/parsers/ai.nwchem and checks which
module each theory is written for:

  RMP2 / UMP2        mp2 block,  task mp2         (works in 7.2.3)
  RCCSD / RCCSD(T)   ccsd block, task ccsd(t)     (closed shell only)
  UCCSD, ROCCSD (+(T))  tce block, task tce       (the ccsd module stops with
                                                   "nopen is not zero")

MP and CC decks aborted generation altogether until #297 (the template's dft
block reaches &SCF with a non-SCF category); that is covered first.

With --run the decks also go through the installed nwchem (water and OH,
6-31G*, two processes) and the stdout through nwchem.thermo.
"""
import os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")

MOL = {
    "h2o": ("O 0.0 0.0 0.1173\nH 0.0 0.7572 -0.4692\nH 0.0 -0.7572 -0.4692\n", 3, 10, 1),
    "oh": ("O 0.0 0.0 0.0\nH 0.0 0.0 0.97\n", 2, 9, 2),
}


def deck(mol, category, theory, runtype, extra=(), work=None):
    atoms, n, nel, mult = MOL[mol]
    own = work is None
    work = work or tempfile.mkdtemp(prefix="ecce-nwcc-")
    try:
        tpl = os.path.join(work, "w.nw")
        shutil.copy(os.path.join(PARSERS, "nwch.tpl"), tpl)
        open(os.path.join(work, "w.frag"), "w").write(
            "title: %s\ntype: molecule\nnum_atoms: %d\natom_info: symbol cart\n"
            "atom_list:\n%s" % (mol, n, atoms))
        open(os.path.join(work, "w.basis"), "w").write(
            'basis "ao basis" cartesian print\n  H library "6-31G*"\n'
            '  O library "6-31G*"\nEND\n')
        params = ["title: " + mol, "annotation: " + mol, "parseFile: w.ep",
                  "Category: " + category, "Theory: " + theory,
                  "RunType: " + runtype, "Charge: 0", "Symmetry: C1",
                  "NumElectrons: %d" % nel, "ChemSys.Multiplicity: %d" % mult,
                  "BasisSet.Coordinates: Cartesian",
                  "ES.Theory.SCF.ConvergenceIterations: 30",
                  "ES.Theory.CC.ConvergenceEnergy.Value: 1e-6",
                  "ES.Theory.CC.ConvergenceIterations: 20"] + list(extra)
        open(os.path.join(work, "w.param"), "w").write("\n".join(params) + "\n")
        env = dict(os.environ, ECCE_HOME=ROOT)
        p = subprocess.run(["perl", os.path.join(PARSERS, "ai.nwchem"), "-n", "w",
                            "-p", "-f", "-b", "-t", tpl], cwd=work, env=env,
                           capture_output=True, text=True)
        if p.returncode:
            raise RuntimeError(p.stderr)
        return open(tpl).read()
    finally:
        if own:
            shutil.rmtree(work, ignore_errors=True)


def blocks(text):
    """Top-level block headers and task lines, in order."""
    return [l.split()[0] if not l.startswith("task") else l.strip()
            for l in text.splitlines()
            if re.match(r"^(scf|dft|mp2|ccsd|tce|task)\b", l)]


FZ = ["ES.Theory.UseFrozenCores: 1", "ES.Theory.CorrelOrb.FrozenCoreValue: 1"]
SYM = ["ES.Theory.UseSymmetry: 1"]

# (molecule, category, theory, runtype, extra, expected blocks/tasks, text that must appear)
CASES = [
    ("h2o", "MP", "RMP2", "Energy", [], ["scf", "mp2", "task mp2 energy"], ["  rhf"]),
    ("oh", "MP", "UMP2", "Gradient", [], ["scf", "mp2", "task mp2 gradient"], ["  uhf"]),
    ("h2o", "CC", "RCCSD", "Energy", FZ, ["scf", "ccsd", "task ccsd energy"], ["freeze core 1", "thresh 1e-6"]),
    ("h2o", "CC", "RCCSD(T)", "Energy", [], ["scf", "ccsd", "task ccsd(t) energy"], []),
    ("h2o", "CC", "RCCSD", "Gradient", [], ["scf", "ccsd", "task ccsd energy", "task ccsd gradient"], []),
    ("oh", "CC", "UCCSD", "Energy", FZ, ["scf", "tce", "task tce energy"],
     ["  uhf", "nopen 1", "  ccsd\n", "freeze core 1", "thresh 1e-6", "maxiter 20"]),
    ("oh", "CC", "UCCSD(T)", "Energy", [], ["scf", "tce", "task tce energy"], ["  uhf", "  ccsd(t)\n"]),
    ("oh", "CC", "ROCCSD", "Energy", [], ["scf", "tce", "task tce energy"], ["  rohf", "nopen 1"]),
    ("oh", "CC", "ROCCSD(T)", "Energy", [], ["scf", "tce", "task tce energy"], ["  rohf", "  ccsd(t)\n"]),
    # numerical gradients/frequencies inside TCE: no extra energy task needed
    ("oh", "CC", "UCCSD", "Gradient", [], ["scf", "tce", "task tce gradient"], []),
    ("oh", "CC", "UCCSD", "Geometry", [], ["scf", "tce", "task tce optimize"], []),
    ("oh", "CC", "UCCSD", "Vibration", [], ["scf", "tce", "task tce freq"], []),
]


def main(run):
    fails = 0

    def check(ok, label):
        nonlocal fails
        print(("ok   " if ok else "FAIL ") + label)
        fails += not ok

    for mol, cat, th, rt, extra, want, must in CASES:
        try:
            t = deck(mol, cat, th, rt, extra)
        except RuntimeError as e:
            check(False, "%s %s %s: generation aborted: %s" % (mol, th, rt, e))
            continue
        got = blocks(t)
        check(got == want and all(m in t for m in must),
              "%s %s %s: %s" % (mol, th, rt, " / ".join(got)))

    # TCE aborts for a non-Abelian point group, so it never gets autosym
    # even when the Theory Details box asks for it; the ccsd module may.
    t = deck("oh", "CC", "UCCSD", "Energy", SYM)
    check("noautosym" in t and not re.search(r"\bautosym", t), "TCE deck turns automatic symmetry off")
    t = deck("h2o", "CC", "RCCSD", "Energy", SYM)
    check(re.search(r"geometry autosym", t) is not None, "ccsd-module deck keeps the user's symmetry")

    # the open-shell reference of an RO theory is ROHF, not RHF
    check("  rohf" in deck("oh", "CC", "ROCCSD", "Energy"), "RO* is ROHF")

    if run:
        fails += run_nwchem()
    return fails


# Expected totals, 6-31G*, from real NWChem 7.2.3 (Eh).
EXPECT = [
    ("h2o", "MP", "RMP2", "Energy", ["Total MP2 energy"], -76.19902659),
    ("h2o", "CC", "RCCSD", "Energy", ["Total CCSD energy"], -76.20782186),
    ("oh", "CC", "UCCSD", "Energy", ["CCSD total energy / hartree"], -75.53762239),
    ("oh", "CC", "ROCCSD(T)", "Energy", ["CCSD(T) total energy / hartree"], -75.53899568),
]


def run_nwchem():
    fails = 0
    for mol, cat, th, rt, pats, ref in EXPECT:
        work = tempfile.mkdtemp(prefix="ecce-nwcc-run-")
        try:
            t = deck(mol, cat, th, rt, work=work)
            r = subprocess.run(["mpirun", "-np", "2", "nwchem", "w.nw"], cwd=work,
                               capture_output=True, text=True, timeout=900)
            out = r.stdout
            m = None
            for pat in pats:
                for l in out.splitlines():
                    if pat in l:
                        m = re.search(r"(-\d+\.\d+)\s*$", l) or m
            ok = r.returncode == 0 and m is not None and abs(float(m.group(1)) - ref) < 2e-6
            print(("ok   " if ok else "FAIL ") + "run %s %s: %s (expect %.8f)" %
                  (mol, th, m.group(1) if m else "no energy", ref))
            fails += not ok
            if cat == "CC" and th[0] in "UR" and "T" not in th and mol == "oh":
                p = subprocess.run(["perl", os.path.join(PARSERS, "nwchem.thermo")],
                                   input=out, capture_output=True, text=True)
                ok = "key: ECCSD\n" in p.stdout and "key: CORRELTN" in p.stdout
                print(("ok   " if ok else "FAIL ") + "nwchem.thermo reads the run")
                fails += not ok
        finally:
            shutil.rmtree(work, ignore_errors=True)
    return fails


if __name__ == "__main__":
    sys.exit(1 if main("--run" in sys.argv) else 0)
