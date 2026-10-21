#!/usr/bin/env python3
"""The Vibrational Frequencies table with its Sym and Mode columns (#367), in
the real Builder, headless, with the tree's own binaries.

    tests/vibanalysis/panel_test.py [--build build-cmake] [--png DIR]

For H2O, CO2, NH3, CH4, HCHO and benzene (ECCE-QM runs, read by the real
parsers) a calculation folder is built, the Builder opens it on the table
(ECCE_NMODE_TABLE_DUMP), and what the table holds is compared with:

  * the textbook assignments (run_tests.py): the panel's analysis is the
    library's, on the calculation's own structure;
  * the panel's width: it must be the width water's table gets, and no
    column of any of the six may need more than the docked panel gives, so
    that every cell is shown whole and nothing widened the dock;
  * a code that prints its own mode symmetry (VIBSYM): its label stays in
    the Sym column.

No synthetic input; the Builder is driven by its hooks.  Exit 77 without
Xvfb or the built binaries.
"""
import argparse
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))
sys.path.insert(0, os.path.join(REPO, "tests", "spectrum"))
sys.path.insert(0, HERE)

import builder_overlay_test as bo  # noqa: E402
import extract  # noqa: E402
import harness  # noqa: E402
import makecalc  # noqa: E402
from harness import say  # noqa: E402

import importlib.util  # noqa: E402

#  Loaded by path: another tests/*/run_tests.py is already on sys.path.
_spec = importlib.util.spec_from_file_location(
    "vibanalysis_run_tests", os.path.join(HERE, "run_tests.py"))
rt = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(rt)

check = bo.check
RIGID = ("Tx", "Ty", "Tz", "Rx", "Ry", "Rz")
#  The helpers the symmetry analysis starts.
SYMMETRY = ("autosym", "cleansym", "symops", "genmol", "getfrag")
TEMPLATE = os.path.join(REPO, "tests", "ecceqm", "fixtures", "h2o-vibration")

#  name, fixture, desc, runtype, elements, bonds key
MOLECULES = [
    ("h2o", os.path.join(rt.FIX, "ecceqm", "h2o_optfreq.qmout"), "OHH", "H2O"),
    ("co2", os.path.join(rt.QM, "co2_optfreq.qmout"), "COO", "CO2"),
    ("nh3", os.path.join(rt.QM, "nh3_optfreq.qmout"), "NHHH", "NH3"),
    ("ch4", os.path.join(rt.QM, "ch4_optfreq.qmout"), "CHHHH", "CH4"),
    ("hcho", os.path.join(rt.QM, "hcho_optfreq.qmout"), "COHH", "HCHO"),
    ("c6h6", os.path.join(rt.QM, "c6h6_optfreq.qmout"), "CCCCCCHHHHHH", "BENZENE"),
]
#  Gaussian's own labels (VIBSYM) for water: shown as the code printed them.
RAMAN = ("h2o-raman", os.path.join(rt.FIX, "ecceqm", "h2o_raman.qmout"))
#  The detail label's width in px, narrower than any dock: forces the wrapping.
NARROW = "150"
CO = ("co-g16", os.path.join(rt.FIX, "gaussian-16", "co_freq.log"))
G16 = ("h2o-g16", os.path.join(rt.FIX, "gaussian-16", "h2o_optfreq.log"))


def mvm(elements, xyz, bonds):
    lines = ["title: vibanalysis", "type: molecule", "num_atoms: %d" % len(elements),
             "atom_info: symbol cart", "atom_list: "]
    for a, el in enumerate(elements):
        lines.append("%s %.6f %.6f %.6f" % ((el,) + tuple(xyz[3 * a:3 * a + 3])))
    lines.append("attr_list:")
    lines += [" 0.000000   1   0   0   0"] * len(elements)
    lines.append("atom_type_list:")
    lines += ["        1   1"] * len(elements)
    lines.append("num_bonds: %d" % len(bonds))
    lines.append("bond_list: ")
    for a, b, order in bonds:
        lines.append("%d %d %.5f 2" % (a + 1, b + 1, order))
    return "\n".join(lines) + "\n"


