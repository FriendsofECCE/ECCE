#!/usr/bin/env python3
"""GromacsStructure (the .gro reader the Inputs page uses) against gmx.

The Inputs page of a GROMACS task turns the attached .gro into PDB so the
Builder can show it, without gmx, which is often not on the machine the
window is on.  Here the same .gro goes through gmx editconf, which is the
reference: the atom count, names, residues and coordinates must agree.

    groconv_test.py <path to the gromacs_groconv binary>

Exit status 77 (CTest SKIP) without gmx.
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))


def atoms(text):
    out = []
    for line in text.splitlines():
        if line.startswith(("ATOM", "HETATM")):
            out.append((line[12:16].strip(), line[17:21].strip(),
                        int(line[22:26]), float(line[30:38]),
                        float(line[38:46]), float(line[46:54])))
    return out


def main():
    conv = sys.argv[1]
    gmx = None
    for d in os.environ.get("PATH", "").split(os.pathsep):
        if os.access(os.path.join(d, "gmx"), os.X_OK):
            gmx = os.path.join(d, "gmx")
    if gmx is None:
        print("SKIP: gmx is not installed")
        return 77
    bad = []
    base = os.path.expanduser("~/.cache")
    os.makedirs(base, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix="groconv-", dir=base)
    try:
        cases = [os.path.join(HERE, "fixtures", "conf.gro"),
                 os.path.join(HERE, "..", "e2e", "fixtures", "gromacs", "conf.gro")]
        for gro in cases:
            ref = os.path.join(tmp, "ref.pdb")
            subprocess.run([gmx, "editconf", "-f", gro, "-o", ref], check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            mine = subprocess.run([conv, gro], stdout=subprocess.PIPE, check=True).stdout.decode()
            a, b = atoms(open(ref).read()), atoms(mine)
            name = os.path.relpath(gro, HERE)
            if len(a) != len(b):
                bad.append("%s: %d atoms, gmx has %d" % (name, len(b), len(a)))
                continue
            worst = max(max(abs(p[3] - q[3]), abs(p[4] - q[4]), abs(p[5] - q[5]))
                        for p, q in zip(a, b))
            names = all(p[0][:2] == q[0][:2] and p[1] == q[1] for p, q in zip(a, b))
            print("%s: %d atoms, largest coordinate difference %.4f A, names %s"
                  % (name, len(a), worst, "agree" if names else "DIFFER"))
            if worst > 0.006 or not names:
                bad.append("%s: coordinates or names differ from gmx editconf" % name)
        # a triclinic box: the CRYST1 cell must be the one gmx editconf writes
        tri = os.path.join(tmp, "tri.gro")
        lines = open(cases[0]).read().splitlines()
        lines[-1] = "   3.00000   3.20000   3.40000   0.00000   0.00000   0.50000   0.00000   0.60000   0.70000"
        open(tri, "w").write("\n".join(lines) + "\n")
        ref = os.path.join(tmp, "tri_ref.pdb")
        subprocess.run([gmx, "editconf", "-f", tri, "-o", ref], check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        mine = subprocess.run([conv, tri], stdout=subprocess.PIPE, check=True).stdout.decode()

        def cell(text):
            for line in text.splitlines():
                if line.startswith("CRYST1"):
                    return [float(x) for x in line[6:54].split()]
            return None
        c1, c2 = cell(open(ref).read()), cell(mine)
        print("triclinic CRYST1: gmx %s, ours %s" % (c1, c2))
        if not c1 or not c2 or max(abs(x - y) for x, y in zip(c1, c2)) > 0.01:
            bad.append("triclinic box: CRYST1 differs from gmx editconf")
        # a file that is not a .gro is refused with a reason
        junk = os.path.join(tmp, "junk.gro")
        open(junk, "w").write("title\nnot a number\n")
        r = subprocess.run([conv, junk], stdout=subprocess.PIPE)
        if r.returncode == 0 or b"number of atoms" not in r.stdout:
            bad.append("a file with no atom count is not refused with a reason")
        trunc = os.path.join(tmp, "trunc.gro")
        lines = open(cases[0]).read().splitlines()[:50]
        open(trunc, "w").write("\n".join(lines) + "\n")
        r = subprocess.run([conv, trunc], stdout=subprocess.PIPE)
        if r.returncode == 0 or b"ends after" not in r.stdout:
            bad.append("a truncated file is not refused with a reason")
    finally:
        import shutil
        shutil.rmtree(tmp, ignore_errors=True)
    for b in bad:
        print("FAIL", b)
    print("FAILED" if bad else "PASSED")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
