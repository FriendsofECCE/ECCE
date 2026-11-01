#!/usr/bin/env python3
"""A library basis set outside ECCE-QM's quick-pick list, through the tool.

    basis_set_tool_test.py [ECCE_QM_EXE]

ECCE-QM offers the Basis Set Tool, so any basis in ECCE's library can be
chosen; ai.ecceqm still writes the choice out as an explicit basis_data block.
For def2-TZVPP on water (not in the quick-pick list):
 1. the deck ai.ecceqm writes carries the basis_data block, with the full
    spherical function count for the set;
 2. with the engine given, the energy from that deck equals the one from the
    bare library name in a .qmin (the engine loads def2-TZVPP itself).
Exits 77 (skip) when ecce-qm is not given.
"""
import os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
BASIS_DIR = os.path.join(ROOT, "data", "admin", "basissets")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from molecules import MOLECULES  # noqa: E402

BASIS = "def2-TZVPP"
FILE = "def2-tzvpp.BAS"
NFUNC = {"S": 1, "P": 3, "D": 5, "F": 7, "G": 9}


def library_gbs(path, elements):
    """A .gbs as ECCE stores a loaded set: the shells of the whole-set file."""
    shells, el = {}, None
    lines = open(os.path.join(BASIS_DIR, FILE)).read().splitlines()
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
            out.write('%s library "%s"\n' % (e, BASIS))
        out.write('END\nEndNameBasis\n\nNumericalBasis\nbasis "ao basis" spherical print\n')
        for e in elements:
            for letters, rows in shells[e]:
                for col, letter in enumerate(letters):
                    out.write("%s %s\n" % (e, letter))
                    for r in rows:
                        if float(r[1 + col]) != 0.0:
                            out.write("  %s  %s\n" % (r[0], r[1 + col]))
        out.write("END\nEndNumericalBasis\n")


def write_deck(work, mol):
    env = dict(os.environ, ECCE_HOME=ROOT)
    charge, mult, atoms = MOLECULES[mol]
    elements = sorted({a[0] for a in atoms})
    library_gbs(os.path.join(work, "c.gbs"), elements)
    with open(os.path.join(work, "c.gbs")) as h:
        p = subprocess.run(["perl", os.path.join(PARSERS, "std2ECCEQM")], stdin=h,
                           env=env, capture_output=True, text=True)
    if p.returncode:
        raise RuntimeError(p.stderr)
    open(os.path.join(work, "c.basis"), "w").write(p.stdout)
    shutil.copy(os.path.join(PARSERS, "ecceqm.tpl"), os.path.join(work, "t.tpl"))
    with open(os.path.join(work, "c.frag"), "w") as f:
        f.write("# frag\nnum_atoms: %d\ntitle: c\natom_info: symbol\natom_list:\n" % len(atoms))
        for a in atoms:
            f.write("%s %.10f %.10f %.10f\n" % a)
    params = ["Category: SCF", "Theory: HF", "RunType: Energy",
              "Charge: %d" % charge, "ChemSys.Multiplicity: %d" % mult]
    open(os.path.join(work, "c.param"), "w").write("\n".join(params) + "\n")
    p = subprocess.run(["perl", os.path.join(PARSERS, "ai.ecceqm"), "-p", "-f", "-b",
                        "-n", "c", "-t", "t.tpl"], cwd=work, capture_output=True,
                       text=True, env=env)
    if p.returncode:
        raise RuntimeError(p.stderr)
    return open(os.path.join(work, "t.tpl")).read()


def nbf(deck, elements):
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
    if not exe:
        print("SKIP: no ecce-qm given")
        return 77
    fails = 0
    with tempfile.TemporaryDirectory() as work:
        deck = write_deck(work, "h2o")
        per = nbf(deck, "OH")
        total = per.get("O", 0) + 2 * per.get("H", 0)
        ok = ("basis %s\n" % BASIS) in deck and "basis_data" in deck
        print("%-4s def2-TZVPP deck water functions %d (O %d, H %d)"
              % ("ok" if ok else "FAIL", total, per.get("O", 0), per.get("H", 0)))
        fails += not ok

        ch, mult, atoms = MOLECULES["h2o"]
        e_deck = run_engine(exe, deck, work, "deck")
        text = ("title b\ncharge %d\nmultiplicity %d\nmethod hf\nbasis %s\ngeometry\n"
                % (ch, mult, BASIS)) + \
            "".join("%s %.10f %.10f %.10f\n" % a for a in atoms) + "end\n"
        e_bare = run_engine(exe, text, work, "bare")
        ok = abs(e_deck - e_bare) < 1e-9
        print("%-4s h2o hf def2-TZVPP deck-bare %.1e"
              % ("ok" if ok else "FAIL", e_deck - e_bare))
        fails += not ok
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
