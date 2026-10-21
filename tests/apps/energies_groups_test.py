#!/usr/bin/env python3
"""The Energies panel groups its rows (#283), headless, with the tree's own
binaries.

    tests/apps/energies_groups_test.py [--build build-cmake] [--png DIR] [--baseline]

Three calculations: calc-water-freq-g16 (Gaussian-16 opt+freq of water, the
scalars of tests/parsers/fixtures/gaussian-16/h2o_optfreq.log),
calc-water-smd-g16 (the same with SMD(water): hand-assembled from
h2o_smd.log, with the pressure and the solvation model) and the ECCE-QM
water GeoVib (tests/ecceqm/fixtures/h2o-geovib).  For each the
Builder opens the Energies panel and reports its rows through the script
commands "rows" and "fold" (no synthetic input):

  * headings in the agreed order, empty groups absent;
  * Components starts folded with its row count, the other groups open;
  * Thermochemistry carries the temperature and the pressure, which are not
    rows of their own, and Solvation the model and solvent;
  * "Entropy Term (T·S)", and Total Energy first in Total;
  * no row's value or unit reaches past the panel's right edge;
  * the fold state is remembered: folded, opened, and again after a restart.

--baseline only photographs (a build from before the change has no "rows").
Exit status 77 (skip) without Xvfb, import, python3-pil/numpy or binaries.
"""

import argparse
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
CASES = (("calc-water-freq-g16", os.path.join(HERE, "fixtures")),
         ("calc-water-smd-g16", os.path.join(HERE, "fixtures")),
         ("h2o-geovib", os.path.join(REPO, "tests", "ecceqm", "fixtures")))


def install(s, name, source):
    data = s.localData()
    project = os.path.join(data, "users", "local", "qm")
    os.makedirs(project, exist_ok=True)
    shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
    calc = os.path.join(project, name)
    shutil.rmtree(calc, ignore_errors=True)
    shutil.copytree(os.path.join(source, name), calc, symlinks=True)
    return "file://" + calc + "/"


def rowSets(text):
    """The "rows" reports of a Builder log: a list of row lists."""
    sets = []
    for line in text.splitlines():
        if line.startswith("BUILDER: rows"):
            sets.append([])
        elif line.startswith("BUILDER: row ") and sets:
            sets[-1].append(line[len("BUILDER: row "):])
    return sets


def fitLines(text):
    """The "fit" lines of a Builder log: (key, value right, unit right, width)."""
    out = []
    for line in text.splitlines():
        if line.startswith("BUILDER: fit "):
            key, v, u, w = [x.strip() for x in line[len("BUILDER: fit "):].split(" | ")]
            out.append((key, int(v), int(u), int(w)))
    return out


def headings(rows):
    return [r[2:] for r in rows if r.startswith("H ")]


