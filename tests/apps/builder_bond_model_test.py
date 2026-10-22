#!/usr/bin/env python3
"""Builder: no bond orders in building; Clean derives them from geometry (#439).

    tests/apps/builder_bond_model_test.py [--build DIR] [--keep] [--shots DIR]

Headless through ECCE_BUILDER_SCRIPT.  Ethene, ethyne, formaldehyde, CO2,
acetonitrile and ethane are built from atoms with a geometry each (no order
exists to choose), joined in Bond mode, given hydrogens and Cleaned; the
bond lengths must be those of the double/triple/single bond the rule
derived, and every bond is drawn as one stick.  Benzene is read from a file,
Cleaned, and must come out aromatic (about 1.39-1.40 A).  Ethane cannot get
a multiple bond, and Clean names what it assumed.

Exit status 77 (skip) without Xvfb or the built Builder.
"""
import argparse
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import builder_edit_bugs_test as ebt  # noqa: E402
import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = ebt.check

BOND = ["mode bond", "bondkind covalent"]


def join(a, b):
    return ["select -1", "selectelem " + a, "addelem " + b, "click"]


def build(atoms, bonds):
    lines = ["wait 3000", "select -1"]
    for i, (el, geom) in enumerate(atoms):
        lines += ["select -1", "add %s %s %d 0 0" % (el, geom, 8 * i)]
    lines += BOND
    for a, b in bonds:
        lines += join(a, b)
    return lines


def run(s, env, wrappers, name, lines, structure=None, shots=None):
    lines = lines + ["cmd addh", "wait 1000", "clean", "wait 3000",
                     "bondorders", "selection", "wait 500"]
    if shots:
        lines.append("shot " + name)
    lines.append("quit")
    return ebt.runBuilder(s, env, wrappers, name, lines, shots,
                          {"ECCE_TEST_DIALOG_CLOSE": "1",
                           "ECCE_TEST_DIALOG_ANSWER": "yes"},
                          structure=structure)


def lengths(text):
    """{(sym1, sym2): [(order, drawn, len)]} of the last bondorders dump."""
    out = {}
    for m in re.finditer(r"BUILDER: bond \d+-\d+ (\w+)-(\w+) order=(\S+) "
                         r"drawn=(\d+) len=(\S+)", text):
        out.setdefault(tuple(sorted(m.group(1, 2))), []).append(
            (float(m.group(3)), int(m.group(4)), float(m.group(5))))
    return out


CASES = [
    # name, atoms, bonds, (pair, order, length, tolerance)
    ("ethene", [("C", "TrigonalPlanar")] * 2, [("C", "C")],
     (("C", "C"), 2.0, 1.33, 0.06)),
    ("ethyne", [("C", "Linear")] * 2, [("C", "C")],
     (("C", "C"), 3.0, 1.20, 0.05)),
    ("formaldehyde", [("C", "TrigonalPlanar"), ("O", "Terminal")],
     [("C", "O")], (("C", "O"), 2.0, 1.21, 0.06)),
    ("co2", [("C", "Linear"), ("O", "Terminal"), ("O", "Terminal")],
     [("C", "O"), ("C", "O")], (("C", "O"), 2.0, 1.21, 0.07)),
    ("acetonitrile", [("C", "Tetrahedral"), ("C", "Linear"),
                      ("N", "Terminal")], [("C", "C"), ("N", "C")],
     (("C", "N"), 3.0, 1.16, 0.06)),
    ("ethane", [("C", "Tetrahedral")] * 2, [("C", "C")],
     (("C", "C"), 1.0, 1.53, 0.05)),
]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(ebt.REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--shots")
    args = ap.parse_args()
    build_dir = os.path.abspath(args.build)
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build_dir, ())
    if not os.access(os.path.join(build_dir, "builder"), os.X_OK):
        harness.skip("builder is not built in %s" % build_dir)
    import xdisplay

    s = harness.Session(build_dir, "bondmodel", {}, local=True,
                        keep=args.keep)
    for exe in bo.GUI:
        harness.link(os.path.join(build_dir, exe),
                     os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build_dir, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1366x768x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        shots = os.path.abspath(args.shots) if args.shots else None
        if shots:
            os.makedirs(shots, exist_ok=True)
        for name, atoms, bonds, (pair, order, length, tol) in CASES:
            text = run(s, env, wrappers, name, build(atoms, bonds),
                       shots=shots)
            got = lengths(text).get(pair, [])
            say("    %s: %s" % (name, got))
            if not check(bool(got), "%s: built, joined and Cleaned" % name):
                say("\n".join("      | " + l
                              for l in text.splitlines()[-25:]))
                continue
            o, drawn, d = got[0]
            check(o == order and abs(d - length) <= tol,
                  "%s: %s-%s order %g, %.3f A (want %.2f +- %.2f)"
                  % (name, pair[0], pair[1], o, d, length, tol))
            check(all(v[1] == 1 for vs in lengths(text).values()
                      for v in vs), "%s: every bond drawn single" % name)
            if name == "ethane":
                check("treated as multiple" not in text,
                      "ethane: no multiple bond can arise")
            else:
                check("Clean treated as multiple bonds" in text,
                      "%s: Clean says what it assumed" % name)
        text = run(s, env, wrappers, "benzene", ["wait 3000"],
                   structure=ebt.STRUCTURE, shots=shots)
        cc = [v for v in lengths(text).get(("C", "C"), [])]
        say("    benzene: %s" % cc[:1])
        check(len(cc) == 6 and all(v[0] == 1.5 and 1.36 <= v[2] <= 1.43
                                   and v[1] == 1 for v in cc),
              "benzene: six aromatic C-C bonds drawn single (%s)" % cc)
        # A metal-pi bond can still be made: Fe over benzene.
        text = ebt.runBuilder(
            s, env, wrappers, "metalpi",
            ["wait 3000", "select -1", "add Fe Tetrahedral 8 0 0",
             "bondkind metal", "mode bond", "selectelem Fe", "addelem C",
             "click", "wait 1000", "bondorders", "bondkind covalent",
             "quit"], None, {"ECCE_TEST_DIALOG_CLOSE": "1",
                             "ECCE_TEST_DIALOG_ANSWER": "yes"})
        fe = [v for k, vs in lengths(text).items() if "Fe" in k for v in vs]
        check(len(fe) >= 5 and all(v[0] == 0.5 for v in fe),
              "a metal-pi bond is still made (%d Fe-C bonds of order 0.5)"
              % len(fe))
        check(re.findall(r"bondkind current=(\S+)", text) == ["0.5", "1"],
              "metal-pi and covalent are the only Bond tool kinds")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    say("FAILED: " + "; ".join(ebt.failures) if ebt.failures else "PASSED")
    return 1 if ebt.failures else 0


if __name__ == "__main__":
    sys.exit(main())