def guessBonds(elements, xyz):
    import math
    radius = {"H": 0.31, "C": 0.76, "N": 0.71, "O": 0.66}
    out = []
    for a in range(len(elements)):
        for b in range(a + 1, len(elements)):
            d = math.dist(xyz[3 * a:3 * a + 3], xyz[3 * b:3 * b + 3])
            if d < 1.2 * (radius[elements[a]] + radius[elements[b]]):
                out.append((a, b, 1.0))
    return out


def makeCalc(project, name, fixture, desc, runtype, elements, bondsKey):
    """A calculation folder: the water template with the structure and the
    vibrational properties of `fixture`, as the real parsers produce them."""
    calc = os.path.join(project, name)
    shutil.rmtree(calc, ignore_errors=True)
    shutil.copytree(TEMPLATE, calc, symlinks=True)
    found = extract.records(fixture, desc, runtype)
    geo = found["GEOMTRACE"][-1]
    xyz = extract.floats(geo["values"])
    if elements is None:
        elements = [lab.split("-")[1] for lab in geo["rowlabels"].split()]
    bonds = rt.BONDS.get(bondsKey) or guessBonds(elements, xyz)
    with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
        f.write(mvm(elements, xyz, bonds))
    props = os.path.join(calc, "Props")
    keep = ["VIB", "VIBFREQ", "VIBIR", "VIBRAM", "VIBSYM"]
    for entry in os.listdir(props):
        path = os.path.join(props, entry)
        if os.path.isfile(path) and not entry.startswith("."):
            os.unlink(path)
    meta = []
    for key in keep:
        if key not in found:
            continue
        flat = found[key][-1]
        if key == "VIB":
            text = makecalc.vectable(flat)
        elif key == "VIBSYM":
            text = makecalc.vecstring(key, flat)
        else:
            text = makecalc.vector(key, flat)
        with open(os.path.join(props, key), "w") as f:
            f.write(text)
        meta.append("%s\thttp://www.emsl.pnl.gov/ecce:mime-type\t\ttext/xml" % key)
    with open(os.path.join(props, ".ecce-meta"), "w") as f:
        f.write("\n".join(meta) + "\n")
    return found.get("VIBSYM", [{}])[-1].get("values", "").split()


def readDump(path):
    d = dict(cols={}, cells={}, modes={}, panel=None, grid=None, detail="", group="")
    for line in open(path, encoding="utf-8"):
        line = line.rstrip("\n")
        f = line.split()
        if not f:
            continue
        if f[0] == "panel":
            d["panel"] = (int(f[1]), int(f[2]), int(f[4]), int(f[5]))
        elif f[0] == "grid":
            d["grid"] = (int(f[1]), int(f[2]), int(f[4]), int(f[5]))
        elif f[0] == "group":
            d["group"] = f[1] if len(f) > 1 else ""
        elif f[0] == "col":
            d["cols"][int(f[1])] = (int(f[2]), " ".join(f[3:]))
        elif f[0] == "cell":
            text = line.split(" ", 5)[5] if len(line.split(" ", 5)) > 5 else ""
            d["cells"][(int(f[1]), int(f[2]))] = (int(f[3]) == 1, int(f[4]) == 1, text)
        elif f[0] == "mode":
            head, _, detail = line.partition(" | ")
            g = head.split(None, 11)
            d["modes"][int(g[1])] = dict(
                irrep=g[3], ir=int(g[5]), raman=int(g[7]), known=int(g[9]),
                label=g[11] if len(g) > 11 else "-", detail=detail)
        elif f[0] == "initial":
            d["initial"] = (int(f[1]), float(f[2]) if len(f) > 2 else None)
        elif f[0] == "axis" and len(f) == 3:
            d["range"] = (float(f[1]), float(f[2]))
        elif f[0] == "detail":
            d["detail"] = line[7:]
        elif f[0] == "detailshown":
            d["shown"] = line[12:]
    return d


