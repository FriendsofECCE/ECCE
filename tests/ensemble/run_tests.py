#!/usr/bin/env python3
"""ecce-ensemble (#836): pick, minimum and benchmark on synthetic calculations.

Own HOME and XDG_CACHE_HOME in a temp dir.  The minimum/benchmark parts run
ecce-thermo from a DESTDIR install: set ECCE_TEST_HOME (<destdir>/usr/local)
and ECCE_TEST_WRAPPERS (<destdir>/usr/bin); without them those parts SKIP and
the exit status is 77 if nothing else failed.
"""
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPT = os.path.join(REPO, "scripts", "ecce_ensemble.py")
FIX = os.path.join(REPO, "tests", "apps", "fixtures")
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))
fails = []


def check(name, cond, info=""):
    print(("ok   " if cond else "FAIL ") + name + ("" if cond else " " + str(info)))
    if not cond:
        fails.append(name)


TMP = tempfile.mkdtemp(prefix="ensemble-test-")
HOME = os.path.join(TMP, "home")
os.makedirs(HOME)
ENV = dict(os.environ, HOME=HOME, XDG_CACHE_HOME=os.path.join(HOME, ".cache"),
           ECCE_REALUSERHOME=HOME)
if os.environ.get("ECCE_TEST_HOME"):
    ENV["ECCE_HOME"] = os.environ["ECCE_TEST_HOME"]
    ENV["PATH"] = os.environ.get("ECCE_TEST_WRAPPERS", "") + os.pathsep + ENV["PATH"]


def run(*args, env=None):
    r = subprocess.run([sys.executable, SCRIPT] + list(args), capture_output=True,
                       text=True, env=env or ENV)
    return r


def write_ensemble(name, els, input_xyz, frames, rel, charge=0):
    """A CREST-like calculation: chemsys.mvm = input, GEOMTRACE frames, CONFREL."""
    d = os.path.join(TMP, name)
    shutil.copytree(os.path.join(FIX, "crest-butane-ensemble"), d)
    with open(os.path.join(d, "Parameters", "chemsys.mvm"), "w") as f:
        f.write("title: t\ntype: molecule\nnum_atoms: %d\natom_info: symbol cart\n"
                "atom_list: \n" % len(els))
        for e, c in zip(els, input_xyz):
            f.write("%s %.6f %.6f %.6f\n" % ((e,) + tuple(c)))
        f.write("attr_list:\ncharge: %d\n" % charge)
    labels = " ".join("%d-%s" % (i + 1, e) for i, e in enumerate(els))
    with open(os.path.join(d, "Props", "GEOMTRACE"), "w") as f:
        f.write('<?xml version="1.0"?>\n<tsvectable columns="3" name="GEOMTRACE" '
                'rowLabels="%s" rows="%d" units="Angstrom">\n' % (labels, len(els)))
        for k, fr in enumerate(frames):
            f.write('<step number="%d">%s</step>\n' % (
                k + 1, "   ".join("%.8f" % v for c in fr for v in c)))
        f.write("</tsvectable>\n")
    with open(os.path.join(d, "Props", "CONFREL"), "w") as f:
        f.write('<?xml version="1.0"?>\n<tsvector name="CONFREL" rows="%d" '
                'units="kcal/mol">\n' % len(rel))
        for k, v in enumerate(rel):
            f.write('<step number="%d">%.3f</step>\n' % (k + 1, v))
        f.write("</tsvector>\n")
    return d


# Methylammonium Na+ Cl-: C N H3 H3 (ammonium) with Na bound to O of water, Cl near NH.
els = ["N", "C", "O", "Na", "Cl", "H", "H", "H", "H", "H", "H", "H", "H"]
base = [(0, 0, 0), (1.5, 0, 0), (0, 3.0, 0), (0, 5.3, 0), (-3.0, -1.5, 0),
        (-0.4, 0.9, 0), (-0.4, -0.5, 0.8), (-0.4, -0.5, -0.8),  # NH3 on N
        (1.9, 1.0, 0), (1.9, -0.5, 0.9), (1.9, -0.5, -0.9),     # CH3 on C
        (0.8, 3.3, 0), (-0.8, 3.3, 0)]                           # water H on O
els = els[:5] + ["H"] * 8
assert len(base) == len(els)


def shifted(f):
    return [tuple(x + dx for x, dx in zip(c, f(i))) for i, c in enumerate(base)]


