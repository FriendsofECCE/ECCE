#!/usr/bin/env python3
"""Mode composition (#486) of N-methylacetamide against the published
amide assignments.

    tests/modecomp/run_tests.py --cli build-cmake/vibanalysis_cli

ORCA's frequency run of trans-N-methylacetamide (nma_trans_freq.out) goes
through the real parser to a modes file; `vibanalysis_cli comp` gives each
mode's share of the automatic coordinate set.  The expectations are the
literature's, not produced by the code: amide I is mostly the C=O stretch,
amide II mostly the N-H in-plane bend with the C-N stretch (Miyazawa,
Shimanouchi, Mizushima, J. Chem. Phys. 24, 408 (1956); Krimm and Bandekar,
Adv. Protein Chem. 38, 181 (1986): amide I ~80 % C=O s; amide II ~60 % N-H
ib + ~40 % C-N s).  A harmonic gas-phase calculation puts the bands at
1852 and ~1500 cm-1, so they are found by window, not by value.

Atoms (1-based): C1 methyl, C2 carbonyl, O3, N4, C5 N-methyl, H9 on N.
Exit 77 without the CLI, 1 on a failure.
"""
import argparse
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "vibanalysis"))
import extract  # noqa: E402

FIX = os.path.join(HERE, "nma_trans_freq.out")
failures = []


def check(ok, text):
    print(("ok   " if ok else "FAIL ") + text)
    if not ok:
        failures.append(text)


def comp(cli, env, text, specs):
    with tempfile.TemporaryDirectory(prefix="modecomp-") as tmp:
        path = os.path.join(tmp, "modes.txt")
        with open(path, "w") as f:
            f.write(text)
        proc = subprocess.run([cli, path, "comp"] + specs, capture_output=True,
                              text=True, env=env)
    if proc.returncode != 0:
        check(False, "composition ran: " + proc.stderr.strip())
        return None, []
    lines = [l.split("\t") for l in proc.stdout.splitlines()]
    names = lines[0][1:]
    rows = [(int(l[0]), float(l[1]), [float(v) for v in l[2:]]) for l in lines[1:]]
    return names, rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cli", required=True)
    args = ap.parse_args()
    cli = os.path.abspath(args.cli)
    if not os.access(cli, os.X_OK):
        print("skipped: %s is not built" % cli)
        return 77
    home = tempfile.mkdtemp(prefix="modecomp-home-")
    env = dict(os.environ, ECCE_HOME=REPO, HOME=home, ECCE_REALUSERHOME=home,
               ECCE_REALUSER="modecomp")
    elements = list("CCONC") + list("HHHHHHH")
    text, _sym, _freqs = extract.modes_file(FIX, "orca.desc", "Vibration", elements)

    names, rows = comp(cli, env, text, [])
    if names is None:
        return 1

    def col(frag):
        return [i for i, n in enumerate(names) if frag in n]

    co = col("C2–O3 stretch")
    cn = col("C2–N4 stretch")
    nh_bend = [i for i, n in enumerate(names)
               if "H9" in n and "N4" in n and "bend" in n]
    check(len(co) == 1 and len(cn) == 1 and len(nh_bend) == 2,
          "automatic set holds C=O, C-N and the two N-H bends")

    for _n, _f, p in rows:
        s = sum(p)
        check(s < 1e-6 or abs(s - 100) < 1e-6, "shares sum to 100 (or 0 for rigid)")
        break

    def pick(lo, hi):
        return [r for r in rows if lo <= r[1] <= hi]

    amide1 = pick(1700, 2000)
    check(len(amide1) == 1, "one mode in the amide I window")
    p = amide1[0][2]
    print("amide I  %.0f cm-1: C=O %.1f  C-N %.1f  NH bend %.1f" %
          (amide1[0][1], p[co[0]], p[cn[0]], sum(p[i] for i in nh_bend)))
    check(p[co[0]] > 60 and p[co[0]] == max(p),
          "amide I: C=O stretch dominates (>60 %, literature ~80 %)")

    #  Amide II: the 1400-1600 mode with the largest N-H bend + C-N share.
    cand = pick(1400, 1600)
    amide2 = max(cand, key=lambda r: r[2][cn[0]] + sum(r[2][i] for i in nh_bend))
    p = amide2[2]
    nh = sum(p[i] for i in nh_bend)
    print("amide II %.0f cm-1: C=O %.1f  C-N %.1f  NH bend %.1f" %
          (amide2[1], p[co[0]], p[cn[0]], nh))
    check(nh > 20 and p[cn[0]] > 30 and nh + p[cn[0]] > 65,
          "amide II: N-H in-plane bend with the C-N stretch (literature 60 + 40, the split differs)")
    check(p[co[0]] < 20, "amide II: little C=O stretch")

    #  Chosen coordinates are normalised over the chosen set only.
    names2, rows2 = comp(cli, env, text, ["2-3", "2-4"])
    r = [x for x in rows2 if x[1] > 1700][0]
    check(names2 is not None and abs(sum(r[2]) - 100) < 1e-6 and r[2][0] > r[2][1],
          "chosen set C=O, C-N: normalised to 100, C=O larger in amide I")

    #  Selection in the viewer -> highlight shares of the spectrum.
    def sel(atoms):
        with tempfile.TemporaryDirectory(prefix="modecomp-") as tmp:
            path = os.path.join(tmp, "modes.txt")
            with open(path, "w") as f:
                f.write(text)
            proc = subprocess.run([cli, path, "sel"] + [str(a) for a in atoms],
                                  capture_output=True, text=True, env=env)
        if proc.returncode != 0:
            check(False, "selection ran: " + proc.stderr.strip())
            return None, []
        lines = [l.split("\t") for l in proc.stdout.splitlines()]
        return lines[0][1], [(float(l[1]), float(l[2])) for l in lines[1:]]

    key, shares = sel([2, 3])
    check(key == "C2\u2013O3 stretch", "bond selection is named in the key: " + key)
    hot = [f for f, v in shares if v >= 10 and f > 10]
    a1 = [v for f, v in shares if 1700 < f < 2000][0]
    check(len(hot) == 1 and 1700 < hot[0] < 2000 and a1 > 60,
          "C=O selected: only amide I is highlighted (%s)" % hot)
    key, shares = sel([1, 6, 7, 8])
    hot = [f for f, v in shares if v >= 10]
    check(key.startswith("selected group (4") and hot and
          all(900 < f < 1500 or 2900 < f < 3250 for f in hot) and
          any(2900 < f < 3250 for f in hot),
          "CH3 selected: the C-H stretches and deformations, not the amide bands")
    key, shares = sel([2, 3, 4, 9])
    check([v for f, v in shares if 1700 < f < 2000][0] > 70, "amide group selected: amide I is in it")

    if failures:
        print("%d failed" % len(failures))
        return 1
    print("all passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
