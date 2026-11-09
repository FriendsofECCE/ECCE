#!/usr/bin/env python3
"""ecce-bandshift (#707) on stored calculations: the same molecule with
scaled frequencies, and a free molecule against one with a distant extra atom."""
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
CLI = os.path.join(REPO, "scripts", "ecce_bandshift.py")
FIX = os.path.join(REPO, "tests", "apps", "fixtures", "calc-water-vib")
fails = []


def check(name, cond, info=""):
    print(("ok   " if cond else "FAIL ") + name + ("" if cond else " " + str(info)))
    if not cond:
        fails.append(name)


def run(*args):
    p = subprocess.run([sys.executable, CLI] + list(args),
                       capture_output=True, text=True)
    return p.returncode, p.stdout, p.stderr


def sub(path, fn):
    s = open(path).read()
    open(path, "w").write(fn(s))


def scaled_copy(dst, k):
    shutil.copytree(FIX, dst)
    def f(s):
        return re.sub(r">([^<]+)</vector>",
                      lambda m: ">" + "   ".join("%.4f" % (float(x) * k)
                                                  for x in m.group(1).split()) + "</vector>", s)
    sub(os.path.join(dst, "Props", "VIBFREQ"), f)


def extra_atom_copy(dst):
    """He first, 50 A away, zero displacement in every mode; O H H follow."""
    shutil.copytree(FIX, dst)
    p = os.path.join(dst, "Props")
    sub(os.path.join(p, "GEOMTRACE"), lambda s: s.replace(
        'rows="3"', 'rows="4"').replace('rowLabels="1-O 2-H 3-H"', 'rowLabels="1-He 2-O 3-H 4-H"')
        .replace('<step number="1">', '<step number="1">50.0 50.0 50.0 '))
    sub(os.path.join(p, "VIB"), lambda s: s.replace('rows="3"', 'rows="4"').replace(
        'rowLabels="1-O 2-H 3-H"', 'rowLabels="1-He 2-O 3-H 4-H"').replace(
        'vectors="3"', 'vectors="3"').replace('">', '">', 1))
    def vib(s):
        return re.sub(r'(<matrix number="\d+">)', r"\g<1>0\n0\n0\n", s)
    sub(os.path.join(p, "VIB"), vib)


SULF = os.path.join(REPO, "data", "client", "examples", "spectroscopy",
                    "sulfate-coordination")


def example(name):
    d = os.path.join(SULF, "orca-" + name, "stored", name)
    return d


def two_copies(dst):
    """Water twice (the second turned 90 deg about z and 50 A away): modes in
    phase at f and out of phase at f+1, for the split case."""
    sys.path.insert(0, os.path.join(REPO, "scripts"))
    import ecce_bandshift as B
    els, xyz, fr, ir, md = B.read_modes(FIX)
    rot = lambda v: (-v[1], v[0], v[2])
    xyz2 = [tuple(c + o for c, o in zip(rot(p), (50.0, 0, 0))) for p in xyz]
    lines = ["6"]
    for e, p in zip(els * 2, list(xyz) + xyz2):
        lines.append("%s %.8f %.8f %.8f" % (e, *p))
    lines.append("modes %d" % (2 * len(fr)))
    for sign, df in ((1, 0.0), (-1, 1.0)):
        for f, m in zip(fr, md):
            lines.append("%.6f" % (f + df))
            for d in m:
                lines.append("%.8f %.8f %.8f" % tuple(d))
            for d in m:
                lines.append("%.8f %.8f %.8f" % tuple(sign * c for c in rot(d)))
    open(dst, "w").write("\n".join(lines) + "\n")


