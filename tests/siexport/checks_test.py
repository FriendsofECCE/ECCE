#!/usr/bin/env python3
"""ecce-si-export --checks on stored calculations (#615).

Four calculations: a converged opt+freq, a not-converged opt, a TS with one
imaginary mode, an open-shell calc with <S2> off S(S+1).  Every column is
compared with the stored value; a missing value must read "-".
"""
import csv
import io
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
SCRIPT = os.path.join(ROOT, "scripts", "ecce_si_export.py")
QM = os.path.join(ROOT, "tests", "ecceqm", "fixtures")
TS = os.path.join(ROOT, "data", "client", "examples", "workflows", "sn2-cl-ch3br",
                  "g16-qst3", "stored", "sn2_qst3")
NS = "http://www.emsl.pnl.gov/ecce:"
fails = []


def check(name, cond, info=""):
    print(("ok   " if cond else "FAIL ") + name + ("" if cond else " " + str(info)))
    if not cond:
        fails.append(name)


def setmeta(calc, key, value):
    path = os.path.join(calc, ".ecce-meta")
    lines = [l for l in open(path) if ":" + key + "\t" not in l]
    lines.append(".\t%s%s\t\t%s\n" % (NS, key, value))
    open(path, "w").writelines(lines)


def setprop(calc, name, text):
    open(os.path.join(calc, "Props", name), "w").write(
        '<?xml version="1.0"?>\n<value name="%s" units="NA">%s</value>\n' % (name, text))


def stored(calc, prop):
    p = os.path.join(calc, "Props", prop)
    if not os.path.exists(p):
        return "-"
    import xml.etree.ElementTree as ET
    return ET.parse(p).getroot().text.strip()


