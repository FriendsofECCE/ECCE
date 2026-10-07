#!/usr/bin/env python3
"""
Reference MO compositions from Multiwfn (Mulliken partition), as TSV.

    multiwfn_ref.py MOLDEN MO[,MO...] [--beta MO[,MO...]] [--multiwfn PATH]
                    [--scpa]

MOLDEN is what `orca_2mkl NAME -molden` writes (or any molden file).
Multiwfn's main function 8 > 1 is driven with the printing threshold at 0
and its text output parsed into the same columns `ecce-mocomp --tsv` prints:

    mo  kind  label  atom  l  shell  component  percent

kind is ao (one basis function), shell (all functions of one contracted
shell), type (all shells of one l on an atom) or atom.  Multiwfn numbers
shells over the molecule; the molden [GTO] block, which it reads too, gives
each one's atom, l and its place among that atom's shells of that l.

--scpa uses the Ros-Schuit partition (function 8 > 3), which is the plain
c^2 population normalised to 100%: the reference for `ecce-mocomp --method c2`.

Multiwfn is not part of this repository.  Used to (re)write the checked-in
reference files; run_tests.py only needs those, and re-derives them from
the checked-in molden files when Multiwfn is found.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

LETTER = "spdfghik"
SPH = {"D0": "z2", "D+1": "xz", "D-1": "yz", "D+2": "x2-y2", "D-2": "xy"}
M_ORDER = ["0", "+1", "-1", "+2", "-2", "+3", "-3"]


def find_multiwfn(explicit=None):
    for c in (explicit, os.environ.get("MULTIWFN"),
              shutil.which("Multiwfn_noGUI"), shutil.which("Multiwfn")):
        if c and os.path.isfile(c) and os.access(c, os.X_OK):
            return c
    return None


def fchk_shells(path):
    """The same from a Gaussian formatted checkpoint file.  An SP shell
    counts as an s shell then a p shell, as Multiwfn numbers them."""
    text = open(path).read().split("\n")
    def block(title):
        for i, line in enumerate(text):
            if line.startswith(title):
                n = int(line.split()[-1])
                vals = []
                j = i + 1
                while len(vals) < n:
                    vals += text[j].split()
                    j += 1
                return [int(v) for v in vals]
    types = block("Shell types")
    atoms = block("Shell to atom map")
    shells = []
    seen = {}
    last = 0
    for t, a in zip(types, atoms):
        if a != last:
            seen = {}
            last = a
        for l in ([0, 1] if t == -1 else [abs(t)]):
            seen[l] = seen.get(l, 0) + 1
            shells.append((a, l, seen[l]))
    return shells


def molden_shells(path):
    """[(atom, l, seq)] per shell, in Multiwfn's numbering (1-based)."""
    if path.endswith(".fchk"):
        return fchk_shells(path)
    shells = []
    text = open(path).read().splitlines()
    i = 0
    while i < len(text) and not text[i].lower().startswith("[gto]"):
        i += 1
    i += 1
    atom = 0
    seen = {}
    while i < len(text):
        line = text[i].strip()
        if line.startswith("["):
            break
        parts = line.split()
        if len(parts) == 2 and parts[0].isdigit():
            atom = int(parts[0])
            seen = {}
        elif len(parts) >= 3 and parts[0].lower() in LETTER:
            l = LETTER.index(parts[0].lower())
            seen[l] = seen.get(l, 0) + 1
            shells.append((atom, l, seen[l]))
            i += int(parts[1])
        i += 1
    return shells


def component(kind_label, l):
    if l == 0:
        return "s"
    if kind_label in SPH:
        return SPH[kind_label]
    m = re.match(r"^[A-Z]([+-]?\d+)$", kind_label)
    if m and l >= 3:
        mm = m.group(1)
        mm = mm if mm in ("0",) else (mm if mm[0] in "+-" else "+" + mm)
        return "%s#%d" % (LETTER[l], M_ORDER.index(mm) + 1)
    return kind_label.lower()        # X, Y, Z, XX, XY ... (Cartesian)


