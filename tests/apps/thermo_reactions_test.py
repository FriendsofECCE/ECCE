#!/usr/bin/env python3
"""ecce-thermo --reactions (#709), headless, with the tree's binaries.

    tests/apps/thermo_reactions_test.py [--build build-cmake]

Three water frequency calculations with constructed low frequencies: two
ORCA ones and one labelled Gaussian.  A reactions file has two reactions
between the ORCA calculations and one that mixes in the Gaussian one.  The
JSON must give: dE_el from the stored TE; dG and the spread equal to
`ecce-thermo --reaction`; the third reaction refused, naming the method
difference; and, under plain RRHO, soft-mode entropies that add up to an
independent sum of the harmonic entropy of the modes below 100 cm-1.  The CSV
and the text carry the same numbers.  Exit status 77 (skip) without binaries.
"""

import argparse
import csv
import io
import json
import math
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import harness  # noqa: E402
from harness import say  # noqa: E402
import thermo_panel_test as tp  # noqa: E402

failures = []
KJ = 4.3597447222071e-18 * 6.02214076e23 / 1000.0
HC_K = 6.62607015e-34 * 2.99792458e10 / 1.380649e-23
R = 8.314462618


def check(ok, what, detail=""):
    say("%s %s" % ("ok   " if ok else "FAIL ", what))
    if not ok:
        if detail:
            say("      " + detail)
        failures.append(what)
    return ok


def sHo(nu, T):
    x = HC_K * nu / T
    return R * (x / (math.exp(x) - 1.0) - math.log(1.0 - math.exp(-x)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "ecce-thermo"), os.X_OK):
        harness.skip("ecce-thermo is not built in %s" % build)
    s = harness.Session(build, "thermorxn", {}, local=True, keep=args.keep)
    harness.link(os.path.join(build, "ecce-thermo"), os.path.join(s.home, "bin", "ecce-thermo"))
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        env = dict(s.env(), ECCE_HOME=s.home)
        project = os.path.join(s.localData(), "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil_copy = __import__("shutil").copy
        import builder_overlay_test as bo
        shutil_copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        f1 = [25.0, 60.0, 150.0, 1645.0, 3866.0, 3965.0]
        f2 = [40.0, 90.0, 300.0, 1600.0, 3800.0, 3900.0]
        calcs = {"one": tp.makeThermoCalc(project, "one", f1),
                 "two": tp.makeThermoCalc(project, "two", f2),
                 "gau": tp.makeThermoCalc(project, "gau", f1, code="NWChem")}
        te2 = -75.0
        with open(os.path.join(calcs["two"], "Props", "TE"), "w") as f:
            f.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                    '<value name="TE" units="Hartree">%.10f</value>\n' % te2)
        te1 = float(re.search(r">(\S+)<", open(os.path.join(calcs["one"], "Props", "TE")).read()).group(1))
        urls = ["file://" + c for c in calcs.values()]
        rx = os.path.join(s.state, "reactions.txt")
        open(rx, "w").write("# comment\none -> two\n2*one -> two, two\none -> gau\n")

        def run(*extra):
            return subprocess.run([os.path.join(s.home, "bin", "ecce-thermo"),
                                   "--reactions", rx] + urls + list(extra),
                                  env=env, capture_output=True, text=True).stdout

        data = json.loads(run("--format", "json"))
        r = data["reactions"]
        check(len(r) == 3, "three reactions in the output", str([x["reaction"] for x in r]))
        a, b, c = r
        check(abs(a["dE_el_Eh"] - (te2 - te1)) < 2e-6, "dE_el = TE(two) - TE(one)", str(a["dE_el_Eh"]))
        check(abs(b["dE_el_Eh"] - (2 * te2 - 2 * te1)) < 2e-6, "coefficients applied to dE_el")
        refText = (subprocess.run(
            [os.path.join(s.home, "bin", "ecce-thermo"), "--reaction", "one -> two", "--json"]
            + urls, env=env, capture_output=True, text=True).stdout)
        ref = json.loads(refText[refText.index("{"):])   # notes come first
        check(all(abs(a["deltaG"][t] - v) < 2e-6 for t, v in ref["deltaG"].items()),
              "dG per treatment equals --reaction", str((a["deltaG"], ref["deltaG"])))
        check(abs(a["dG_Eh"] - ref["deltaG"]["qrrho-grimme:100"]) < 2e-6,
              "the chosen dG is the Grimme 100 value")
        check(abs(a["spread_kJ"] - ref["spread_kJ_per_mol"]) < 2e-3,
              "the spread equals --reaction's", "%s %s" % (a["spread_kJ"], ref["spread_kJ_per_mol"]))
        check(a["spread_kJ"] > 0.0, "the spread is not zero for soft modes")
        check(c["status"] == "refused" and "method" in c["reason"],
              "a reaction across codes is refused, with the reason", c.get("reason", ""))
        check("dG_kJ" not in c, "a refused reaction has no energies")

        # Soft modes under RRHO against an independent harmonic-oscillator sum.
        rr = json.loads(run("--format", "json", "--chosen", "rrho"))["reactions"][0]
        for sp, freqs in zip(rr["species"], (f1, f2)):
            T = 298.15
            want = sum(sHo(f, T) for f in freqs if f < 100.0)
            got = sum(m["S"] for m in sp["soft_modes"])
            check(abs(got - want) < 1e-3 and abs(sp["S_soft_J_mol_K"] - want) < 1e-3,
                  "%s: soft-mode S adds up to the low-frequency S" % sp["name"], "%s %s" % (got, want))
            tot = sum(m["S_share"] for m in sp["soft_modes"]) * sp["S_J_mol_K"]
            check(abs(tot - want) < 1e-2, "%s: the shares times S add up too" % sp["name"], str(tot))
            check(len(sp["soft_modes"]) == sum(1 for f in freqs if f < 100.0), "%s: soft modes flagged < 100 cm-1" % sp["name"])
        # Grimme: the shares still add up to the soft part of the chosen S.
        for sp in a["species"]:
            tot = sum(m["S_share"] for m in sp["soft_modes"]) * sp["S_J_mol_K"]
            check(abs(tot - sp["S_soft_J_mol_K"]) < 1e-2, "%s: Grimme shares add up" % sp["name"])

        text = run("--format", "csv")
        head = text.split("\n\n")[0]
        rows = list(csv.DictReader(io.StringIO(head)))
        check(len(rows) == 3 and abs(float(rows[0]["dG_kJ_mol"]) - a["dG_kJ"]) < 1e-3
              and rows[2]["status"] == "refused", "CSV carries the same numbers", head)
        txt = run()
        check("refused:" in txt and "soft modes" in txt and "spread" in txt, "text output", txt[:600])
    finally:
        s.stop()
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