f_input = base
f_dupe = shifted(lambda i: (0, 0.25, 0) if i == 1 else (0, 0, 0))   # same conformer, one atom 0.25 A off
f_other = shifted(lambda i: (0, 0, 3.0) if i in (1, 8, 9, 10) else (0, 0, 0))   # CH3 end displaced
f_proton = [c for c in base]
f_proton[5] = (0.2, 3.0 + 1.0, 0.0)                          # NH proton moved to the water O
f_ion = shifted(lambda i: (0, 9.0, 0) if i == 3 else (0, 0, 0))   # Na leaves the water
frames = [f_input, f_dupe, f_other, f_proton, f_ion]
rel = [0.0, 0.1, 1.0, 2.0, 3.0]
calc = write_ensemble("ens", els, f_input, frames, rel)

# 1. pick -------------------------------------------------------------
r = run("pick", calc, "n=3", "--json")
j = json.loads(r.stdout)
check("pick: exit 0", r.returncode == 0, r.stderr)
check("pick: near-duplicate merged into the lowest", j["picks"][0]["merged"] == [2], j["picks"][0])
ranks = [p["member"] for p in j["picks"]]
check("pick: lowest first, distinct members 1,3,4", ranks == [1, 3, 4], ranks)
check("pick: first pick passes all checks", j["picks"][0]["ok"], j["picks"][0]["flags"])
p4 = j["picks"][2]
check("pick: proton transfer flagged with both atoms",
      not p4["protonation_ok"] and {c["atom"] for c in p4["changed_protons"]} == {"N1", "O3"},
      p4["changed_protons"])
check("pick: counter-ion contacts counted (Na-O in the input)",
      any(i["ion"] == "Na4" and i["bound"] and i["contacts"] == 1 for i in j["picks"][0]["ions"]),
      j["picks"][0]["ions"])
check("pick: Na-O listed as a metal contact at 2.30 A",
      [(m["metal"], m["donor"], m["distance"]) for m in j["picks"][0]["metal_contacts"]]
      == [("Na4", "O3", 2.3)], j["picks"][0]["metal_contacts"])
j5 = json.loads(run("pick", calc, "n=5", "--rmsd", "0.05", "--json").stdout)
check("pick: --rmsd 0.05 keeps the slightly distorted copy distinct", [p["member"] for p in j5["picks"]] == [1, 2, 3, 4, 5],
      [p["member"] for p in j5["picks"]])
p5 = j5["picks"][4]
check("pick: ion that left is flagged and not bound",
      not p5["ok"] and any(not i["bound"] for i in p5["ions"]) and
      "metal_contacts_lost" in p5 and p5["metal_contacts_lost"] == ["Na4-O3"], p5)
js = json.loads(run("pick", calc, "n=3", "--skip-failed", "--json").stdout)
check("pick: --skip-failed skips the proton transfer and the ion loss",
      [p["member"] for p in js["picks"]] == [1, 3] and
      [s["member"] for s in js["skipped"]] == [4, 5], js)
def wpick(*args):
    jj = json.loads(run("pick", calc, *args, "--json").stdout)
    return [p["member"] for p in jj["picks"]], jj


m, jw = wpick("window=1.5")
check("window: alone, no cap at 5, merge before window", m == [1, 3], m)
check("window: JSON records the window used", jw["window_kcal_mol"] == 1.5, jw["window_kcal_mol"])
check("window: 3kcal keeps everything up to 3.0", wpick("window=3kcal")[0] == [1, 3, 4, 5])
check("window: plain number is kcal/mol", wpick("window=1.5")[0] == wpick("window=1.5kcal")[0])
check("window: with n as an upper limit", wpick("window=3", "n=2")[0] == [1, 3])
check("window: n larger than the window keeps the window", wpick("window=1.5", "n=4")[0] == [1, 3])
check("window: kJ (12.5 kJ = 2.99 kcal) keeps 1,3,4", wpick("window=12.5kJ")[0] == [1, 3, 4])
check("window: 4.184 kJ is exactly 1 kcal", wpick("window=4.184kJ")[0] == [1, 3])
check("window: a window that keeps only the lowest", wpick("window=0.5")[0] == [1])
check("window: absent -> null and default n=5", wpick()[1]["window_kcal_mol"] is None)
check("window: bad value refused", run("pick", calc, "window=abc").returncode == 2)
check("--help documents window and the order", all(s in run("--help").stdout
      for s in ("window=E", "merge first, then window, then n")))
