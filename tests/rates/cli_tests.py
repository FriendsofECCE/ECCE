#!/usr/bin/env python3
"""ecce-rate (#872) on the stored SN2 workflow calculations, checked against a
hand computation from ecce-thermo's own reaction dG.

Own HOME and XDG_CACHE_HOME in a temp dir.  ecce-thermo comes from a DESTDIR
install: set ECCE_TEST_HOME (<destdir>/usr/local) and ECCE_TEST_WRAPPERS
(<destdir>/usr/bin); without them the test SKIPs (exit 77).
"""
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPT = os.path.join(REPO, "scripts", "ecce_rate.py")
WF = os.path.join(REPO, "data", "client", "examples", "workflows", "sn2-cl-ch3br")
REACT = os.path.join(WF, "g16-reactant", "stored", "sn2_reactant")
TS = os.path.join(WF, "g16-qst3", "stored", "sn2_qst3")
PROD = os.path.join(WF, "g16-product", "stored", "sn2_product")
sys.path.insert(0, os.path.join(REPO, "scripts"))
import ecce_rates as R  # noqa: E402

if not os.environ.get("ECCE_TEST_HOME"):
    print("SKIP: ECCE_TEST_HOME not set (needs a DESTDIR install)")
    sys.exit(77)

fails = []


def check(name, cond, info=""):
    print(("ok   " if cond else "FAIL ") + name + ("" if cond else " " + str(info)))
    if not cond:
        fails.append(name)


TMP = tempfile.mkdtemp(prefix="rate-test-")
HOME = os.path.join(TMP, "home")
os.makedirs(HOME)
ENV = dict(os.environ, HOME=HOME, XDG_CACHE_HOME=os.path.join(HOME, ".cache"),
           ECCE_REALUSERHOME=HOME, ECCE_HOME=os.environ["ECCE_TEST_HOME"])
ENV["PATH"] = os.environ.get("ECCE_TEST_WRAPPERS", "") + os.pathsep + ENV["PATH"]
KJ = 4.3597447222071e-18 * 6.02214076e23 / 1000.0


def rate(*args):
    return subprocess.run([sys.executable, SCRIPT] + list(args), capture_output=True,
                          text=True, env=ENV)


def thermo_dG(reaction, ss, T, treatment="qrrho-grimme:100"):
    r = subprocess.run(["ecce-thermo", "file://" + REACT, "file://" + TS, "file://" + PROD,
                        "--reaction", reaction, "--standard-state", ss, "--T", str(T),
                        "--treatment", treatment, "--json"], capture_output=True, text=True, env=ENV)
    return json.loads(r.stdout[r.stdout.index("{"):])["deltaG"][treatment] * KJ


