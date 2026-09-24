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
import re
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(REPO, "scripts", "parsers")
DATA = os.path.join(REPO, "data", "admin", "basissets")
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


def fortran_d_notation():
    """No basis data file may use Fortran D exponent notation.

    EDSIGaussianBasisSetLibrary.C reads these values with strtod(), which
    stops at the "D" and returns the mantissa alone: 0.30612488044D-01 is
    read as 0.30612488044, ten times too large. Perl's numeric conversion,
    used by rdStandardGBS.pm and the wr*GBS.pm writers, does the same.

    11 files and 24045 values were affected when this was found -- among
    them STO-6G, WTBS and the cc-pV*Z files behind cc-pCVDZ and the
    aug-cc-pV*Z diffuse sets. Nothing reported an error; the basis was
    simply wrong.
    """
    import glob
    pattern = re.compile(r"[0-9]D[-+][0-9]")
    findings = []
    for path in sorted(glob.glob(os.path.join(DATA, "*.BAS"))
                       + glob.glob(os.path.join(DATA, "*.POT"))):
        hits = sum(1 for line in open(path, errors="replace")
                   if pattern.search(line))
        if hits:
            findings.append("%s: %d value(s) in Fortran D notation, which "
                            "strtod() reads without the exponent"
                            % (os.path.basename(path), hits))
    return findings


def alias_records_complete():
    """Every basis alias record needs name=, files= AND atoms=.

    gbsNameList() sorts alias->atoms[0] whenever any element is selected,
    and atoms[0] on an empty vector is out of bounds -- a record with no
    atoms= line crashes the Basis Set Tool as soon as its section is
    listed.  One was shipped: LANL2TZ+-ecp, whose atoms= line was simply
    absent, which took the tool down on clicking the ECP section.
    """
    #  Only the real alias files -- the same list EDSIGaussianBasisSetLibrary
    #  reads.  The directory also holds editor backups (ecp~) and
    #  maintenance files, which are stripped when the library is served.
    ALIAS = ("pople", "other_segmented", "correlation_consistent",
             "other_generally_contracted", "ECPOrbital", "DFTOrbital",
             "Charge", "Exchange", "diffuse", "polarization", "rydberg",
             "ecp")
    findings = []
    for name in ALIAS:
        path = os.path.join(DATA, name)
        if not os.path.isfile(path):
            continue
        record = None
        seen = {}
        for line in open(path, errors="replace"):
            line = line.rstrip("\n")
            if line.startswith("name="):
                if record and not seen.get("atoms"):
                    findings.append("%s: %r has no atoms= line" % (name, record))
                record = line.split("=", 1)[1].strip()
                seen = {}
            elif line.startswith("files="):
                seen["files"] = True
            elif line.startswith("atoms="):
                seen["atoms"] = True
        if record and not seen.get("atoms"):
            findings.append("%s: %r has no atoms= line" % (name, record))
    return findings


#  The AO ordering each code prints its MO coefficients in.  Verified by
#  RUNNING the code and reading its own output, never from a manual and
#  never by copying a sibling's block, because getting it wrong is
#  completely silent: the MO isosurface stays smooth and plausible, the
#  electron density stays molecule shaped, and only an integral over the
#  density (trace(P S), which must equal the electron count) shows it.
#
#  ORCA shipped with p declared as x,y,z, copied from Gaussian.  ORCA
#  actually prints "1pz 1px 1py".  Every ORCA p coefficient therefore
#  landed on the wrong Cartesian axis, and every electrostatic potential
#  map came out positive everywhere because the molecule was being drawn
#  with several units of net positive charge (trace(P S) = 14.69 against
#  18 electrons for methanol).
#
#  The tell was internal: ORCA's d and f rows already used its
#  m = 0, +1, -1, +2, -2 convention while p alone did not.  If a new
#  code's p ordering does not follow the same convention as its own d
#  ordering, that is the thing to check first.
MO_ORDERING = {
    #  code .edml stem: (l=1 components, how it was verified)
    "ORCA": (("z", "x", "y"),
             "ORCA 6.1.1 def2-SVP methanol, MOLECULAR ORBITALS block: "
             "0C 1s 2s 3s 1pz 1px 1py 2pz 2px 2py 1dz2 ..."),
    "Gaussian-16": (("x", "y", "z"),
                    "g16 HF/6-31G(d) 5D water, pop=full: "
                    "1S 2S 2PX 2PY 2PZ 3S 3PX 3PY 3PZ 4D 0 4D+1 ..."),
    "Gaussian-09": (("x", "y", "z"), "same printout format as Gaussian-16"),
    "Gaussian-03": (("x", "y", "z"), "same printout format as Gaussian-16"),
    "NWChem": (("x", "y", "z"),
               "NWChem DFT Final Molecular Orbital Analysis, cc-pVDZ water: "
               "Bfn 4 = '1 O px', Bfn 6 = '1 O pz', so a p shell runs x y z"),
    "MOPAC": (("x", "y", "z"),
              "MOPAC GRAPHF basis order, verified by numerical integration "
              "in tools/mopac/verify_slater_basis.py"),
}


def mo_ordering_matches_the_code():
    """Each code's spherical l=1 ordering, against what the code prints.

    Only the codes whose ordering has actually been checked against a
    real printout are listed; a code absent from MO_ORDERING is not
    silently passed, it is reported as unverified, so adding one forces
    the question to be answered rather than copied.
    """
    CAP = os.path.join(REPO, "data", "client", "cap")
    RETIRED = ("Gaussian-98", "GAMESS-UK", "Amica", "DirDyVTST", "MetaDyn",
               "Polyrate", "NWChemMD", "GROMACS", "MOLCAS",
               "QuantumESPRESSO")

    findings = []
    for entry in sorted(os.listdir(CAP)):
        if not entry.endswith(".edml"):
            continue
        stem = entry[:-5]
        text = open(os.path.join(CAP, entry), errors="replace").read()

        block = re.search(r'<MOOrdering\s+type="spherical".*?</MOOrdering>',
                          text, re.S)
        if block is None:
            continue
        row = re.search(r'<lshell\s+lval="1"([^>]*)>', block.group(0))
        if row is None:
            findings.append("%s: spherical MOOrdering has no l=1 row" % stem)
            continue
        got = tuple(re.findall(r'a\d+="([^"]*)"', row.group(1)))

        if stem in RETIRED:
            continue
        if stem not in MO_ORDERING:
            findings.append(
                "%s declares a spherical MOOrdering but its l=1 order has "
                "never been checked against the code's own MO printout. "
                "Run the code, read the labels, and add it to MO_ORDERING "
                "with the evidence." % stem)
            continue

        want, evidence = MO_ORDERING[stem]
        if got != want:
            findings.append(
                "%s: spherical l=1 order is %s, the code prints %s (%s)"
                % (stem, " ".join(got), " ".join(want), evidence))
    return findings


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

    checks += 1
    for finding in fortran_d_notation():
        failures.append("basis data: " + finding)

    checks += 1
    for finding in alias_records_complete():
        failures.append("basis alias: " + finding)

    checks += 1
    for finding in mo_ordering_matches_the_code():
        failures.append("MO ordering: " + finding)

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
