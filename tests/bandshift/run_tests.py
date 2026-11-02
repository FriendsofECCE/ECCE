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
        rc, _, err = run(FIX, ex)
        check("different atom counts need --atoms", rc != 0 and "--atoms" in err, err)
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
    finally:
        shutil.rmtree(tmp, True)
    print("FAILED: %s" % ", ".join(fails) if fails else "all passed")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