def run(multiwfn, molden, indices, scpa=False):
    work = tempfile.mkdtemp()
    shutil.copy(multiwfn, os.path.join(work, "mw"))
    ini = os.path.join(os.path.dirname(multiwfn), "settings.ini")
    text = open(ini).read()
    text = re.sub(r"compthres=\s*[\d.]+", "compthres= 0.0", text, count=1)
    open(os.path.join(work, "settings.ini"), "w").write(text)
    env = dict(os.environ, Multiwfnpath=work, OMP_STACKSIZE="200M")
    keys = ("8\n3\n" if scpa else "8\n1\n") + "".join("%d\n" % i for i in indices) + "0\nq\n"
    proc = subprocess.run(["bash", "-c", "ulimit -s unlimited; exec %s/mw %s"
                           % (work, molden)],
                          input=keys, capture_output=True, text=True, env=env,
                          timeout=900)
    shutil.rmtree(work, ignore_errors=True)
    return proc.stdout


def parse(out, shells, label_of):
    """label_of(idx) -> (mo number to print, spin tag) for Multiwfn's index."""
    rows = []
    blocks = re.split(r"\n Orbital:\s+(\d+)\s+Energy", out)
    for b in range(1, len(blocks), 2):
        idx = int(blocks[b])
        body = blocks[b+1]
        mo = label_of(idx)
        atom_of = {}
        for m in re.finditer(r"^\s+(\d+)\s+(.+?)\s+(\d+)\(\s*\S+\s*\)\s+(\d+)\s+"
                             r"(?:-?[\d.]+ %\s+){0,2}(-?[\d.]+) %",
                             body, re.M):
            fn, typ, atom, shell = (m.group(1), m.group(2).replace(" ", ""),
                                    int(m.group(3)), int(m.group(4)))
            a, l, seq = shells[shell-1]
            assert a == atom, "shell/atom mismatch"
            rows.append((mo, "ao", "", atom, l, seq, component(typ, l),
                         float(m.group(5))))
        tail = body.split("Composition of each shell", 1)[1]
        shell_part, rest = tail.split("Composition of different types", 1)
        for m in re.finditer(r"Shell\s+(\d+) Type:\s+(\S+)\s+in atom\s+(\d+)\(\s*\S+\s*\) :\s+(-?[\d.]+) %",
                             shell_part):
            a, l, seq = shells[int(m.group(1))-1]
            assert LETTER[l].upper() == m.group(2) and a == int(m.group(3)), \
                "shell numbering differs from Multiwfn's"
            rows.append((mo, "shell", "", a, l, seq, "", float(m.group(4))))
        for m in re.finditer(r"Atom\s+(\d+)\(\s*\S+\s*\) :\s+(-?[\d.]+) %", rest):
            rows.append((mo, "atom", "", int(m.group(1)), -1, 0, "",
                         float(m.group(2))))
        types = {}
        for r in rows:
            if r[0] == mo and r[1] == "shell":
                types[(r[3], r[4])] = types.get((r[3], r[4]), 0.0) + r[7]
        for (a, l), v in sorted(types.items()):
            rows.append((mo, "type", "", a, l, 0, "", v))
    return rows


def main(argv):
    args = argv[1:]
    mw = None
    beta = []
    if "--multiwfn" in args:
        i = args.index("--multiwfn"); mw = args[i+1]; del args[i:i+2]
    scpa = "--scpa" in args
    if scpa:
        args.remove("--scpa")
    if "--beta" in args:
        i = args.index("--beta"); beta = [int(x) for x in args[i+1].split(",")]
        del args[i:i+2]
    molden = args[0]
    alpha = [int(x) for x in args[1].split(",")] if len(args) > 1 and args[1] else []
    multiwfn = find_multiwfn(mw)
    if not multiwfn:
        sys.exit("no Multiwfn found (set MULTIWFN or --multiwfn)")
    shells = molden_shells(molden)
    if molden.endswith(".fchk"):
        nmo = int([l for l in open(molden) if l.startswith("Number of independent functions")][0].split()[-1])
    else:
        nmo = sum(1 for l in open(molden) if l.strip().lower().startswith("ene="))
    nbeta_offset = nmo // 2 if beta else 0     # unrestricted: alpha then beta
    indices = alpha + [nbeta_offset + b for b in beta]
    out = run(multiwfn, molden, indices, scpa)

    def label_of(idx):
        return str(idx) if idx not in [nbeta_offset + b for b in beta] or not beta \
            else "b%d" % (idx - nbeta_offset)
    for r in parse(out, shells, label_of):
        print("\t".join(str(x) if not isinstance(x, float) else "%.5f" % x for x in r))


if __name__ == "__main__":
    main(sys.argv)
