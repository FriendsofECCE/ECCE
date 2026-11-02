#!/usr/bin/env python3
"""ecce-spectrum (#707): difference curve, experimental import and CSV."""
import math
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "scripts"))
import ecce_spectrum as S  # noqa: E402

FIX = os.path.join(REPO, "tests", "apps", "fixtures", "calc-water-vib")
fails = []


def check(name, ok, info=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else " " + str(info)))
    if not ok:
        fails.append(name)


def scaled_copy(dst, k):
    shutil.copytree(FIX, dst)
    p = os.path.join(dst, "Props", "VIBFREQ")
    s = open(p).read()
    a, b = s.index(">", s.index("<vector")) + 1, s.index("</vector>")
    new = "   ".join("%.4f" % (float(x) * k) for x in s[a:b].split())
    open(p, "w").write(s[:a] + new + s[b:])


def write_exp(path, transmittance, percent):
    centres = [(1612.0, 0.8, 25.0), (3790.0, 0.2, 30.0)]
    with open(path, "w") as f:
        f.write("# measured\nwavenumber,value\n")
        for x in range(400, 4001, 4):
            a = sum(h * math.exp(-((x - c) / w) ** 2) for c, h, w in centres) + 0.02
            v = 10 ** (-a) if transmittance else a
            f.write("%d,%.6f\n" % (x, v * (100 if transmittance and percent else 1)))


def main():
    tmp = tempfile.mkdtemp(prefix="ecce-spectrum-")
    try:
        sc = os.path.join(tmp, "scaled")
        scaled_copy(sc, 0.98)
        r = S.spectrum(FIX, FIX, None, 0, 4000, 1.0)
        check("a calculation against itself: difference zero",
              max(abs(d) for d in r["difference"]) < 1e-12)
        r = S.spectrum(FIX, sc, None, 0, 4000, 1.0)
        xs, d = r["x"], r["difference"]

        def argext(lo, hi, fn):
            idx = [i for i in range(len(xs)) if lo <= xs[i] <= hi]
            return xs[fn(idx, key=lambda i: d[i])]
        check("difference peaks at B's band (1612)", abs(argext(1590, 1630, max) - 1612.1) < 5,
              argext(1590, 1630, max))
        check("difference dips at A's band (1645)", abs(argext(1625, 1670, min) - 1645.0) < 5,
              argext(1625, 1670, min))
        check("difference ~0 between the bands", abs(d[2500]) < 1e-9, d[2500])
        check("difference = B - A", all(abs(d[i] - (r["b"][i] - r["a"][i])) < 1e-12
                                         for i in range(0, 4001, 97)))

        ea, et, ep = (os.path.join(tmp, n) for n in ("a.csv", "t.csv", "p.csv"))
        write_exp(ea, False, False)
        write_exp(et, True, False)
        write_exp(ep, True, True)
        ra, rt, rp = (S.read_experimental(p) for p in (ea, et, ep))
        check("kinds detected", (ra[2], rt[2], rp[2]) ==
              ("absorbance", "transmittance", "transmittance"), (ra[2], rt[2], rp[2]))
        for name, other in (("T (0-1)", rt), ("T (percent)", rp)):
            err = max(abs(p - q) for p, q in zip(ra[1], other[1]))
            check(name + " gives the absorbance curve", err < 1e-4, err)
        res = S.spectrum(FIX, sc, rt, 0, 4000, 1.0)
        top = max(v for v in res["experimental"] if v is not None)
        check("experimental scaled to the computed maximum", abs(top - max(res["a"])) < 1e-6 * max(res["a"]) + 1e-9,
              (top, max(res["a"])))
        a2 = S.spectrum(FIX, sc, ra, 0, 4000, 1.0, scale=0.9)
        t2 = S.spectrum(FIX, sc, rt, 0, 4000, 1.0, scale=0.9)
        check("T and A files plot the same curve", max(abs(p - q) for p, q in
              zip(a2["experimental"][400:3900], t2["experimental"][400:3900])) < 1e-3 * max(a2["a"]))
        res = S.spectrum(FIX, None, ra, 0, 4000, 1.0, scale=1.0)
        res2 = S.spectrum(FIX, None, ra, 0, 4000, 1.0, scale=0.9)
        check("the frequency factor moves the computed curve only",
              max(abs(p - q) for p, q in zip(res["a"], res2["a"])) > 0.1 and
              all((p is None) == (q is None) and (p is None or abs(p - q) < 1e-9 * 1e6 + 1)
                  for p, q in zip(res["experimental"], res2["experimental"])) and
              abs(max(v for v in res["experimental"] if v is not None) -
                  max(res["a"])) < 1e-6 * max(res["a"]))
        check("empty outside the data", res["experimental"][0] is None and
              res["experimental"][2000] is not None)

        cli = os.path.join(REPO, "scripts", "ecce_spectrum.py")
        p = subprocess.run([sys.executable, cli, FIX, sc, "--exp", et, "--lo", "1000",
                            "--hi", "2000", "--step", "5", "--csv"],
                           capture_output=True, text=True)
        lines = p.stdout.splitlines()
        check("CSV: header and rows", p.returncode == 0 and lines[0] ==
              "wavenumber_cm-1,intensity_calc-water-vib,intensity_scaled,"
              "difference_scaled_minus_calc-water-vib,experimental" and len(lines) == 202,
              (p.stderr, lines[:1], len(lines)))
        p = subprocess.run([sys.executable, cli, "--parse-exp", ep], capture_output=True, text=True)
        check("--parse-exp lines", p.stdout.startswith("kind\ttransmittance\n"), p.stdout[:40])
    finally:
        shutil.rmtree(tmp, True)
    print("FAILED: %s" % ", ".join(fails) if fails else "all passed")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
