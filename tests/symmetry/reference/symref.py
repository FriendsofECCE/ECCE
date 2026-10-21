"""Shared helpers for the symmetry-helper reference test (#422).

A case file is plain text:

    program autosym | cleansym | symops | genmol | getfrag | genmollat
    threshold 0.01          (all but symops)
    group C2V               (all but autosym; symops takes it as argv[1])
    lattice a b c al be ga  (genmollat only)
    atoms
    O 0.0 0.0 0.1173
    ...

stdin is built exactly as src/tdat/chemistry/SymmetryOps.C and Fragment.C
write it (see docs/claude/codereg/symmetry-helpers-interface.md).
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CORPUS = os.path.join(HERE, "corpus")
EXPECTED = os.path.join(HERE, "expected")
PROGRAMS = ["autosym", "cleansym", "symops", "genmol", "getfrag", "genmollat"]

ELEM = {"H": 1, "He": 2, "Li": 3, "Be": 4, "B": 5, "C": 6, "N": 7, "O": 8,
        "F": 9, "Ne": 10, "Na": 11, "Mg": 12, "Al": 13, "Si": 14, "P": 15,
        "S": 16, "Cl": 17, "Ar": 18, "Ca": 20, "Ti": 22, "Fe": 26, "Zn": 30,
        "Br": 35, "Kr": 36, "Xe": 54}


def parse_case(path):
    c = {"program": None, "threshold": 0.01, "group": "", "lattice": None,
         "atoms": []}
    in_atoms = False
    for line in open(path):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if in_atoms:
            s, x, y, z = line.split()
            c["atoms"].append((s, float(x), float(y), float(z)))
            continue
        k, _, rest = line.partition(" ")
        if k == "program":
            c["program"] = rest.strip()
        elif k == "threshold":
            c["threshold"] = float(rest)
        elif k == "group":
            c["group"] = rest.strip()
        elif k == "lattice":
            c["lattice"] = [float(v) for v in rest.split()]
        elif k == "atoms":
            in_atoms = True
    return c


def _g(v):
    # C's %G, which is what ECCE prints the numbers with
    return "%G" % v


def stdin_for(c):
    p, a = c["program"], c["atoms"]
    if p == "symops":
        return ""
    out = ["%d" % len(a), _g(c["threshold"])]
    if p != "autosym":
        out.append("%-8s" % c["group"])
    if p == "genmollat":
        out.append(" ".join(_g(v) for v in c["lattice"]))
    for s, x, y, z in a:
        if p == "cleansym":
            out.append("%s %s %s" % (_g(x), _g(y), _g(z)))
        else:
            out.append("%-16s" % s)
            out.append("%d %s %s %s" % (ELEM[s], _g(x), _g(y), _g(z)))
    return "\n".join(out) + "\n"


def args_for(c):
    return [c["group"]] if c["program"] == "symops" else []


def resolve(impl, prog):
    """impl is a directory holding the programs, or a path prefix."""
    cands = []
    if os.path.isdir(impl):
        cands = [os.path.join(impl, prog), os.path.join(impl, prog + ".exe")]
    else:
        cands = [impl + prog, impl + prog + ".exe"]
    for x in cands:
        if os.path.isfile(x) and os.access(x, os.X_OK):
            return x
    raise SystemExit("no executable %s under --impl %s" % (prog, impl))


def run_case(impl, c):
    exe = resolve(impl, c["program"])
    r = subprocess.run([exe] + args_for(c), input=stdin_for(c),
                       capture_output=True, text=True, timeout=120)
    return r.returncode, r.stdout


def all_cases():
    """[(program, name, path)] sorted, from corpus/<program>/*.case"""
    out = []
    for prog in PROGRAMS:
        d = os.path.join(CORPUS, prog)
        if os.path.isdir(d):
            for f in sorted(os.listdir(d)):
                if f.endswith(".case"):
                    out.append((prog, f[:-5], os.path.join(d, f)))
    return out


def default_impl():
    for cand in (os.environ.get("ECCE_BUILD_DIR"),
                 os.path.join(HERE, "..", "..", "..", "build-cmake")):
        if cand and os.path.isfile(os.path.join(cand, "autosym")):
            return os.path.abspath(cand)
    return os.path.abspath(os.path.join(HERE, "..", "..", "..", "build-cmake"))


def write_case(path, prog, atoms, threshold=None, group=None, lattice=None,
               note=""):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        if note:
            f.write("# %s\n" % note)
        f.write("program %s\n" % prog)
        if threshold is not None:
            f.write("threshold %s\n" % threshold)
        if group is not None:
            f.write("group %s\n" % group)
        if lattice:
            f.write("lattice %s\n" % " ".join(repr(v) for v in lattice))
        if atoms:
            f.write("atoms\n")
            for s, x, y, z in atoms:
                f.write("%s %.6f %.6f %.6f\n" % (s, x, y, z))


# ---- parsing of program output ------------------------------------------

def parse_output(prog, text):
    L = text.splitlines()
    if prog == "autosym":
        return {"group": L[0].strip(),
                "xyz": [[float(v) for v in l.split()] for l in L[1:] if l.strip()]}
    if prog == "cleansym":
        n = int(L[0])
        return {"xyz": [[float(v) for v in l.split()] for l in L[1:1 + n]]}
    if prog == "symops":
        n = int(L[0])
        rows = [[float(v) for v in l.split()] for l in L[1:1 + 3 * n]]
        return {"ops": [rows[3 * i:3 * i + 3] for i in range(n)]}
    if prog == "getfrag":
        n = int(L[0])
        return {"idx": [int(l) for l in L[1:1 + n]]}
    # genmol, genmollat: count, blank line, then a16 tag + 3 numbers
    n = int(L[0])
    atoms = []
    for l in L[2:2 + n]:
        atoms.append((l[:16].strip(), [float(v) for v in l[16:].split()]))
    return {"atoms": atoms}