def run(s, display, env, wrappers, png, url, name):
    dump = os.path.join(s.state, "nmode-%s.dump" % name)
    if os.path.exists(dump):
        os.unlink(dump)
    #  No exit after the dump: Copy all (#408) is read from the same run.
    lines = ["wait 15000", "tablecopy Vibrational Frequencies", "quit"]
    text, _shots = bo.runScript(
        s, display, env, wrappers, ["-context", url], lines, png, name,
        {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Vibrational Frequencies",
         "ECCE_NMODE_TABLE_DUMP": dump, "ECCE_NMODE_DETAIL_WIDTH": NARROW})
    if not check(os.path.exists(dump) and os.path.getsize(dump) > 0,
                 "%s: the table dumped what it holds" % name):
        say(text[-2000:])
        return None
    check("segmentation" not in text.lower(), "%s: no crash in the Builder log" % name)
    d = readDump(dump)
    copied = [l.split("BUILDER: tablecopy ", 1)[1] for l in text.splitlines()
              if "BUILDER: tablecopy " in l]
    rows = len({k[0] for k in d["cells"]}) if d else 0
    check(len(copied) == rows + 1 and rows > 0,
          "%s: Copy all gives the column labels and one line per row (%d for %d rows)"
          % (name, len(copied), rows))
    return d


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "vibtable", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI + SYMMETRY:
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
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        data = s.localData()
        project = os.path.join(data, "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))

        results = {}
        for name, fixture, elements, key in MOLECULES:
            makeCalc(project, name, fixture, "ecceqm.desc", "GeoVib", list(elements), key)
            say("table: %s" % name)
            results[name] = run(s, display, env, wrappers, png,
                                "file://" + os.path.join(project, name) + "/", name)
        codeSym = makeCalc(project, G16[0], G16[1], "gaussian-16.desc", "GeoVib", None, "H2O")
        say("table: %s" % G16[0])
        results[G16[0]] = run(s, display, env, wrappers, png,
                              "file://" + os.path.join(project, G16[0]) + "/", G16[0])
        makeCalc(project, CO[0], CO[1], "gaussian-16.desc", "GeoVib", None, "CO")
        say("table: %s" % CO[0])
        results[CO[0]] = run(s, display, env, wrappers, png,
                             "file://" + os.path.join(project, CO[0]) + "/", CO[0])
        makeCalc(project, RAMAN[0], RAMAN[1], "ecceqm.desc", "Vibration", list("OHH"), "H2O")
        say("table: %s" % RAMAN[0])
        results[RAMAN[0]] = run(s, display, env, wrappers, png,
                                "file://" + os.path.join(project, RAMAN[0]) + "/", RAMAN[0])
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()

    base = results.get("h2o")
    for name, key in [(m[0], m[3]) for m in MOLECULES]:
        d = results.get(name)
        if d is None:
            continue
        modes = [dict(n=i, freq=0.0, irrep=m["irrep"],
                      ir="IR" if m["ir"] else "-", raman="Raman" if m["raman"] else "-",
                      label=m["label"])
                 for i, m in sorted(d["modes"].items())
                 if m["irrep"] != "-" and m["label"] not in RIGID]
        rt.match_textbook("panel " + name, modes, rt.TEXTBOOK[key]) if key in rt.TEXTBOOK else None
        if key == "BENZENE":
            counts = {}
            for m in modes:
                counts[m["irrep"]] = counts.get(m["irrep"], 0) + 1
            check(counts == rt.BENZENE, "panel benzene: irreps span the textbook set (%s)" % counts)
        say("  %s: panel %s, grid %s, columns %s" % (
            name, d["panel"][:2], d["grid"][:2], [(v[1].split()[0], v[0]) for _, v in sorted(d["cols"].items())]))
        #  Every cell whole, the column header present, the dock unchanged.
        cut = [k for k, v in d["cells"].items() if not v[0] or v[1]]
        check(not cut, "%s: every cell is shown whole at the docked width (cut: %s)" % (name, cut[:5]))
        check(base is not None and d["panel"][:2] == base["panel"][:2],
              "%s: the panel is %s, water's %s: nothing widened the dock"
              % (name, d["panel"][:2], base and base["panel"][:2]))
        check(d["panel"][2] <= d["panel"][0],
              "%s: the panel's minimum width %d fits its width %d" % (name, d["panel"][2], d["panel"][0]))
        labels = [v[1] for _, v in sorted(d["cols"].items())]
        check(labels[:3] == ["Frequency cm-1", "Sym", "Mode"], "%s: columns %s" % (name, labels))
        check(d["detail"].strip() != "", "%s: the selected mode has a detail line: %s" % (name, d["detail"]))
        row8 = d["modes"].get(8)
        if row8 and row8["detail"]:
            check(row8["detail"] in d["detail"], "%s: the detail line holds the mode's description" % name)
            check(d["detail"].count(" \u00b7 ") >= 1 and d["detail"].split(" \u00b7 ")[0] == d["cells"][(8, 1)][2],
                  "%s: the detail line reads 'irrep \u00b7 description \u00b7 activity': %s" % (name, d["detail"]))
        check(d.get("shown", "").count("|") <= 1, "%s: the detail line is at most two lines: %s" % (name, d.get("shown")))
        #  The six zero rows: Tx..Rz and their irreps in the Sym column.
        zero = [r for r in sorted(d["modes"]) if d["modes"][r]["label"] in RIGID]
        check([d["cells"][(r, 2)][2] for r in zero] == [x for x in RIGID if x in
              [d["modes"][r]["label"] for r in zero]],
              "%s: the zero rows are Tx..Rz in the Mode column" % name)
        check(len(zero) == (5 if name == "co2" else 6),
              "%s: %d translation/rotation rows named" % (name, len(zero)))
        check(all(d["cells"][(r, 1)][2] == d["modes"][r]["irrep"] != "-" for r in zero),
              "%s: and their irreps are in the Sym column" % name)
        #  Sym column: ECCE's labels where the code printed none.
        for r, m in d["modes"].items():
            cell = d["cells"].get((r, 1))
            if m["irrep"] != "-" and cell is not None:
                check(cell[2] == m["irrep"], "%s mode %d: Sym column %s, analysis %s" % (name, r, cell[2], m["irrep"]))

    #  Narrow detail line: at most two lines, only whole words of the full text,
    #  an ellipsis only after a whole word, and the description kept.
    for name, d in sorted(results.items()):
        if d is None or not d.get("detail"):
            continue
        shown = d.get("shown", "")
        full = set(d["detail"].replace("\u00b7", " ").split())
        words = [w for w in shown.replace("|", " ").replace("\u00b7", " ").split()]
        bad = [w for w in words if w.rstrip("\u2026") not in full]
        check(not bad and shown.count("|") <= 1,
              "%s: the wrapped detail line has whole words only and two lines at most: %s" % (name, shown))
        desc = (d["modes"].get(8) or {}).get("detail", "")
        if desc and name != CO[0]:
            check(desc.split()[0] in shown, "%s: the description survives the narrow line: %s" % (name, shown))
    for name, d in sorted(results.items()):
        if d is None or "initial" not in d:
            continue
        freqs = [float(d["cells"][(r, 0)][2]) for r in range(1, 1 + max(r for r, c in d["cells"]))]
        first = next((i + 1 for i, v in enumerate(freqs) if abs(v) >= 10), None)
        check(d["initial"][0] == first and abs(d["initial"][1]) >= 10,
              "%s: opens on the first real vibration (row %s), not a translation: %s" % (name, first, d["initial"]))
        lo, hi = d["range"]
        top = max(freqs)
        check(hi >= 3400 and hi >= top and lo <= 0 and lo <= min(0, min(freqs)),
              "%s: the spectrum axis spans 0 to at least 3400 and every peak: %s" % (name, d["range"]))
    co = results.get(CO[0])
    if co is not None:
        say("  CO detail: %r -> %r" % (co["detail"], co.get("shown")))

    d = results.get(G16[0])
    if d is not None and codeSym:
        shown = [d["cells"][(r + 1, 1)][2] for r in range(len(codeSym))]
        check(shown == codeSym, "code's own Sym labels stay: %s (printed %s)" % (shown, codeSym))
        check(all(m["irrep"] in ("A1", "B1", "B2", "A2") for m in d["modes"].values() if m["irrep"] != "-"),
              "and ECCE's own analysis is still there for the tooltip")

    d = results.get(RAMAN[0])
    if d is not None:
        labels = [v[1].split()[0] for _, v in sorted(d["cols"].items())]
        check(labels == ["Frequency", "Sym", "Mode", "Infrared", "Raman"],
              "with Raman too the columns are %s" % labels)
        cut = [k for k, v in d["cells"].items() if not v[0]]
        check(not cut, "five columns still fit the docked width, every cell whole (cut: %s)" % cut[:5])
        check(base is not None and d["panel"][:2] == base["panel"][:2],
              "and the panel is still %s" % (d["panel"][:2],))

    failures = rt.failures + bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
