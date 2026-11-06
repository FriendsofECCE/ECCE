#!/usr/bin/env python3
"""ecce_displace on stored ORCA and Gaussian 16 water frequency output (#618).

The modes come through the real parsers (tests/vibanalysis/extract.py).  Water
has no imaginary mode, so the sign of the stored frequency of the lowest real
mode is flipped: the vectors are what the code printed.  Expected values are
computed here from the vectors, not by the tool: displaced - source must be
amplitude * mode / (largest atom length of the mode), to 1e-5 Angstrom.
"""
import json
import math
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "vibanalysis"))
import extract  # noqa: E402

CLI = os.path.join(REPO, "scripts", "ecce_displace.py")
FIX = os.path.join(REPO, "tests", "parsers", "fixtures")
CASES = [("ORCA", os.path.join(FIX, "orca/h2o_freq_raman.out"), "orca.desc", "Vibration"),
         ("G16", os.path.join(FIX, "gaussian-16/h2o_optfreq.log"), "gaussian-16.desc", "GeoVib")]
fails = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else " " + str(detail)))
    if not ok:
        fails.append(name)


def run(path, *args):
    p = subprocess.run([sys.executable, CLI, path] + list(args),
                       capture_output=True, text=True)
    return p.returncode, (json.loads(p.stdout) if p.returncode == 0 else p.stderr)


def parse(text):
    tok = text.split()
    n = int(tok[0])
    xyz = [[float(t) for t in tok[1 + 4 * a + 1:1 + 4 * a + 4]] for a in range(n)]
    pos = 1 + 4 * n
    assert tok[pos] == "modes"
    m = int(tok[pos + 1]); pos += 2
    freqs, vecs = [], []
    for _ in range(m):
        freqs.append(float(tok[pos])); pos += 1
        v = [float(t) for t in tok[pos:pos + 3 * n]]; pos += 3 * n
        vecs.append([v[3 * a:3 * a + 3] for a in range(n)])
    return xyz, freqs, vecs


def longest(v):
    return max(math.sqrt(sum(c * c for c in a)) for a in v)


def expect(vec, amp):
    k = amp / longest(vec)
    return [[c * k for c in a] for a in vec]


def main():
    tmp = tempfile.mkdtemp(prefix="ecce-displace-")
    for code, fix, desc, rt in CASES:
        text, _sym, _f = extract.modes_file(fix, desc, rt, None, None, None)
        xyz, freqs, vecs = parse(text)
        real = sorted((i for i, f in enumerate(freqs) if f > 100), key=lambda i: freqs[i])
        a, b = real[0], real[1]
        freqs[a], freqs[b] = -freqs[a], -freqs[b]
        lines = text.splitlines()
        out, fi = [], 0
        for ln in lines:
            if len(ln.split()) == 1 and "." in ln and fi < len(freqs):
                ln = "%.4f" % freqs[fi]; fi += 1
            out.append(ln)
        path = os.path.join(tmp, code + ".modes")
        open(path, "w").write("\n".join(out) + "\n")

        for amp in (0.1, 0.25):
            rc, res = run(path, "--mode", str(a + 1), "--amplitude", str(amp), "--sign", "both")
            check("%s mode %d amp %g runs" % (code, a + 1, amp), rc == 0, res)
            if rc:
                continue
            want = expect(vecs[a], amp)
            for r, k in zip(res["results"], (1, -1)):
                diff = [[c - x for c, x in zip(ca, xa)] for ca, xa in zip(r["coords"], xyz)]
                err = max(abs(d - k * w) for da, wa in zip(diff, want) for d, w in zip(da, wa))
                check("%s amp %g sign %s: displaced - source = amplitude * mode" %
                      (code, amp, r["sign"]), err < 1e-5, err)
                check("%s amp %g sign %s: largest displacement = amplitude" %
                      (code, amp, r["sign"]), abs(longest(diff) - amp) < 1e-5, longest(diff))
        rc, res = run(path, "--mode", str(a + 1), "--source", "h2o-freq")
        note = res["results"][0]["note"] if rc == 0 else res
        check("%s annotation" % code,
              note == "displaced +0.10 Å along mode %d (%di cm-1) of h2o-freq"
              % (a + 1, round(-freqs[a])), note)

        rc, res = run(path, "--imaginary", "--sign", "-")
        want = [[x + y for x, y in zip(p, q)] for p, q in
                zip(expect(vecs[a], 0.1), expect(vecs[b], 0.1))]
        want = [[-c * 0.1 / longest(want) for c in v] for v in want]
        diff = [[c - x for c, x in zip(ca, xa)] for ca, xa in zip(res["results"][0]["coords"], xyz)]
        err = max(abs(d - w) for da, wa in zip(diff, want) for d, w in zip(da, wa))
        check("%s all imaginary: summed, rescaled, minus" % code, err < 1e-5, err)
        check("%s all imaginary annotation" % code,
              res["results"][0]["note"].startswith("displaced -0.10 Å along all 2 imaginary modes"),
              res["results"][0]["note"])

        rc, _ = run(path, "--mode", "99")
        check("%s bad mode refused" % code, rc != 0)
    print("FAILED: %s" % ", ".join(fails) if fails else "all passed")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
