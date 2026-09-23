#!/usr/bin/env python3
"""
ECCE basis-set export regression suite.

Runs the real ``scripts/parsers/std2<Code>`` exporters over checked-in
``.gbs`` inputs -- the format ``TGBSConfig::dump()`` produces -- and checks
what each code's writer emits.

Needs nothing but Python 3 and perl: no GUI, no ECCE services, no chemistry
codes.

    ./run_tests.py            run everything
    ./run_tests.py -v         show each exporter's full output
    ./run_tests.py --update   regenerate the golden files in expected/

Why this exists
---------------
Nothing exercised these writers, and they decide whether a calculation asks
the code for a basis BY NAME or writes out every exponent and coefficient.
Every bug found in them on 2026-09-23 was silent -- a deck that Gaussian
rejects, or one that quietly uses a different basis than the user chose:

  * the Gaussian writer printed ECCE's own basis name rather than the
    translated one, so "midi!", "dz (dunning)" and "sv (dunning-hay)"
    produced decks Gaussian rejects outright;
  * Gaussian's table mapped 6-31G(3df,3pd) to "...3dp", a fatal syntax
    error, which went unnoticed because the value was never printed;
  * both writers fell back to explicit primitives for EVERY element as
    soon as one element's basis was unmappable.

Exit status is 0 only if every check passed.
"""

import argparse
import difflib
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(REPO, "scripts", "parsers")
FIXTURES = os.path.join(HERE, "fixtures")
EXPECTED = os.path.join(HERE, "expected")

#  (name, fixture, exporter, [assertions])
#  Each assertion is (description, predicate over the emitted text).
CASES = [
    dict(
        name="g16-single-named",
        fixture="water_same.gbs",
        exporter="std2Gaussian-16",
        #  Every atom shares one mappable basis, so the whole molecule goes
        #  on the route card and no primitives are written at all.
        expect=[("uses the route card", lambda t: "useRouteCard" in t),
                ("emits the TRANSLATED name, not ECCE's",
                 lambda t: "useRouteCard 6-31G*" in t),
                ("writes no primitives", lambda t: " S " not in t)],
    ),
    dict(
        name="orca-single-named",
        fixture="water_same.gbs",
        exporter="std2ORCA",
        expect=[("uses the route card", lambda t: "useRouteCard 6-31G*" in t),
                ("writes no %basis block", lambda t: "%basis" not in t)],
    ),
    dict(
        name="g16-mixed-per-element",
        fixture="water_mixed.gbs",
        exporter="std2Gaussian-16",
        #  O's basis is mappable and H's is not. Before per-element naming
        #  BOTH were written out in full.
        expect=[("no route card", lambda t: "useRouteCard" not in t),
                ("O named", lambda t: "6-31G*" in t),
                ("H explicit", lambda t: "18.73113700" in t)],
    ),
    dict(
        name="orca-mixed-per-element",
        fixture="water_mixed.gbs",
        exporter="std2ORCA",
        expect=[("no route card", lambda t: "useRouteCard" not in t),
                ("O named via NewGTO",
                 lambda t: 'NewGTO O "6-31G*" end' in t),
                ("H explicit", lambda t: "18.73113700" in t)],
    ),
    dict(
        name="g16-ecp-per-element",
        fixture="pth_ecp.gbs",
        exporter="std2Gaussian-16",
        #  Pt carries an ECP so keeps explicit primitives; H does not and is
        #  still named. Naming is decided per element, not per molecule.
        expect=[("H named", lambda t: "6-31G*" in t),
                ("Pt explicit", lambda t: "2.63000000" in t),
                ("ECP still written", lambda t: "PT-ECP" in t)],
    ),
    dict(
        name="orca-ecp-per-element",
        fixture="pth_ecp.gbs",
        exporter="std2ORCA",
        expect=[("H named", lambda t: 'NewGTO H "6-31G*" end' in t),
                ("Pt explicit", lambda t: "2.63000000" in t),
                ("ECP still written", lambda t: "NewECP Pt" in t)],
    ),
    dict(
        name="orca-no-names-all-explicit",
        fixture="water_numonly.gbs",
        exporter="std2ORCA",
        #  No NameBasis section at all: nothing may be named.
        expect=[("no route card", lambda t: "useRouteCard" not in t),
                ("no named NewGTO", lambda t: '" end' not in t),
                ("primitives written", lambda t: "18.73113700" in t)],
    ),
]


def run(case):
    path = os.path.join(PARSERS, case["exporter"])
    with open(os.path.join(FIXTURES, case["fixture"])) as handle:
        proc = subprocess.run(["perl", path], stdin=handle,
                              capture_output=True, text=True, timeout=60,
                              env=dict(os.environ, ECCE_HOME=REPO))
    return proc.stdout, proc.returncode, proc.stderr


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--update", action="store_true")
    args = ap.parse_args()

    os.makedirs(EXPECTED, exist_ok=True)
    failures, checks = [], 0

    for case in CASES:
        text, rc, err = run(case)
        if rc != 0:
            failures.append("%s: exporter exited %d\n%s"
                            % (case["name"], rc, err.strip()[:400]))
            continue
        if args.verbose:
            print("--- %s ---\n%s" % (case["name"], text))

        for description, predicate in case["expect"]:
            checks += 1
            if not predicate(text):
                failures.append("%s: %s\n--- emitted ---\n%s"
                                % (case["name"], description, text))

        golden = os.path.join(EXPECTED, case["name"] + ".txt")
        if args.update:
            with open(golden, "w") as handle:
                handle.write(text)
            continue
        checks += 1
        if not os.path.exists(golden):
            failures.append("%s: no golden file -- run with --update and "
                            "review the result before committing it"
                            % case["name"])
        else:
            want = open(golden).read()
            if want != text:
                diff = "".join(difflib.unified_diff(
                    want.splitlines(True), text.splitlines(True),
                    "expected", "emitted"))
                failures.append("%s: output drifted\n%s" % (case["name"], diff))

    if args.update:
        print("golden files regenerated -- review the diff before committing")
        return 0

    print("\n%d checks run" % checks)
    if failures:
        print("\nFAILURES (%d):" % len(failures))
        for f in failures:
            print("  * %s\n" % f)
        print("FAILED")
        return 1
    print("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