try:
    T = 298.15
    RT = 8.314462618e-3 * T
    pref = 1.380649e-23 * T / 6.62607015e-34

    # Unimolecular step, reactant complex -> TS -> product complex.
    r = rate(REACT, TS, PROD, "--json")
    check("unimolecular exits 0", r.returncode == 0, r.stderr)
    j = json.loads(r.stdout)
    row = j["temperatures"][0]
    dg = thermo_dG("sn2_reactant -> sn2_qst3", "1M", T)
    check("dG‡ = ecce-thermo reaction dG (%.3f kJ/mol)" % dg, abs(row["dG_act"] - dg) < 2e-4,
          row["dG_act"])
    k0 = pref * math.exp(-dg / RT)
    check("k(298 K) TST = kB T/h exp(-dG‡/RT) = %.4e s⁻¹" % k0, abs(row["k"]["none"] / k0 - 1) < 1e-5,
          row["k"]["none"])
    freqs = [float(x) for x in ET.parse(os.path.join(TS, "Props", "VIBFREQ")).getroot().text.split()]
    nu = min(freqs)
    kw = 1 + (6.62607015e-34 * 29979245800.0 * abs(nu) / (1.380649e-23 * T)) ** 2 / 24
    check("Wigner kappa = %.4f" % kw, abs(row["kappa"]["wigner"] - kw) < 1e-9, row["kappa"])
    check("k Wigner = kappa k", abs(row["k"]["wigner"] / (kw * k0) - 1) < 1e-5)
    check("Eckart kappa between Wigner and 2x Wigner-1", 1.0 < row["kappa"]["eckart"] < 1 + 2 * (kw - 1) + 0.05,
          row["kappa"])
    check("half-life = ln2/k", abs(row["half_life"]["none"] * row["k"]["none"] - math.log(2)) < 1e-9)
    dgr = thermo_dG("sn2_reactant -> sn2_product", "1M", T)
    check("dG_rxn = ecce-thermo (%.3f kJ/mol)" % dgr, abs(row["dG_rxn"] - dgr) < 2e-4, row["dG_rxn"])
    check("K_eq = exp(-dG_rxn/RT)", abs(row["K_eq"] / math.exp(-dgr / RT) - 1) < 1e-4)
    check("k/k_rev = K_eq (same molecularity)",
          abs(row["k"]["none"] / row["k_reverse"]["none"] / row["K_eq"] - 1) < 1e-4)
    check("reverse dG‡ = dG‡ - dG_rxn", abs(row["dG_act_reverse"] - (row["dG_act"] - row["dG_rxn"])) < 1e-9)
    check("imaginary frequency %.1f cm-1" % nu, abs(j["imaginary_frequency_cm-1"] - nu) < 1e-6)
    check("barriers positive", j["zpe_barrier_forward_kJ_per_mol"] > 0 and j["zpe_barrier_reverse_kJ_per_mol"] > 0)

    # Temperature range and fits.
    r = rate(REACT, TS, PROD, "--T", "250:350:25", "--json")
    j = json.loads(r.stdout)
    check("range gives 5 temperatures", [t["T"] for t in j["temperatures"]] == [250, 275, 300, 325, 350])
    ks = [t["k"]["none"] for t in j["temperatures"]]
    A, Ea = R.arrhenius_fit([250, 275, 300, 325, 350], ks)
    check("Arrhenius fit in the CLI = library fit", abs(j["fits"]["none"]["Ea_kJ_per_mol"] - Ea) < 1e-6)
    check("Eyring dH‡ near the mean dH‡",
          abs(j["fits"]["none"]["dH_act_kJ_per_mol"] - sum(t["dH_act"] for t in j["temperatures"]) / 5) < 1.5)

    # Bimolecular, 1 atm: two copies of the reactant complex give nonsense energies;
    # only the molecularity, unit and dG‡ bookkeeping are checked (k is in the unit tests).
    both = REACT + "," + REACT
    r = rate(both, TS, "--standard-state", "1atm", "--tunnelling", "none", "--json")
    j = json.loads(r.stdout)
    check("bimolecular unit", j["unit"] == "cm³ molecule⁻¹ s⁻¹", j["unit"])
    dg2 = thermo_dG("2*sn2_reactant -> sn2_qst3", "1atm", T)
    check("bimolecular dG‡ = ecce-thermo 2*A -> TS (1 atm)",
          abs(j["temperatures"][0]["dG_act"] - dg2) < 2e-3 * max(1, abs(dg2) * 1e-3), dg2)
    check("no product: Eckart symmetric note", any("symmetric" in n for n in j["notes"]))

    # Refusals and warnings.
    r = rate(REACT, REACT)
    check("a minimum as TS is refused", r.returncode == 1 and "imaginary" in r.stderr, r.stderr)
    r = rate(REACT, TS, TS)
    check("a TS as product warns", r.returncode == 0 and "warning" in r.stdout, r.stdout[:300])
    r = rate(REACT, TS, PROD)
    check("text output has the formulas' units", "k in s⁻¹" in r.stdout and "Eckart" in r.stdout)
    r = rate(REACT, TS, "--T", "abc")
    check("bad --T refused", r.returncode == 1)
finally:
    shutil.rmtree(TMP, ignore_errors=True)

print("FAILED: %s" % ", ".join(fails) if fails else "all passed")
sys.exit(1 if fails else 0)
