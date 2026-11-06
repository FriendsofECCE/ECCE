#!/usr/bin/env python3
"""Tools > Add from SMILES... and Replace Group... through the Builder scene hook (#833).

Cysteine zwitterion from SMILES: 13 atoms, structure charge -1, the same
chirality as Open Babel's own output; added beside an open structure
without overlap, as one undo step.  Replacing the methyl of dimethyl
sulfide by [*]C(=O)O keeps the S-C direction and leaves no close contacts.

Exit status 77 (skip) without Xvfb, the built Builder or obabel.
"""
import argparse
import math
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_edit_bugs_test as eb  # noqa: E402
import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = eb.check
CYS = "[NH3+][C@@H](C[S-])C(=O)[O-]"


def obabelSdf(smiles):
    out = subprocess.run(["obabel", "-:" + smiles, "--gen3d", "-osdf"],
                         capture_output=True, text=True).stdout
    lines = out.splitlines()
    n = int(lines[3][:3])
    return [tuple(float(lines[4 + i][k:k + 10]) for k in (0, 10, 20))
            for i in range(n)]


def dmsCar(path):
    out = subprocess.run(["obabel", "-:CSC", "--gen3d", "-oxyz"],
                         capture_output=True, text=True).stdout.splitlines()
    counts = {}
    with open(path, "w") as f:
        f.write("!BIOSYM archive 3\nPBC=OFF\ndms\n!DATE Mon Oct 05 "
                "12:00:00 2026\n")
        for line in out[2:]:
            sym, x, y, z = line.split()[:4]
            n = counts[sym] = counts.get(sym, 0) + 1
            f.write("%-6s %14.9f %14.9f %14.9f XXXX 1      %-2s      %-2s"
                    "  0.000\n" % ("%s%d" % (sym, n), float(x), float(y),
                                    float(z), sym.lower(), sym))
        f.write("end\nend\n")


def dumps(text):
    out = []
    for m in re.finditer(r"BUILDER: atom (\d+) (\S+) (\S+) (\S+) (\S+)", text):
        if m.group(1) == "1":
            out.append([])
        out[-1].append((m.group(2),) +
                       tuple(float(m.group(k)) for k in (3, 4, 5)))
    return out


def dist(a, b):
    return math.sqrt(sum((a[k] - b[k]) ** 2 for k in range(3)))


def volume(p, centre, a, b, c):
    """Signed volume of the three substituents seen from the centre."""
    v = [[p[i][k] - p[centre][k] for k in range(3)] for i in (a, b, c)]
    return (v[0][0] * (v[1][1] * v[2][2] - v[1][2] * v[2][1])
            - v[0][1] * (v[1][0] * v[2][2] - v[1][2] * v[2][0])
            + v[0][2] * (v[1][0] * v[2][1] - v[1][1] * v[2][0]))


