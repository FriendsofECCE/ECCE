#!/usr/bin/env python3
"""ecce-series (#621): one template, several structures, one calculation each.

An ORCA template and three xyz files (neutral, charged, triplet) give three
calculations with the template's theory and basis, each with its own
geometry (to 1e-6 A), charge and multiplicity and a deck that names them.
The same for a Gaussian-16 and an ECCE-QM template with a methane, whose
carbon the template's basis set was never assigned: the basis is rebuilt
from the template's basis name.  The data folder is a temporary one; no
GUI.

    tests/series/series_test.py [--home DIR] [--wrappers DIR]

Exit status 77 (CTest SKIP) without an install.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
FIX = os.path.join(REPO, "tests", "apps", "fixtures")
PROJECT_META = os.path.join(REPO, "tools", "screenshots", "data", "project.ecce-meta")

failures = []


def check(ok, what):
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


WATER = ("O  0.000000  0.000000  0.117300\nH  0.000000  0.757200 -0.469200\n"
         "H  0.000000 -0.757200 -0.469200\n")
OH = "O 0.000000 0.000000 0.000000\nH 0.000000 0.000000 0.970000\n"
O2 = "O 0.000000 0.000000 0.000000\nO 0.000000 0.000000 1.207000\n"
CH4 = ("C 0.000000 0.000000 0.000000\nH 0.629118 0.629118 0.629118\n"
       "H -0.629118 -0.629118 0.629118\nH -0.629118 0.629118 -0.629118\n"
       "H 0.629118 -0.629118 -0.629118\n")
SETS = {
    "orca": (os.path.join(FIX, "calc-orca-water-ccpvdz"), ".orcain",
             [("w1", WATER, "", 0, 1), ("ohm", OH, "charge=-1 mult=1", -1, 1),
              ("o2t", O2, "mult=3", 0, 3)]),
    "g16": (os.path.join(FIX, "calc-polar-h2o-g16"), ".g16in",
            [("w1", WATER, "", 0, 1), ("ch4", CH4, "", 0, 1),
             ("o2t", O2, "mult=3", 0, 3)]),
    "ecceqm": (os.path.join(REPO, "tests", "ecceqm", "fixtures", "h2o-geometry"),
               ".qmin", [("w1", WATER, "", 0, 1), ("ch4", CH4, "", 0, 1),
                         ("ohm", OH, "charge=-1", -1, 1)]),
}


def atoms(text):
    out = []
    for line in text.splitlines():
        p = line.split()
        if len(p) == 4 and re.match(r"^[A-Z][a-z]?$", p[0]):
            try:
                out.append((p[0], [float(x) for x in p[1:]]))
            except ValueError:
                pass
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--home", default=os.environ.get("ECCE_TEST_HOME",
                    os.path.join(REPO, ".inst", "ecce")))
    ap.add_argument("--wrappers", default=os.environ.get("ECCE_TEST_WRAPPERS",
                    os.path.join(REPO, ".inst", "bin")))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    exe = os.path.join(args.wrappers, "ecce-series")
    if not os.access(exe, os.X_OK):
        print("SKIP: %s is not installed" % exe)
        return 77
    work = tempfile.mkdtemp(prefix="ecce-series-")
    try:
        home = os.path.join(work, "home")
        data = os.path.join(work, "data")
        project = os.path.join(data, "users", "local", "p")
        os.makedirs(home)
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        env = dict(os.environ, HOME=home, ECCE_REALUSERHOME=home, ECCE_LOCAL_DATA=data,
                   ECCE_HOME=args.home)
        env.pop("ECCE_REMOTE_SERVER", None)
        env.pop("ECCE_SESSION_ID", None)

        for code, (source, ext, cases) in SETS.items():
            print(code)
            tpl = os.path.join(project, "tpl-" + code)
            shutil.copytree(source, tpl, symlinks=True)
            # git does not keep the empty Inputs folder of a stored calculation
            os.makedirs(os.path.join(tpl, "Inputs"), exist_ok=True)
            folder = os.path.join(project, "series-" + code)
            os.makedirs(folder)
            shutil.copy(PROJECT_META, os.path.join(folder, ".ecce-meta"))
            files = []
            for name, body, comment, _, _ in cases:
                path = os.path.join(work, code + "-" + name + ".xyz")
                with open(path, "w") as h:
                    h.write("%d\n%s\n%s" % (len(atoms(body)), comment, body))
                files.append(path)
            r = subprocess.run([exe, tpl, folder] + files, env=env,
                               capture_output=True, text=True, timeout=300)
            print(r.stdout + r.stderr)
            check(r.returncode == 0, "%s: ecce-series exits 0 (%d)" % (code, r.returncode))
            tdeck = ""
            tin = os.path.join(tpl, "Inputs")
            if os.path.isdir(tin):
                tdeck = " ".join(open(os.path.join(tin, f), errors="replace").read()
                                 for f in os.listdir(tin))
            for name, body, _, charge, mult in cases:
                calcname = code + "-" + name
                calc = os.path.join(folder, calcname)
                check(os.path.isdir(calc), "%s: the calculation exists" % calcname)
                meta = open(os.path.join(calc, ".ecce-meta"), errors="replace").read() \
                    if os.path.isdir(calc) else ""
                check(re.search(r"ecce:state\t\tReady", meta) is not None,
                      "%s: Ready" % calcname)
                inp = os.path.join(calc, "Inputs")
                decks = [f for f in os.listdir(inp) if f.endswith(ext)] \
                    if os.path.isdir(inp) else []
                deck = open(os.path.join(inp, decks[0]), errors="replace").read() \
                    if decks else ""
                check(bool(deck), "%s: a deck was generated" % calcname)
                # only some codes write the calculation's name into the deck
                if code == "g16":
                    check(calcname in deck, "%s: the deck names the calculation" % calcname)
                got, want = atoms(deck), atoms(body)
                check(len(got) == len(want) and all(
                    g[0] == w[0] and all(abs(a - b) < 1e-6 for a, b in zip(g[1], w[1]))
                    for g, w in zip(got, want)),
                    "%s: the deck's geometry is the xyz file's to 1e-6 A" % calcname)
                # charge and multiplicity as each code writes them
                if code == "orca":
                    check(re.search(r"\*\s*xyz\s+%d\s+%d\b" % (charge, mult), deck) is not None,
                          "%s: charge %d, multiplicity %d in the deck" % (calcname, charge, mult))
                elif code == "g16":
                    check(re.search(r"^\s*%d\s+%d\s*(!.*)?$" % (charge, mult), deck, re.M) is not None,
                          "%s: charge %d, multiplicity %d in the deck" % (calcname, charge, mult))
                else:
                    check(re.search(r"charge\s+%d\b" % charge, deck, re.I) is not None or
                          charge == 0, "%s: charge %d in the deck" % (calcname, charge))
            # the template's theory and basis: the deck's method line is the template's
            if tdeck:
                words = [w for w in re.findall(r"[A-Za-z0-9()+*/-]{4,}", tdeck)
                         if re.search(r"(?i)b3lyp|hf|dz|def2|pvdz|svp|631|6-31|pbe", w)]
                for name, body, _, _, _ in cases:
                    inp = os.path.join(folder, code + "-" + name, "Inputs")
                    text = " ".join(open(os.path.join(inp, f), errors="replace").read()
                                    for f in os.listdir(inp)) if os.path.isdir(inp) else ""
                    check(all(w in text for w in set(words)),
                          "%s-%s: the template's method and basis words %s are in the deck"
                          % (code, name, sorted(set(words))[:4]))
        # a multiplicity that does not fit is refused, nothing made
        bad = os.path.join(work, "bad.xyz")
        open(bad, "w").write("2\nmult=2\nO 0 0 0\nO 0 0 1.2\n")
        r = subprocess.run([exe, os.path.join(project, "tpl-orca"),
                            os.path.join(project, "series-orca"), bad], env=env,
                           capture_output=True, text=True, timeout=300)
        check(r.returncode == 1 and "not made" in r.stdout and
              not os.path.exists(os.path.join(project, "series-orca", "bad")),
              "a doublet O2 is refused")
    finally:
        if not args.keep:
            shutil.rmtree(work, ignore_errors=True)
    print("FAIL: %d" % len(failures) if failures else "PASS")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
