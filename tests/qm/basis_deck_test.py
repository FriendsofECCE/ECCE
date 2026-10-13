#!/usr/bin/env python3
"""The ECCE-QM basis list through both ways a basis reaches the engine.

    basis_deck_test.py [ECCE_QM_EXE]

For 6-31G, 6-31+G*, 6-311G*, cc-pVDZ, aug-cc-pVDZ and cc-pVTZ on water:
 1. the deck ai.ecceqm writes (basis_data, from the library's whole-set file
    as ECCE loads it) has the full number of spherical functions, counted by
    hand from the published contractions (aug-cc-pVDZ: O 4s3p2d, H 3s2p, 41);
 2. with the engine given, the energy from that deck equals the one from the
    bare name in a .qmin, and (HF, water and HCl) the ORCA energy in oracle.json.
"""
import json, os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
BASIS_DIR = os.path.join(ROOT, "data", "admin", "basissets")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from molecules import MOLECULES  # noqa: E402

# basis -> (library file, functions on water)
BASES = {"6-31G": ("6-31G", 13), "6-31+G*": ("6-31PGS-AGG", 22), "6-311G*": ("6-311GS-AGG", 24),
         "cc-pVDZ": ("cc-pVDZ", 24), "aug-cc-pVDZ": ("AUG-CC-PVDZ-AGG", 41), "cc-pVTZ": ("cc-pVTZ", 58)}
NFUNC = {"S": 1, "P": 3, "D": 5, "F": 7}


def library_gbs(path, elements, basis):
    """A .gbs as ECCE stores a loaded set: the shells of the whole-set file."""
    shells, el = {}, None
    lines = open(os.path.join(BASIS_DIR, BASES[basis][0] + ".BAS")).read().splitlines()
    i = 0
    while i < len(lines):
        m = re.match(r"atom=(\w+)", lines[i])
        if m:
            el = m.group(1)
            shells.setdefault(el, [])
        m = re.match(r"contraction shell=(\w+) num_primitives=(\d+)", lines[i])
        if m:
            rows = [lines[i + 1 + k].split() for k in range(int(m.group(2)))]
            shells[el].append((m.group(1), rows))
            i += int(m.group(2))
        i += 1
    with open(path, "w") as out:
        out.write('NameBasis\nbasis "ao basis" spherical print\n')
        for e in elements:
            out.write('%s library "%s"\n' % (e, basis))
        out.write('END\nEndNameBasis\n\nNumericalBasis\nbasis "ao basis" spherical print\n')
        for e in elements:
            for letters, rows in shells[e]:
                # "SS" is two contractions sharing primitives; ECCE stores each alone
                for col, letter in enumerate(letters):
                    out.write("%s %s\n" % (e, letter))
                    for r in rows:
                        if float(r[1 + col]) != 0.0:
                            out.write("  %s  %s\n" % (r[0], r[1 + col]))
        out.write("END\nEndNumericalBasis\n")


def write_deck(work, mol, basis, method):
    env = dict(os.environ, ECCE_HOME=ROOT)
    charge, mult, atoms = MOLECULES[mol]
    elements = sorted({a[0] for a in atoms})
    library_gbs(os.path.join(work, "c.gbs"), elements, basis)
    with open(os.path.join(work, "c.gbs")) as h:
        p = subprocess.run(["perl", os.path.join(PARSERS, "std2ECCEQM")], stdin=h, env=env,
                           capture_output=True, text=True)
    if p.returncode:
        raise RuntimeError(p.stderr)
    open(os.path.join(work, "c.basis"), "w").write(p.stdout)
    shutil.copy(os.path.join(PARSERS, "ecceqm.tpl"), os.path.join(work, "t.tpl"))
    with open(os.path.join(work, "c.frag"), "w") as f:
        f.write("# frag\nnum_atoms: %d\ntitle: c\natom_info: symbol\natom_list:\n" % len(atoms))
        for a in atoms:
            f.write("%s %.10f %.10f %.10f\n" % a)
    params = ["Category: %s" % ("SCF" if method == "hf" else "DFT"),
              "Theory: %s" % ("HF" if method == "hf" else "DFT"), "RunType: Energy",
              "Charge: %d" % charge, "ChemSys.Multiplicity: %d" % mult]
    if method != "hf":
        params.append("ES.Theory.DFT.XCFunctionals: B3LYP")
    open(os.path.join(work, "c.param"), "w").write("\n".join(params) + "\n")
    p = subprocess.run(["perl", os.path.join(PARSERS, "ai.ecceqm"), "-p", "-f", "-b", "-n", "c",
                        "-t", "t.tpl"], cwd=work, capture_output=True, text=True, env=env)
    if p.returncode:
        raise RuntimeError(p.stderr)
    return open(os.path.join(work, "t.tpl")).read()