def white(name, args):
    """With --shots: a white background and a screen shot of the view."""
    return (["background 1 1 1", "wait 1500", "shot " + name]
            if args.shots else [])


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--shots")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb") or not shutil.which("obabel"):
        harness.skip("Xvfb or obabel is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "builder"), os.X_OK):
        harness.skip("builder is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "smiles", {}, local=True, keep=args.keep)
    for exe in bo.GUI + ("autosym", "cleansym"):
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        dms = os.path.join(s.state, "dms.car")
        dmsCar(dms)

        # 1. The zwitterion into an empty Builder.
        text = eb.runBuilder(s, env, wrappers, "smiles1",
                             ["wait 3000", "smiles " + CYS] +
                             white("zwitterion", args) +
                             ["coords", "key viewer undo", "coords", "quit"],
                             args.shots, structure=None)
        d = dumps(text)
        ch = re.findall(r"BUILDER: smiles: (\S+) charge=(\S+)", text)
        if not check(len(d) == 1 and len(d[0]) == 13,
                     "zwitterion has 13 atoms (%s)" % [len(x) for x in d]):
            say("\n".join("      | " + l for l in text.splitlines()[-30:]))
            return 1
        check(ch and ch[0][0] == "done" and abs(float(ch[0][1]) + 1) < 1e-9,
              "structure charge is -1 (%s)" % ch)
        ref = obabelSdf(CYS)
        pos = [a[1:] for a in d[0]]
        # N=1, CA=2, CB=3, C=5: the order Open Babel writes, kept by the Builder.
        mine, theirs = volume(pos, 1, 0, 4, 2), volume(ref, 1, 0, 4, 2)
        check(mine * theirs > 0 and abs(mine) > 0.5,
              "chirality at CA matches Open Babel (%.2f vs %.2f)" % (mine, theirs))
        forms = re.findall(r"BUILDER: formula=(\S*)", text)
        check(len(forms) == 2 and forms[0] == "H6C3NSO2-" and forms[1] == "",
              "undo removes the molecule in one step (%s)" % forms)

        # 2. Beside an open structure.
        text = eb.runBuilder(s, env, wrappers, "smiles2",
                             ["wait 3000", "coords", "smiles " + CYS, "coords",
                              "key viewer undo", "coords", "quit"],
                             args.shots, structure=dms)
        d = dumps(text)
        if check(len(d) == 3 and len(d[0]) == 9 and len(d[1]) == 22 and
                 len(d[2]) == 9, "added beside: 9, 22, undone 9 atoms (%s)"
                 % [len(x) for x in d]):
            gap = min(dist(a[1:], b[1:]) for a in d[1][:9] for b in d[1][9:])
            check(gap > 2.0, "no overlap with the existing atoms (%.2f A)" % gap)
            check(all(dist(a[1:], b[1:]) < 1e-9 for a, b in zip(d[0], d[2])),
                  "undo restores the existing atoms")
            check(all(dist(a[1:], b[1:]) < 1e-9 for a, b in zip(d[0], d[1][:9])),
                  "the existing atoms did not move")

        # 3. Replace the methyl of dimethyl sulfide by COOH.
        text = eb.runBuilder(s, env, wrappers, "smiles3",
                             ["wait 3000", "coords", "select 1",
                              "replacegroup [*]C(=O)O"] +
                             white("replace", args) +
                             ["coords", "bondorders",
                              "key viewer undo", "coords", "quit"],
                             args.shots, structure=dms)
        d = dumps(text)
        if not check(len(d) == 3 and len(d[0]) == 9 and len(d[1]) == 9 and
                     len(d[2]) == 9, "replace: 9, 9, undone 9 atoms (%s)"
                     % [len(x) for x in d]):
            say("\n".join("      | " + l for l in text.splitlines()[-30:]))
            return 1
        before, after = d[0], d[1]
        syms = sorted(a[0] for a in after)
        check(syms == sorted("C C H H H H O O S".split()),
              "formula C2H4O2S (%s)" % "".join(syms))
        bonds = set()
        for m in re.finditer(r"BUILDER: bond (\d+)-(\d+) ", text):
            bonds.add((int(m.group(1)), int(m.group(2))))
        close = [(i, j) for i in range(9) for j in range(i + 1, 9)
                 if dist(after[i][1:], after[j][1:]) < 1.0 and
                 (i, j) not in bonds and (j, i) not in bonds]
        check(not close, "no non-bonded pair closer than 1.0 A (%s)" % close)
        # Open Babel writes C S C: the methyl being replaced is atom 1.  The
        # Builder renumbers after the delete, so find S by symbol, the
        # kept methyl by its unchanged position, the new carbon as the
        # other carbon bonded to S.
        si = [k for k, a in enumerate(after) if a[0] == "S"][0]
        s0 = before[1][1:]
        old = before[0][1:]
        nb = [(j if i == si else i) for (i, j) in bonds if si in (i, j)]
        new = [after[k][1:] for k in nb if after[k][0] == "C" and
               dist(after[k][1:], before[2][1:]) > 1e-6]
        if check(len(new) >= 1, "S has a new carbon neighbour"):
            v1 = [old[k] - s0[k] for k in range(3)]
            v2 = [new[0][k] - s0[k] for k in range(3)]
            cos = sum(a * b for a, b in zip(v1, v2)) / (
                math.sqrt(sum(a * a for a in v1)) * math.sqrt(sum(a * a for a in v2)))
            check(cos > math.cos(math.radians(1.0)),
                  "the S-C direction is kept (cos %.5f)" % cos)
            ln = dist(s0, new[0])
            check(1.5 < ln < 2.1, "S-C length from the covalent radii (%.3f A)" % ln)
        check(all(dist(a[1:], b[1:]) < 1e-9 for a, b in zip(before, d[2])),
              "undo restores the structure")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(eb.failures) if eb.failures else "PASSED")
    return 1 if eb.failures else 0


if __name__ == "__main__":
    sys.exit(main())
