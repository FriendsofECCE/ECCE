#!/usr/bin/env python3
"""ai.vina's automatic ligand choice (#962) and verifyinput on a Vina deck (#963).

A fake complex: two identical 2-residue protein chains, one 5-atom ligand
(LIG), four crystal waters and a sodium ion, with the waters first so that the
old "smallest molecule" rule picks atom 1, a water oxygen.
Usage: vina_autoligand_test.py <source dir>
"""
import os, re, shutil, subprocess, sys, tempfile

SRC = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else ".")
PARSERS = os.path.join(SRC, "scripts", "parsers")

atoms = []  # (symbol, x, y, z, atomname, resname, resnum, chain)
for k in range(4):
    atoms.append(("O", 40.0 + 7 * k, 40.0, 0.0, "O", "HOH", 500 + k, "W"))
for chain, y in (("A", 0.0), ("B", 20.0)):
    for r, (rn, base) in enumerate((("ALA", 0.0), ("GLY", 4.5))):
        for j in range(3):
            atoms.append(("C", base + 1.5 * j, y, 0.0, "C%d" % j, rn, r + 1, chain))
lig_first = len(atoms) + 1
for j in range(5):
    atoms.append(("C", 1.5 * j, 0.0, 60.0, "C%d" % j, "LIG", 1, "L"))
lig_last = len(atoms)
atoms.append(("Na", 0.0, 80.0, 0.0, "NA", "NA", 700, "I"))
n = len(atoms)

def frag(with_pdb):
    out = ["title: fake", "type: molecule", "num_atoms: %d" % n,
           "atom_info: symbol cart", "atom_list: "]
    out += ["%s %.3f %.3f %.3f" % a[:4] for a in atoms]
    if with_pdb:
        out.append("pdb_list: ")
        for a in atoms:
            out.append("M %-4s %-4s%6d   %-4s %s" % (a[4], a[5], a[6], "", a[7]))
    return "\n".join(out) + "\n"

PARAM = "title: fake\nES.Runtype.Vina.BoxAuto: 1\n"
fail = []

def check(ok, what):
    print(("ok   " if ok else "FAIL ") + what)
    if not ok:
        fail.append(what)

def run_ai(d, with_pdb):
    open(os.path.join(d, "t.frag"), "w").write(frag(with_pdb))
    open(os.path.join(d, "t.param"), "w").write(PARAM)
    env = dict(os.environ, ECCE_HOME=SRC)
    # ai.vina writes the deck over its -t file, as DeckWriter expects of its
    # vina.vinain.orig copy; given the source template it overwrote that, and
    # the GUI tests, whose ECCE_HOME links this tree's scripts, then ran on it.
    shutil.copy(os.path.join(PARSERS, "vina.tpl"), os.path.join(d, "vina.vinain.orig"))
    p = subprocess.run(["perl", os.path.join(PARSERS, "ai.vina"), "-p", "-f", "-n", "t",
                        "-t", "vina.vinain.orig"],
                       cwd=d, env=env, capture_output=True, text=True)
    return p, open(os.path.join(d, "vina.vinain.orig")).read()

def verify(d, deck_text, atoms_n, code="Vina"):
    f = os.path.join(d, "deck.in")
    open(f, "w").write(deck_text)
    p = subprocess.run(["perl", os.path.join(PARSERS, "verifyinput"), "-f", f,
                        "-c", code, "-a", str(atoms_n)], capture_output=True, text=True)
    return p.stdout

with tempfile.TemporaryDirectory(prefix="vina_autoligand_") as d:
    p, deck = run_ai(d, True)
    check(p.returncode == 0, "ai.vina runs (rc=%d %s)" % (p.returncode, p.stderr[-200:]))
    m = re.search(r"#\| LIGAND_ATOMS=(\S+)", deck)
    check(m and m.group(1) == "%d-%d" % (lig_first, lig_last),
          "ligand is the LIG molecule %d-%d, not a water (got %s)" % (lig_first, lig_last, m and m.group(1)))
    check("ai.vina note [LIGAND_ATOMS]: the ligand was chosen automatically: residue LIG, 5 atoms" in p.stderr,
          "the choice is reported with residue name and atom count")
    check("receptor still holds 4 water molecules" in p.stderr, "waters in the receptor are warned about")
    check("identical chains (A, B)" in p.stderr, "identical chains are warned about")
    out = verify(d, deck, n)
    check("BAD" not in out and "|structure|All %d atoms" % n in out,
          "verifyinput: Vina deck is clean and both lists cover all %d atoms" % n)

    # No residue information: waters and the ion are still not the ligand.
    p, deck = run_ai(d, False)
    m = re.search(r"#\| LIGAND_ATOMS=(\S+)", deck)
    check(m and m.group(1) != "1", "without residue names the ligand is not a lone water (got %s)" % (m and m.group(1)))

    # verifyinput: overlaps, gaps and out-of-range numbers.
    bad = re.sub(r"LIGAND_ATOMS=\S+", "LIGAND_ATOMS=1-3", deck)
    bad = re.sub(r"RECEPTOR_ATOMS=\S+", "RECEPTOR_ATOMS=3-%d" % (n - 1), bad)
    out = verify(d, bad, n)
    check("both the ligand and the receptor lists: 3" in out, "verifyinput reports the overlap")
    check("neither list: %d" % n in out, "verifyinput reports the gap")
    out = verify(d, re.sub(r"RECEPTOR_ATOMS=\S+", "RECEPTOR_ATOMS=1-%d" % (n + 2), deck), n)
    check("beyond the calculation's %d atoms" % n in out, "verifyinput reports numbers out of range")

    # Other codes keep the generic atom-count check.
    out = verify(d, "xtb --chrg 0\n2\nt\nH 0 0 0\nH 0 0 0.74\n", 3, "MOPAC")
    check("The geometry has 2 atoms; the calculation says it has 3" in out,
          "the atom-count check of other codes is unchanged")

sys.exit(1 if fail else 0)