def run(s, display, env, wrappers, png, name, url, baseline, restart=False):
    """First run: start, open Components, fold Total.  Restart: what was
    remembered, then the defaults put back (the settings file is shared)."""
    lines = ["wait 4000"]
    if not baseline:
        lines += ["rows Energies"]
    if not restart:
        lines += ["shot " + name + "-start"]
        if not baseline:
            lines += ["fold Energies Components", "wait 500", "rows Energies",
                      "shot " + name + "-components-open", "fold Energies Total",
                      "wait 500", "rows Energies"]
    else:
        lines += ["fold Energies Components", "fold Energies Total", "wait 500",
                  "rows Energies"]
    lines += ["quit"]
    return bo.runScript(s, display, env, wrappers, ["-context", url], lines, png,
                        name + ("-restart" if restart else ""),
                        {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Energies"})


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--baseline", action="store_true")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    try:
        import numpy  # noqa: F401
        from PIL import Image  # noqa: F401
    except ImportError:
        harness.skip("python3-pil and python3-numpy are needed")
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    for name, source in CASES:
        if not os.path.isdir(os.path.join(source, name)):
            harness.skip("fixture %s is missing" % name)
    import xdisplay

    s = harness.Session(build, "energiesgroups", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    say("screenshots: %s" % png)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    say("binaries: %s" % os.path.realpath(os.path.join(s.home, "bin", "builder")))

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        say("X display %s" % display.name)
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        for name, source in CASES:
            say("Energies panel, %s" % name)
            url = install(s, name, source)
            text, shots = run(s, display, env, wrappers, png, name, url, args.baseline)
            check(name + "-start" in shots, "the window was photographed")
            check("crash" not in text.lower() and "segmentation" not in text.lower(),
                  "no crash in the Builder log")
            if args.baseline:
                continue
            sets = rowSets(text)
            if not check(len(sets) == 3, "three row reports (got %d)" % len(sets)):
                say(text[-3000:])
                continue
            first, opened, total_folded = sets
            fits = fitLines(text)
            check(len(fits) > 0, "the rows report their positions")
            sel = [l.split(" | ")[-1] for l in text.splitlines()
                   if l.startswith("BUILDER: selectable ")]
            check(sel and all(v.strip() == "1" for v in sel),
                  "every value can be selected with the mouse (#408): %s" % sel)
            #  one label per row holds name, value and unit together
            labels = [l.split(" | ") for l in text.splitlines()
                      if l.startswith("BUILDER: selectable ")]
            check(labels and all(len(x) >= 3 and x[0].split("BUILDER: selectable ")[1][:12] in x[1]
                                 for x in labels),
                  "each row is one label that carries its name: %s" %
                  [x for x in labels if not (len(x) >= 3 and x[0].split("BUILDER: selectable ")[1][:12] in x[1])][:3])
            clipped = [f[0] for f in fits if f[2] > f[3] or f[1] > f[3]]
            check(not clipped, "every value and unit lies inside the panel "
                  "(panel %s px): %s" % (fits[0][3] if fits else "?", clipped))
            # Copy all (#408): one tab-separated line per value row shown
            copied = [l[len("BUILDER: copy "):] for l in text.splitlines()
                      if l.startswith("BUILDER: copy ")]
            shown = [r[2:].replace(" | ", " \\t ") for r in first if r.startswith("R ")]
            check(copied[:len(shown)] == shown,
                  "Copy all holds the rows shown, tab-separated: %s" % copied[:3])
            say("rows at start:\n  " + "\n  ".join(first))
            heads = headings(first)
            check(heads[0] == "▾ Total", "Total is first and open: %s" % heads)
            check(any(h.startswith("▸ Components (") for h in heads),
                  "Components starts folded with its count: %s" % heads)
            check(not any(r.startswith("R ") and "Nuclear" in r for r in first),
                  "no component rows while folded")
            check(any(r.startswith("R Total Energy") for r in first[:3]),
                  "Total Energy is the first row")
            thermo = [h for h in heads if "Thermochemistry" in h]
            check(len(thermo) == 1, "one Thermochemistry heading")
            conditions = {"calc-water-freq-g16": "(298.15 K)",
                          "calc-water-smd-g16": "(298.15 K, 1 atm)",
                          "h2o-geovib": "(298.15 K, 1 atm)"}[name]
            check(thermo and conditions in thermo[0],
                  "the conditions are in the heading, %s: %s" % (conditions, thermo))
            check(not any("Thermochemistry Pressure" in r or "Solvation Model" in r
                          for r in first),
                  "the pressure and the solvent model are not rows")
            check(not any("Thermochemistry Temperature" in r for r in first),
                  "the temperature is not a row")
            check(any(r.startswith("R Entropy Term (T·S)") for r in first),
                  "Entropy Term (T·S)")
            check(not any("T*S" in r for r in first), "no T*S spelling left")
            order = ["Total", "Components", "Thermochemistry", "Solvation", "Derived"]
            names = [h[2:].split(" (")[0] for h in heads]
            check(names == sorted(names, key=order.index),
                  "groups in the agreed order: %s" % names)
            check(all(g in names for g in ("Total", "Components", "Thermochemistry")),
                  "the groups this calculation has are present")
            solv = [h for h in heads if "Solvation" in h]
            if "smd" in name:
                check(solv and solv[0].endswith("Solvation (SMD, Water)"),
                      "the solvation model and solvent are in the heading: %s" % solv)
                check(any(r.startswith("R SMD Cavity-Dispersion") for r in first),
                      "the CDS term is a row")
            else:
                check("Solvation" not in names, "empty groups are not shown")
            check("Derived" not in names, "empty groups are not shown")
            check(any(r.startswith("R Nuclear Repulsion") for r in opened),
                  "opening Components shows its rows")
            check(any(h.startswith("▸ Total") for h in headings(total_folded)),
                  "Total can be folded too")
            text2, _ = run(s, display, env, wrappers, png, name, url, False, True)
            again = rowSets(text2)
            if check(len(again) == 2, "two row reports after the restart"):
                check(headings(again[0]) == headings(total_folded),
                      "the fold state is remembered per user: %s" % headings(again[0]))
                check(headings(again[1]) == headings(first),
                      "folding again restores the defaults")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
