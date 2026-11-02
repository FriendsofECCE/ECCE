#!/usr/bin/env python3
"""ecce_si_export on stored ORCA, G16 and ECCE-QM calculations (#625)."""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
SCRIPT = os.path.join(ROOT, "scripts", "ecce_si_export.py")
FIX = os.path.join(ROOT, "tests", "apps", "fixtures")
QM = os.path.join(ROOT, "tests", "ecceqm", "fixtures", "h2o-geovib")
fails = []


def check(name, cond, info=""):
    print(("ok   " if cond else "FAIL ") + name + ("" if cond else " " + str(info)))
    if not cond:
        fails.append(name)


def stored(calc, prop):
    return ET.parse(os.path.join(calc, "Props", prop)).getroot().text.strip()


def final_xyz(calc):
    sys.path.insert(0, os.path.join(ROOT, "scripts"))
    import ecce_si_export as S
    r = S.read_calc(calc)
    return r["elements"], r["coords"]


def parse_xyz(text):
    lines = text.splitlines()
    out, i = [], 0
    while i < len(lines):
        n = int(lines[i])
        head = lines[i + 1]
        atoms = [l.split() for l in lines[i + 2:i + 2 + n]]
        out.append((head, [(a[0], [float(v) for v in a[1:4]]) for a in atoms]))
        i += 2 + n
    return out


tmp = tempfile.mkdtemp(prefix="siexport-")
try:
    # ORCA: the stored fixture has no energy; give the copy one.
    orca = os.path.join(tmp, "orca_water")
    shutil.copytree(os.path.join(FIX, "calc-orca-water-ccpvdz"), orca)
    open(os.path.join(orca, "Props", "TE"), "w").write(
        '<?xml version="1.0"?>\n<value name="TE" units="Hartree">-76.0267123456</value>\n')
    # G16 with a frequency list that has one imaginary mode.
    g16 = os.path.join(tmp, "g16_water")
    shutil.copytree(os.path.join(FIX, "calc-water-freq-g16"), g16)
    open(os.path.join(g16, "Props", "VIBFREQ"), "w").write(
        '<?xml version="1.0"?>\n<vector name="VIBFREQ" rows="3" units="cm-1">'
        '-123.4   1645.0   3866.2</vector>\n')
    calcs = [orca, g16, QM]
    out = os.path.join(tmp, "si.xyz")
    p = subprocess.run([sys.executable, SCRIPT] + calcs + ["-o", out, "--table", "latex"],
                       capture_output=True, text=True)
    check("CLI exits 0", p.returncode == 0, p.stderr)
    recs = parse_xyz(open(out).read())
    check("three structures", len(recs) == 3, len(recs))
    for calc, (head, atoms) in zip(calcs, recs):
        name = os.path.basename(calc)
        check(name + ": name first", head.split()[0] == name, head)
        els, xyz = final_xyz(calc)
        check(name + ": elements", [a[0] for a in atoms] == els)
        check(name + ": geometry to 1e-6", all(
            abs(a[1][k] - xyz[i][k]) < 1e-6 for i, a in enumerate(atoms) for k in range(3)))
        e = re.search(r"E=(\S+)", head).group(1)
        g = re.search(r"G=(\S+)", head).group(1)
        check(name + ": E equals stored", e == stored(calc, "TE"), e)
        gs = stored(calc, "EGIBBS") if os.path.exists(
            os.path.join(calc, "Props", "EGIBBS")) else "-"
        check(name + ": G equals stored", g == gs, g)
    check("ORCA geometry is the stored chemsys one",
          recs[0][1][0][0] == "O" and len(recs[0][1]) == 3)
    check("code and method in the comment",
          "code=ORCA" in recs[0][0] and "code=Gaussian-16" in recs[1][0]
          and "code=ECCE-QM" in recs[2][0] and "method=DFT/RDFT" in recs[0][0], recs[0][0])
    check("QM geometry is the last trace frame", abs(recs[2][1][0][1][2] - 0.1246398629) < 1e-6)
    tex = open(os.path.join(tmp, "si.tex")).read()
    check("tabular balanced", tex.count(r"\begin{tabular}") == 1 and
          tex.count(r"\end{tabular}") == 1)
    body = [l for l in tex.splitlines() if l.endswith(r"\\")]
    check("rows have equal column counts", len({l.count("&") for l in body}) == 1, body)
    check("underscores escaped", "orca_water" not in tex and r"orca\_water" in tex)
    check("no bare underscore outside math", not re.search(r"(?<!\\)_", re.sub(r"\$[^$]*\$", "", tex)))
    g16 = [l for l in body if "g16" in l.lower()]
    check("imaginary count 1 for the G16 copy",
          g16 and [c.strip() for c in g16[0].rstrip("\\").split("&")][7] == "1", tex)
    # CSV and a bad folder.
    p = subprocess.run([sys.executable, SCRIPT] + calcs + ["-o", out, "--table", "csv"],
                       capture_output=True, text=True)
    csv = open(os.path.join(tmp, "si.csv")).read().splitlines()
    head = csv[0].split(",") if csv else []
    check("csv header + 3 rows", len(csv) == 4 and "nimag" in head and head[-1] == "excluded", csv)
    p = subprocess.run([sys.executable, SCRIPT, os.path.join(tmp, "none"), "-o", out],
                       capture_output=True, text=True)
    check("missing folder fails", p.returncode != 0)
finally:
    shutil.rmtree(tmp, ignore_errors=True)
sys.exit(1 if fails else 0)