tmp = tempfile.mkdtemp(prefix="checks-")
try:
    good = os.path.join(tmp, "opt_freq")
    shutil.copytree(os.path.join(QM, "h2o-geovib"), good)
    setmeta(good, "progress", "# seq=3\\nscf cycle=9 converged=yes\\nopt step=4 converged=yes\\n")

    bad = os.path.join(tmp, "opt_notconv")
    shutil.copytree(os.path.join(QM, "h2o-geovib"), bad)
    setmeta(bad, "state", "Unsuccessful")
    setmeta(bad, "progress", "# seq=3\\nscf cycle=9 converged=yes\\nopt step=100 converged=no\\n")

    ts = os.path.join(tmp, "ts_one_imag")
    shutil.copytree(TS, ts)

    radical = os.path.join(tmp, "triplet_s2")
    shutil.copytree(os.path.join(QM, "o2-roks"), radical)
    setprop(radical, "S2", "2.2500")
    excl = os.path.join(tmp, "excluded_one")
    shutil.copytree(os.path.join(QM, "o2-roks"), excl)
    setmeta(excl, "excluded", "wrong spin state")

    calcs = [good, bad, ts, radical, excl]
    p = subprocess.run([sys.executable, SCRIPT, "--checks", "csv"] + calcs,
                       capture_output=True, text=True)
    check("CLI exits 0", p.returncode == 0, p.stderr)
    rows = {r["name"]: r for r in csv.DictReader(io.StringIO(p.stdout))}
    check("five rows", len(rows) == 5, list(rows))

    g = rows["opt_freq"]
    check("good: terminated, converged", (g["normal_termination"], g["opt_converged"],
          g["scf_converged"]) == ("yes", "yes", "yes"), g)
    check("good: 0 imaginary", g["imaginary_modes"] == "0", g)
    check("good: E, ZPE, G equal stored", (g["E_el"], g["ZPE"], g["G"]) ==
          (stored(good, "TE"), stored(good, "EZEROPT"), stored(good, "EGIBBS")), g)
    check("good: charge/mult/method", (g["charge"], g["multiplicity"], g["method"]) ==
          ("0", "1", "DFT/DFT"), g)
    check("good: no problems", g["problems"] == "" and g["solvent"] == "-", g)

    b = rows["opt_notconv"]
    check("notconverged: abnormal, opt no, scf yes", (b["normal_termination"],
          b["opt_converged"], b["scf_converged"]) == ("no", "no", "yes"), b)
    check("notconverged: flagged", "not converged" in b["problems"], b)

    t = rows["ts_one_imag"]
    check("TS: one imaginary mode, not a problem", t["imaginary_modes"] == "1" and
          t["problems"] == "", t)
    check("TS: charge -1, ZPE equals stored", t["charge"] == "-1" and
          t["ZPE"] == stored(ts, "EZEROPT"), t)
    # The same TS with a frequency list lacking the imaginary mode is wrong.
    ts0 = os.path.join(tmp, "ts_no_imag")
    shutil.copytree(TS, ts0)
    open(os.path.join(ts0, "Props", "VIBFREQ"), "w").write(
        '<vector name="VIBFREQ" rows="3" units="cm-1">10.0 20.0 30.0</vector>\n')
    p0 = subprocess.run([sys.executable, SCRIPT, "--checks", "csv", ts0],
                        capture_output=True, text=True)
    check("TS without imaginary mode is flagged", "expected 1" in p0.stdout, p0.stdout)

    r = rows["triplet_s2"]
    check("radical: S2 and deviation", (r["S2"], r["S2_deviation"], r["multiplicity"]) ==
          ("2.2500", "0.2500", "3"), r)
    check("radical: flagged", "<S2>" in r["problems"], r)
    check("radical: missing values are '-'", (r["opt_converged"], r["ZPE"], r["G"],
          r["imaginary_modes"]) == ("-",) * 4, r)
    check("excluded: reason listed", rows["excluded_one"]["excluded"] == "wrong spin state")
    check("O2 as stored (S2=2): no problem", rows["excluded_one"]["problems"] == "")

    out = subprocess.run([sys.executable, SCRIPT, "--checks", "latex", good],
                         capture_output=True, text=True).stdout
    check("LaTeX table", out.startswith("\\begin{tabular}") and "opt\\_freq" in out, out)

    # Core-hours (#627). 28 cores x "1 0:00" = 672 booked; start to end 2 h
    # 30 min = 70 used.  The same figures as tests/machregister/testLaunchInfoRows.C.
    chd = os.path.join(tmp, "folder")
    os.makedirs(chd)
    a = os.path.join(chd, "run_a")
    shutil.copytree(os.path.join(QM, "h2o-geovib"), a)
    for k, v in (("launch_totalprocs", "28"), ("launch_maxwall", "1 0:00"),
                 ("startdate", "Fri, 30 Oct 2026 03:00:00 GMT"),
                 ("completiondate", "Fri, 30 Oct 2026 05:30:00 GMT")):
        setmeta(a, k, v)
    b2 = os.path.join(chd, "run_b")      # booked only: still waiting or killed unstarted
    shutil.copytree(os.path.join(QM, "h2o-geovib"), b2)
    for k, v in (("launch_totalprocs", "4"), ("launch_maxwall", "0 0:30"),
                 ("startdate", ""), ("completiondate", "")):
        setmeta(b2, k, v)
    c2 = os.path.join(chd, "run_c")      # nothing stored: never 0
    shutil.copytree(os.path.join(QM, "h2o-geovib"), c2)
    for k in ("launch_totalprocs", "launch_maxwall", "startdate", "completiondate"):
        setmeta(c2, k, "")
    pc = subprocess.run([sys.executable, SCRIPT, "--checks", "csv", "--total", chd],
                        capture_output=True, text=True)
    cr = {r["name"]: r for r in csv.DictReader(io.StringIO(pc.stdout))}
    check("folder expands to its calculations plus a total",
          list(cr) == ["run_a", "run_b", "run_c", "total"], list(cr))
    check("booked 28 x 24 h = 672.0, used 28 x 2.5 h = 70.0",
          (cr["run_a"]["booked_coreh"], cr["run_a"]["used_coreh"]) == ("672.0", "70.0"), cr["run_a"])
    check("booked without a start: used is '-'",
          (cr["run_b"]["booked_coreh"], cr["run_b"]["used_coreh"]) == ("2.0", "-"), cr["run_b"])
    check("nothing stored: '-', not 0",
          (cr["run_c"]["booked_coreh"], cr["run_c"]["used_coreh"]) == ("-", "-"), cr["run_c"])
    check("folder total is the sum of the column, missing values add nothing",
          (cr["total"]["booked_coreh"], cr["total"]["used_coreh"]) == ("674.0", "70.0"), cr["total"])
    allmissing = subprocess.run([sys.executable, SCRIPT, "--checks", "csv", "--total", c2],
                                capture_output=True, text=True).stdout.splitlines()[-1]
    check("total of calculations with no values is '-'", allmissing.startswith("total,") and
          "0.0" not in allmissing, allmissing)
finally:
    shutil.rmtree(tmp, ignore_errors=True)

sys.exit(1 if fails else 0)