r = run("pick", os.path.join(FIX, "calc-water-opt"), "--json")
check("pick: a non-ensemble calculation is refused", r.returncode != 0 and "CONFREL" in r.stderr, r.stderr)
check("pick: text output runs", run("pick", calc).returncode == 0)
check("--help documents the threshold and the spec format",
      all(s in run("--help").stdout for s in ("0.50 A", "ecce-benchmark-1", "reaction: LABEL")))

# --create goes through ecce-series (a recording stand-in on PATH)
fake = os.path.join(TMP, "fakebin")
os.makedirs(fake)
log = os.path.join(TMP, "series.log")
open(os.path.join(fake, "ecce-series"), "w").write(
    '#!/bin/sh\necho "$@" > %s\nshift 2\nfor f in "$@"; do head -2 "$f" >> %s; '
    'echo "$(basename $f .xyz): made x"; done\n' % (log, log))
os.chmod(os.path.join(fake, "ecce-series"), 0o755)
env2 = dict(ENV, PATH=fake + os.pathsep + ENV["PATH"])
r = run("pick", calc, "n=2", "--create", "proj/template", "proj", env=env2)
txt = open(log).read() if os.path.exists(log) else ""
check("create: ecce-series called with template, folder and one xyz per pick",
      r.returncode == 0 and txt.startswith("proj/template proj ") and txt.count(".xyz") == 2
      and "charge=0 mult=1" in txt and "ens_conf1: made x" in r.stdout, (r.stdout, txt, r.stderr))

# 2/3. minimum and benchmark need ecce-thermo ---------------------------
if not os.environ.get("ECCE_TEST_HOME"):
    print("SKIP minimum/benchmark: ECCE_TEST_HOME not set")
    shutil.rmtree(TMP, ignore_errors=True)
    sys.exit(1 if fails else 77)

import thermo_panel_test as tp  # noqa: E402

proj = os.path.join(TMP, "proj")
os.makedirs(proj)
F = [25.0, 60.0, 150.0, 1645.0, 3866.0, 3965.0]


def calc_g(name, g, te, freqs=F, code="ORCA"):
    d = tp.makeThermoCalc(proj, name, freqs, code=code)
    for key, val in (("EGIBBS", g), ("TE", te)):
        open(os.path.join(d, "Props", key), "w").write(
            '<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<value name="%s" units="Hartree">%.8f</value>\n' % (key, val))
    return d


calc_g("conf_a", -76.000010, -76.05)
calc_g("conf_b", -76.000100, -76.06)                     # lowest G, a minimum? no: imaginary below
calc_g("conf_c", -76.000050, -76.06)
calc_g("conf_img", -76.000500, -76.07, freqs=[-80.0] + F)  # lowest of all but not a minimum
bad = calc_g("conf_noG", -1.0, -1.0)
os.remove(os.path.join(bad, "Props", "EGIBBS"))
nof = calc_g("conf_nofreq", -76.0009, -76.07)
os.remove(os.path.join(nof, "Props", "VIBFREQ"))
calc_g("conf_nwchem", -76.0009, -76.07, code="NWChem")

r = run("minimum", os.path.join(proj, "conf_[abc]"), os.path.join(proj, "conf_img"),
        os.path.join(proj, "conf_noG"), os.path.join(proj, "conf_nofreq"), "--json")
j = json.loads(r.stdout)
st = {x["name"]: x["status"] for x in j["rows"]}
check("minimum: lowest-G minimum is conf_b", j["chosen"] and j["chosen"]["name"] == "conf_b", j["chosen"])
check("minimum: imaginary mode excluded and named", st["conf_img"].startswith("excluded: 1 imaginary"), st)
check("minimum: no frequencies and no G listed and excluded",
      st["conf_nofreq"] == "excluded: no frequencies" and st["conf_noG"] == "excluded: no G", st)
check("minimum: dG to the chosen in kJ/mol",
      abs([x for x in j["rows"] if x["name"] == "conf_a"][0]["dG_kJ_mol"] - 0.00009 * 2625.4996) < 1e-2)
r = run("minimum", proj, "--json")
check("minimum: a folder with mixed codes is refused", r.returncode == 1 and
      json.loads(r.stdout)["chosen"] is None and json.loads(r.stdout)["mixed_methods"])