def main():
    tmp = tempfile.mkdtemp(prefix="ecce-bandshift-")
    try:
        sc = os.path.join(tmp, "scaled")
        scaled_copy(sc, 0.98)
        rc, out, err = run(FIX, sc, "--json")
        check("scaled copy runs", rc == 0, err)
        r = json.loads(out)
        ms = r["matches"]
        check("three bands matched", len(ms) == 3, ms)
        check("same mode matched to itself", all(m["mode_a"] == m["mode_b"] for m in ms), ms)
        check("shift = B - A = -2% of the frequency",
              all(abs(m["shift"] - (m["freq_b"] - m["freq_a"])) < 1e-9 and
                  abs(m["shift"] + 0.02 * m["freq_a"]) < 0.01 for m in ms), ms)
        check("overlap 1", all(abs(m["overlap"] - 1) < 1e-9 for m in ms), ms)
        check("IR carried", all(m["ir_a"] is not None and m["ir_a"] == m["ir_b"] for m in ms), ms)
        check("nothing unmatched", not r["unmatched_a"] and not r["unmatched_b"])
        rc, out, _ = run(FIX, sc, "--csv")
        check("csv header and rows", rc == 0 and out.splitlines()[0].startswith("mode_a,freq_a")
              and len(out.splitlines()) == 4, out)
        rc, out, _ = run(FIX, sc)
        check("text table", rc == 0 and "shift" in out and "-32." in out, out)

        ex = os.path.join(tmp, "extra")
        extra_atom_copy(ex)
        rc, out, err = run(FIX, ex, "--json")
        r = json.loads(out) if rc == 0 else {}
        check("different atom counts: automatic map", rc == 0 and
              r["occurrences"][0]["label_b"] == "O2 H3 H4" and len(r["matches"]) == 3, err)
        rc, out, err = run(FIX, ex, "--atoms", "1-3:2-4", "--json")
        check("fragment case runs", rc == 0, err)
        r = json.loads(out)
        ms = r["matches"]
        check("free molecule's three modes matched",
              len(ms) == 3 and all(m["mode_a"] == m["mode_b"] and abs(m["shift"]) < 1e-9
                                   and abs(m["overlap"] - 1) < 1e-9 for m in ms), ms)
        check("common atoms", r["n_common"] == 3)
        rc, out, err = run(ex, FIX, "--atoms", "2-4:1-3", "--json")
        check("reverse direction", rc == 0 and len(json.loads(out)["matches"]) == 3, err)
        rc, _, err = run(FIX, ex, "--atoms", "1-9")
        check("out-of-range atoms refused", rc != 0, err)

        # The sulfate in four cobalt complexes, found without any typed atoms.
        sul = example("sulfate")
        want = {"co_nh3_5_oso3": {22, 23, 24, 25, 26},
                "co_en2_cl_oso3": None, "co_en2_o2so2": None,
                "co2_en4_oh_so4_bridged": {4, 5, 6, 8, 9}}
        for name, atoms in want.items():
            rc, out, err = run(sul, example(name), "--json")
            check("sulfate in " + name, rc == 0, err)
            if rc:
                continue
            r = json.loads(out)
            oc = r["occurrences"]
            ok = len(oc) == 1 and oc[0]["label_a"].startswith("S1 ") and \
                oc[0]["rmsd"] < 0.2 and len(r["matches"]) >= 6 and \
                [l.split()[0][0] for l in [oc[0]["label_b"]]] == ["S"] and \
                sorted(a[0] for a in oc[0]["label_b"].split()) == ["O"] * 4 + ["S"]
            if atoms:
                ok = ok and set(oc[0]["atoms_b"]) == atoms
            check("one map, S O4, fit below 0.2 A, " + name, ok, oc)
            # Sulfate has no band above ~1300 cm-1, so a partner above 1500
            # is a ligand mode (C-H, N-H, CH2) that barely moves the sulfate.
            high = [m for m in r["matches"] if m["freq_b"] > 1500]
            check("no sulfate band paired with a ligand mode above 1500, " + name,
                  not high, high)
            if name == "co2_en4_oh_so4_bridged":
                check("the OH bridge oxygen (atom 3) is not used",
                      3 not in oc[0]["atoms_b"])
        rc, out, err = run(example("co2_en4_oh_so4_bridged"), sul, "--json")
        r = json.loads(out) if rc == 0 else {}
        check("complex as A: same atoms, swapped sides", rc == 0 and
              set(r["occurrences"][0]["atoms_a"]) == {4, 5, 6, 8, 9} and
              r["occurrences"][0]["label_b"].startswith("S1 ") and r["matches"], err)
        rc, _, err = run(FIX, example("co_nh3_5_oso3"))
        check("not found names the molecule", rc != 0 and "not found" in err
              and "O H2" in err, err)
        rc, _, err = run(sul, example("co_nh3_5_oso3"), "--atoms", "1-5")
        check("typed map with different elements refused", rc != 0 and
              "same element" in err, err)
        rc, out, err = run(sul, example("co_nh3_5_oso3"), "--atoms", "1-5:23,24,25,26,22", "--json")
        check("typed map still works", rc == 0 and json.loads(out)["n_common"] == 5, err)

        two = os.path.join(tmp, "two.modes")
        two_copies(two)
        rc, out, err = run(FIX, two, "--json")
        r = json.loads(out) if rc == 0 else {}
        check("two copies: two maps", rc == 0 and len(r["occurrences"]) == 2, err)
        ms = r.get("matches", [])
        check("two copies: each free band pairs with two bands (split)",
              len(ms) == 6 and all(m["split"] == 2 for m in ms) and
              all(abs(m["overlap"] - 1) < 1e-6 for m in ms), ms)
        check("two copies: in-phase unshifted, out-of-phase +1",
              sorted(round(m["shift"], 3) for m in ms) == [0.0] * 3 + [1.0] * 3, ms)
    finally:
        shutil.rmtree(tmp, True)
    print("FAILED: %s" % ", ".join(fails) if fails else "all passed")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
