#!/usr/bin/env python3
"""JCAMP-DX reading (scripts/jcampdx.py, ecce-spectrum) and the export (#986).

    jcamp_tests.py [<jcamp_export_test binary>]
All files here are synthetic.
"""
import math
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
CLI = os.path.join(REPO, "scripts", "ecce_spectrum.py")
sys.path.insert(0, os.path.join(REPO, "scripts"))
import jcampdx as J  # noqa: E402
import ecce_spectrum as S  # noqa: E402

fails = []


def check(name, ok, info=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else " " + str(info)))
    if not ok:
        fails.append(name)


# Steps, runs of equal steps and a line-start Y-check all occur in this series.
YS = [100, 100, 100, 103, 107, 107, 107, 107, 102, 95, 95, 95, 90, 91, 92, 93,
      94, 95, 100, 110]


def header(xu="1/CM", yu="ABSORBANCE", dt="INFRARED SPECTRUM", n=len(YS),
           first=1000.0, last=1019.0, yf=1):
    return ("##TITLE=Test spectrum\n##JCAMP-DX=4.24\n##DATA TYPE=%s\n##ORIGIN=Synthetic\n"
            "##STATE=GAS\n##XUNITS=%s\n##YUNITS=%s\n##XFACTOR=1\n##YFACTOR=%s\n"
            "##FIRSTX=%s\n##LASTX=%s\n##NPOINTS=%d\n" % (dt, xu, yu, yf, first, last, n))


def sqz(v):
    s = str(abs(v))
    return ("@ABCDEFGHI"[int(s[0])] if v >= 0 else "abcdefghi"[int(s[0]) - 1]) + s[1:]


def dif_line(vals):
    """First value as SQZ, the rest as DIF with DUP for runs of equal steps."""
    out = [sqz(vals[0])]
    diffs = [b - a for a, b in zip(vals, vals[1:])]
    i = 0
    while i < len(diffs):
        d = diffs[i]
        run = 1
        while i + run < len(diffs) and diffs[i + run] == d and run < 9:
            run += 1
        s = str(abs(d))
        out.append("%" + s[1:] if d == 0 else
                   ("%JKLMNOPQR"[int(s[0])] if d > 0 else "jklmnopqr"[int(s[0]) - 1]) + s[1:])
        if run > 1:
            out.append("STUVWXYZs"[run - 1])
        i += run
    return "".join(out)


def xydata(lines):
    return "##XYDATA=(X++(Y..Y))\n" + "\n".join(lines) + "\n##END=\n"


AFFN = header() + xydata(["1000 " + " ".join(map(str, YS[:10])),
                          "1010 " + " ".join(map(str, YS[10:]))])
SQZ = header() + xydata(["1000 " + "".join(sqz(v) for v in YS[:10]),
                         "1010 " + "".join(sqz(v) for v in YS[10:])])
# Line 2 starts with the Y-check: the last value of line 1 again.
DIF = header() + xydata(["1000 " + dif_line(YS[:10]), "1009 " + dif_line(YS[9:])])


def jx(path, text):
    with open(path, "w") as f:
        f.write(text)
    return path