r = run("minimum", os.path.join(proj, "conf_[abc]"), "--treatment", "rrho", "--json")
jt = json.loads(r.stdout)
check("minimum: --treatment recomputes G with ecce-thermo",
      r.returncode == 0 and all(x["G_Eh"] is not None and abs(x["G_Eh"] + 76.0) < 0.1 and
                                abs(x["G_Eh"] + 76.0) > 1e-5 for x in jt["rows"]), r.stdout[:300])

# benchmark: dG equals ecce-thermo's own, 1 M term and log K from independent formulas
calc_g("prod", -76.03, -76.09, freqs=[30.0, 90.0, 300.0, 1600.0, 3800.0, 3900.0])
spec = os.path.join(proj, "set1.txt")
open(spec, "w").write(
    "# test\nset: toy\nspecies: W = conf_[abc] conf_img\nspecies: P = prod\n"
    "reaction: iso | W -> P | -2.0 | made up\n"
    "reaction: dim | 2*W -> P | | \n"
    "reaction: bad | W -> Q | 1.0 | x\n".replace("W -> Q", "W -> P, P") if False else
    "# test\nset: toy\nspecies: W = conf_[abc] conf_img\nspecies: P = prod\nspecies: X = conf_img\n"
    "reaction: iso | W -> P | -2.0 | made up\n"
    "reaction: dim | 2*W -> P | | \n"
    "reaction: noMin | W -> X | 1.0 | x\n")
r = run("benchmark", spec, "--treatment", "rrho", "--save", "toy", "--json")
j = json.loads(r.stdout)
rx = {x["label"]: x for x in j["reactions"]}
check("benchmark: picks the lowest-G minimum of the species", j["species"]["W"]["chosen"] == "conf_b", j["species"])
check("benchmark: species without a minimum -> reaction not computed",
      rx["noMin"]["status"].startswith("not computed: no usable minimum for X"), rx["noMin"])
thermo = os.path.join(ENV["ECCE_HOME"], "bin", "ecce-thermo")
if not os.path.isfile(thermo):
    thermo = os.path.join(os.environ["ECCE_TEST_WRAPPERS"], "ecce-thermo")


def thermo_dG(rxn, ss):
    o = subprocess.run([thermo, "file://" + os.path.join(proj, "conf_b"),
                        "file://" + os.path.join(proj, "prod"), "--reaction", rxn,
                        "--standard-state", ss, "--treatment", "rrho", "--json"],
                       capture_output=True, text=True, env=ENV).stdout
    return json.loads(o[o.index("{"):])["deltaG"]["rrho"] * 4.3597447222071e-18 * 6.02214076e23 / 1000


KB = 8.314462618e-3
T = 298.15
want = thermo_dG("conf_b -> prod", "1M")
check("benchmark: dG equals ecce-thermo --reaction at 1 M", abs(rx["iso"]["deltaG_kJ_mol"] - want) < 2e-3,
      (rx["iso"], want))
check("benchmark: log K = -dG/(RT ln 10)",
      abs(rx["iso"]["logK_calc"] + rx["iso"]["deltaG_kJ_mol"] / (KB * T * math.log(10))) < 2e-3)
check("benchmark: calc - exp", abs(rx["iso"]["calc_minus_exp"] - (rx["iso"]["logK_calc"] + 2.0)) < 2e-3)
d1atm = thermo_dG("2*conf_b -> 1*prod", "1atm")
check("benchmark: 1 M term for dn = -1 is -RT ln(24.465)",
      abs((rx["dim"]["deltaG_kJ_mol"] - d1atm) - (-KB * T * math.log(0.0820574 * T))) < 2e-3,
      (rx["dim"]["deltaG_kJ_mol"] - d1atm))
check("benchmark: no experimental value -> no error column", "calc_minus_exp" not in rx["dim"] and rx["dim"]["exp_logK"] is None)
saved = os.path.join(proj, "toy.benchmark.json")
check("benchmark: --save writes the named set next to the spec file",
      os.path.isfile(saved) and json.load(open(saved))["format"] == "ecce-benchmark-1" and
      json.load(open(saved))["name"] == "toy")
check("benchmark: summary has the statistics", j["summary"]["with_experiment"] == 1 and
      abs(j["summary"]["rmse"] - abs(rx["iso"]["calc_minus_exp"])) < 1e-9, j["summary"])
check("benchmark: exit 1 when a reaction was not computed", r.returncode == 1)
check("benchmark: text output runs", run("benchmark", spec, "--treatment", "rrho").returncode == 1)

shutil.rmtree(TMP, ignore_errors=True)
print("%d failure(s)" % len(fails))
sys.exit(1 if fails else 0)