def nbf(deck, elements):
    """Spherical functions per element from the deck's shell lines."""
    per = {}
    for m in re.finditer(r"^shell (\w+) (\w+)$", deck, re.M):
        per[m.group(1)] = per.get(m.group(1), 0) + NFUNC[m.group(2).upper()]
    return per


def run_engine(exe, text, work, name):
    inp, out = os.path.join(work, name + ".qmin"), os.path.join(work, name + ".qmout")
    open(inp, "w").write(text)
    env = dict(os.environ, ECCE_BASIS_DIR=BASIS_DIR)
    p = subprocess.run([exe, "-o", out, inp], env=env, capture_output=True, text=True)
    if p.returncode:
        raise RuntimeError(p.stderr[-300:])
    for line in open(out):
        if line.startswith("energy_total"):
            return float(line.split()[1])
    raise RuntimeError("no energy in " + out)


def main():
    exe = sys.argv[1] if len(sys.argv) > 1 else None
    oracle = json.load(open(os.path.join(ROOT, "tests", "qm", "oracle.json")))
    fails = 0
    with tempfile.TemporaryDirectory() as work:
        for basis, (_f, want) in BASES.items():
            deck = write_deck(work, "h2o", basis, "hf")
            per = nbf(deck, "OH")
            # the deck lists each element's shells once per atom type
            total = per.get("O", 0) + 2 * per.get("H", 0)
            ok = total == want and ("basis %s\n" % basis) in deck
            print("%-4s deck %-12s water functions %d (expect %d)" % ("ok" if ok else "FAIL", basis, total, want))
            fails += not ok
        if exe:
            for mol in ("h2o", "hcl"):
                for method in ("hf", "b3lyp"):
                    for basis in BASES:
                        deck = write_deck(work, mol, basis, method)
                        e_deck = run_engine(exe, deck, work, "deck")
                        key = "%s_%s_%s_r" % (mol, basis.replace("*", "s").replace("+", "p").lower(), method)
                        ch, mult, atoms = MOLECULES[mol]
                        text = "title b\ncharge %d\nmultiplicity %d\nmethod %s\nbasis %s\ngeometry\n" % (
                            ch, mult, method, basis) + "".join("%s %.10f %.10f %.10f\n" % a for a in atoms) + "end\n"
                        e_bare = run_engine(exe, text, work, "bare")
                        e_orca = oracle[key]["orca"]["energy"] if key in oracle else None
                        tol = 1e-8 if method == "hf" else 1e-5
                        ok = abs(e_deck - e_bare) < 1e-9
                        if e_orca is not None:
                            ok = ok and abs(e_deck - e_orca) < tol and abs(e_bare - e_orca) < tol
                        print("%-4s %-4s %-5s %-12s deck-bare %.1e  deck-ORCA %s" % (
                            "ok" if ok else "FAIL", mol, method, basis, e_deck - e_bare,
                            ("%.1e" % (e_deck - e_orca)) if e_orca is not None else "-"))
                        fails += not ok
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