def main():
    tmp = tempfile.mkdtemp(prefix="jcamp-test-")
    try:
        for name, text in (("AFFN", AFFN), ("SQZ", SQZ), ("DIF+DUP+Y-check", DIF)):
            sp = J.parse(text)[0]
            check("%s decodes to the 20 values" % name, sp["y"] == YS, (sp["y"], text))
            check("%s x axis" % name, abs(sp["x"][0] - 1000) < 1e-9 and abs(sp["x"][-1] - 1019) < 1e-9)
        check("DIF text uses DIF and DUP", any(c in DIF.split("XYDATA")[1] for c in "JKLjkl%") and
              any(c in DIF.split("XYDATA")[1] for c in "STUVs"), DIF)

        sp = J.parse(header(yf=0.5) + xydata(["1000 " + " ".join(map(str, YS[:10])),
                                              "1010 " + " ".join(map(str, YS[10:]))]))[0]
        check("YFACTOR applied", abs(sp["y"][3] - 51.5) < 1e-9)

        # 5..10 um is 2000..1000 cm-1; the x axis comes back ascending.
        um = header(xu="MICROMETERS", first=5.0, last=10.0, n=6, yu="TRANSMITTANCE") + \
            xydata(["5 1 0.9 0.8 0.5 0.8 0.9"])
        sp = J.parse(um)[0]
        check("micrometres to cm-1", abs(sp["x"][0] - 1000) < 1e-9 and abs(sp["x"][-1] - 2000) < 1e-9, sp["x"])
        check("transmittance kind", sp["kind"] == "transmittance")

        pc = jx(os.path.join(tmp, "pct.jdx"), header(yu="%TRANSMITTANCE", n=3, first=1000, last=1002) +
                xydata(["1000 100 10 50"]))
        x, y, k = S.read_experimental(pc)
        check("%T file: transmittance, 0-100 -> absorbance", k == "transmittance" and
              abs(y[0]) < 1e-9 and abs(y[1] - 1.0) < 1e-9, (k, y))
        tf = jx(os.path.join(tmp, "t.jdx"), header(yu="TRANSMITTANCE", n=3, first=1000, last=1002) +
                xydata(["1000 1 0.1 0.5"]))
        check("fractional T: -log10", abs(S.read_experimental(tf)[1][1] - 1.0) < 1e-9)
        af = jx(os.path.join(tmp, "a.jdx"), AFFN)
        x, y, k = S.read_experimental(af, "transmittance")
        check("units from the file beat the user's kind", k == "absorbance" and y == [float(v) for v in YS])

        pk = ("##TITLE=Peaks\n##DATA TYPE=INFRARED PEAK TABLE\n##XUNITS=1/CM\n##YUNITS=ABSORBANCE\n"
              "##NPOINTS=2\n##PEAK TABLE=(XY..XY)\n1600, 0.8 $$ a\n3000, 0.4\n##END=\n")
        sp = J.parse(pk)[0]
        check("PEAK TABLE read", sp["form"] == "PEAKTABLE" and max(sp["y"]) == 0.8 and
              abs(sp["x"][sp["y"].index(0.8)] - 1600) < 1e-9)
        sp = J.parse(header(n=3) + "##XYPOINTS=(XY..XY)\n1000, 1; 1010, 2; 1020, 3\n##END=\n")[0]
        check("XYPOINTS read", sp["y"] == [1, 2, 3] and sp["x"][1] == 1010)

        nist = ("##TITLE=WATER\n##JCAMP-DX=4.24\n##DATA TYPE=INFRARED SPECTRUM\n"
                "##CAS REGISTRY NO=0-00-0\n##ORIGIN=SYNTHETIC SET\n##OWNER=none\n"
                "##STATE=GAS (0.1 mmHg DILUTED WITH N2 TO A TOTAL PRESSURE OF 600 mmHg)\n"
                "##RESOLUTION=4\n##XUNITS=1/CM\n##YUNITS=TRANSMITTANCE\n##XFACTOR=1.000000\n"
                "##YFACTOR=0.000100\n##FIRSTX=450.0\n##LASTX=452.0\n##NPOINTS=3\n"
                "##FIRSTY=0.99\n##XYDATA=(X++(Y..Y))\n450 9900 9000 5000\n##END=\n")
        nf = jx(os.path.join(tmp, "nist.jdx"), nist)
        x, y, k, label = S.read_experimental_info(nf)
        check("NIST-style header and label", k == "transmittance" and label.startswith("WATER, GAS")
              and "SYNTHETIC SET" in label and abs(y[2] - math.log10(2)) < 1e-9, (k, label, y))

        two = "##TITLE=Link\n##JCAMP-DX=4.24\n##DATA TYPE=LINK\n##BLOCKS=2\n" + AFFN + AFFN + "##END=\n"
        lf = jx(os.path.join(tmp, "link.jdx"), two)
        x, y, k, label = S.read_experimental_info(lf)
        check("LINK file: first spectrum, and says so", "first of 2 spectra" in label and len(x) == 20, label)

        def bad(name, text, want):
            p = jx(os.path.join(tmp, name), text)
            r = subprocess.run([sys.executable, CLI, "--parse-exp", p], capture_output=True, text=True)
            err = r.stderr.strip()
            check(name + " gives a one-line error", r.returncode == 1 and len(err.splitlines()) == 1
                  and want in err, err)
        bad("nmr.jdx", "##TITLE=x\n##DATA TYPE=NMR SPECTRUM\n##XUNITS=HZ\n" + xydata(["1 2 3"]),
            "Not an infrared or Raman")
        bad("ms.jdx", "##TITLE=x\n##DATA TYPE=MASS SPECTRUM\n##PEAK TABLE=(XY..XY)\n1,2\n##END=\n",
            "Not an infrared or Raman")
        bad("units.jdx", header(xu="HZ") + xydata(["1000 1 2"]), "XUNITS")

        p = subprocess.run([sys.executable, CLI, "--parse-exp", nf], capture_output=True, text=True)
        check("--parse-exp prints kind and label",
              p.stdout.startswith("kind\ttransmittance\nlabel\tWATER, GAS"), p.stdout[:60])

        if len(sys.argv) > 1:       # the export, read back
            out = os.path.join(tmp, "export")
            os.mkdir(out)
            subprocess.run([sys.argv[1], out], check=True)
            text = open(os.path.join(out, "curve.jdx")).read()
            check("export header lines", "##$BROADENING=Gaussian, FWHM 15 cm-1" in text and
                  "##$FREQUENCY SCALING FACTOR=0.96" in text and "##XYDATA=(X++(Y..Y))" in text
                  and "##PEAK TABLE=(XY..XY)" in text)
            blocks = J.parse(text)
            check("export: curve block then peak block",
                  [b["form"] for b in blocks] == ["XYDATA", "PEAKTABLE"])
            csv = [l.split(",") for l in open(os.path.join(out, "curve.csv")).read().splitlines()]
            at = [i for i, l in enumerate(csv) if l[0] == "wavenumber_cm-1"][0]
            rows = [(float(r[0]), float(r[1])) for r in csv[at + 1:] if len(r) == 2]
            cur = blocks[0]
            top = max(v for _, v in rows)
            err = max(abs(a - b[1]) for a, b in zip(cur["y"], rows))
            check("curve equals the CSV export", len(cur["y"]) == len(rows) and err < 1e-6 * top and
                  abs(cur["x"][0] - rows[0][0]) < 1e-9 and abs(cur["x"][-1] - rows[-1][0]) < 1e-9,
                  (err, len(rows), len(cur["y"])))
            peaks = blocks[1]["x"][1::3]
            want = [2169.8577 * 0.96, 4141.5426 * 0.96, 4392.6459 * 0.96]
            check("peak table = scaled sticks, mode numbers kept",
                  all(abs(a - b) < 1e-3 for a, b in zip(sorted(peaks), sorted(want))) and
                  "$$ mode 3" in text, peaks)
            only = J.parse(open(os.path.join(out, "sticks.jdx")).read())
            check("width 0: only the peak block", len(only) == 1 and only[0]["form"] == "PEAKTABLE")
            x, y, k, label = S.read_experimental_info(os.path.join(out, "curve.jdx"))
            check("export reads back through ecce-spectrum", len(x) == len(rows) and "water" in label,
                  (k, label))
    finally:
        shutil.rmtree(tmp, True)
    print("FAILED: %s" % ", ".join(fails) if fails else "all passed")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
