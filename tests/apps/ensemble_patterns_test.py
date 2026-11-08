#!/usr/bin/env python3
"""Coordination patterns of a CREST ensemble (#682): CLI and Geometry Trace panel.

    tests/apps/ensemble_patterns_test.py [--cli-only]

The ensemble is built from the shipped Na+(H2O)6 workflow structures (6_0, 5_1
and 4_2, twice each at different relative energies) and written as a CREST-style
crest_conformers.xyz.  ecce-contacts --group must find three patterns whose
representatives are each pattern's lowest-energy member; the Builder's Geometry
Trace panel must list the same three rows.
"""
import json
import csv
import os
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

WF = os.path.join(ROOT, "data", "client", "examples", "workflows", "na-hydration")
CONTACTS = os.path.join(ROOT, "scripts", "ecce_contacts.py")
BASE = os.path.join(HERE, "fixtures", "crest-butane-ensemble")
# (structure, relative energy kcal/mol): frame 1..6
FRAMES = (("6_0", 1.2), ("5_1", 0.4), ("4_2", 2.1), ("6_0", 0.0), ("5_1", 0.9), ("4_2", 1.5))
WANT = {"6_0": "Na: 6×O(H2O)", "5_1": "Na: 5×O(H2O)", "4_2": "Na: 4×O(H2O)"}
fails = []


def check(name, ok, detail=""):
    print(("PASS  " if ok else "FAIL  ") + name + ("" if ok else "  " + str(detail)))
    if not ok:
        fails.append(name)


def atoms(tag):
    lines = open(os.path.join(WF, "xtb-na_%s" % tag, "na_%s.xyz" % tag)).read().splitlines()
    return [l.split() for l in lines[2:2 + int(lines[0])]]


def crest_xyz(path):
    with open(path, "w") as f:
        for tag, rel in FRAMES:
            a = atoms(tag)
            f.write("%d\n %.8f\n" % (len(a), -30.0 + rel / 627.5095))
            for s, x, y, z in a:
                f.write("%s %s %s %s\n" % (s, x, y, z))


def cli(tmp):
    xyz = os.path.join(tmp, "crest_conformers.xyz")
    crest_xyz(xyz)
    out = subprocess.run([sys.executable, CONTACTS, xyz, "--group", "--format", "json"],
                         capture_output=True, text=True, check=True).stdout
    groups = json.loads(out)
    check("three patterns are found", len(groups) == 3, [g["pattern"] for g in groups])
    for tag in WANT:
        members = [i + 1 for i, (t, _r) in enumerate(FRAMES) if t == tag]
        best = min(members, key=lambda i: FRAMES[i - 1][1])
        g = [g for g in groups if g["pattern"] == WANT[tag]]
        check("pattern %s: representative is its lowest member (frame %d)" % (tag, best),
              len(g) == 1 and g[0]["representative"] == best and g[0]["count"] == 2, g)
    return xyz


def fixture(out, dav=False, proton=False):
    """The butane CREST calculation with the ion cluster's structures.  With
    dav, the data-server layout of calc-water-vib (as geomtrace_stress.py
    uses) holding only these two properties."""
    if dav:
        shutil.copytree(os.path.join(HERE, "fixtures", "calc-water-vib"), out)
        for d in (os.path.join(out, "Props"), os.path.join(out, "Props", ".DAV")):
            for name in os.listdir(d):
                if name != ".DAV":
                    os.remove(os.path.join(d, name))
    else:
        shutil.copytree(BASE, out, symlinks=True)
    a = atoms("6_0")
    n = len(a)
    chem = ["title: na_ensemble", "type: molecule", "num_atoms: %d" % n,
            "atom_info: symbol cart", "atom_list: "]
    chem += ["%s %s %s %s" % tuple(r) for r in a]
    chem += ["attr_list:"] + [" 0.000000   1   0   0   0"] * n
    chem += ["atom_type_list:"] + ["        1   1"] * n + ["num_bonds: 0", "bond_list: "]
    for sub in (("Parameters",), ("Parameters", ".DAV")) if dav else (("Parameters",),):
        open(os.path.join(out, *sub, "chemsys.mvm"), "w").write("\n".join(chem) + "\n")
    steps = []
    for tag, _r in FRAMES:
        row = "   ".join(c for r in atoms(tag) for c in r[1:])
        # A protomer: CREST appends the proton after the input's atoms.
        steps.append(row + "   0.5   0.5   9.5" if proton else row)
    head = open(os.path.join(BASE, "Props", "GEOMTRACE")).read().split("\n")[1]
    labels = ["%d-%s" % (i + 1, r[0]) for i, r in enumerate(a)]
    if proton:
        labels.append("%d-H" % (n + 1))
    head = re.sub(r'rowLabels="[^"]*"', 'rowLabels="%s"' % " ".join(labels), head)
    head = re.sub(r'rows="\d+"', 'rows="%d"' % (n + (1 if proton else 0)), head)
    with open(os.path.join(out, "Props", "GEOMTRACE"), "w") as f:
        f.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n' + head + "\n")
        for i, s in enumerate(steps):
            f.write('  <step number="%d">%s</step>\n' % (i + 1, s))
        f.write("</tsvectable>\n")
    with open(os.path.join(out, "Props", "CONFREL"), "w") as f:
        f.write('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
                '<tsvector name="CONFREL" rowLabel="Geometry Step" rows="1" '
                'units="kcal/mol">\n')
        for i, (_t, r) in enumerate(FRAMES):
            f.write('  <step number="%d">%.3f</step>\n' % (i + 1, r))
        f.write("</tsvector>\n")
    # The data server lists properties from Props/.DAV, local mode from .ecce-meta.
    if dav:
        for key in ("GEOMTRACE", "CONFREL"):
            shutil.copy(os.path.join(out, "Props", key),
                        os.path.join(out, "Props", ".DAV", key))


def panel(tmp, proton=False):
    import apps
    import fixture as fx
    import isolate
    import xdisplay
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fx.USER
    settings = isolate.apply(apps.INSTALL)
    state = settings["ECCE_REALUSERHOME"]
    isolate.killLeftovers(state)
    os.environ["ECCE_NO_REAP"] = "1"
    src = os.path.join(tmp, "na-protomers" if proton else "na-ensemble")
    fixture(src, dav=True, proton=proton)
    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        try:
            if not all(apps.serviceState(display).values()):
                check("services came up", False, log)
                return
            fx.ensureRealUserAccount()
            restore = fx.settleUpgradeNotices()
            try:
                url, error = fx.install("na-ensemble", source_dir=src)
                if error:
                    check("install the fixture", False, error)
                    return
                scene = os.path.join(tmp, "scene")
                csvout = os.path.join(tmp, "ensemble.csv")
                kinds = [("Conformer_Search", 0), ("Conformer_search_(simple)", 0),
                         ("Protonation_Sites", 0), ("Tautomers", 0),
                         ("Solvation_Shell", 0), ("Docking_(aISS)", 1), ("Other", 1)]
                if proton:
                    # Protomers have the input's atoms plus the proton (#912).
                    open(scene, "w").write("gtfocus\ngtenrows\ngtenselect 1\n"
                                           "gtenselect 2\ngtclose\nhold 1\n")
                else:
                  open(scene, "w").write(
                    "gtfocus\ngtenrows\n" +
                    "".join("gtenkind %s %d\n" % k for k in kinds) +
                    "".join("gtenselect %d\n" % k for k in range(1, len(FRAMES) + 1)) +
                    "gtencalc orca_es 1 3\ngtencsv %s\ngtclose\nhold 1\n" % csvout)
                args = ("-context", url)
                if os.path.realpath(state) != os.path.realpath(os.path.expanduser("~")):
                    auth = fx.authFile(os.path.join(state, ".ECCE", "auth.pipe"), user=fx.USER)
                    args = ("-pipe", auth) + args
                result = apps.run(display, "builder", args=args, windowTimeout=120,
                                  settle=120,
                                  env={"ECCE_VIEWER_SCENE": scene,
                                       "ECCE_VIEWER_SCENE_OUT": tmp,
                                       "ECCE_REALUSER": fx.USER})
                log = result.log or ""
                check("builder did not crash", not result.crashed, result.signalName)
                failed = os.path.join(tmp, "FAILED")
                if os.path.exists(failed):
                    check("the scene ran", False, open(failed).read())
                if proton:
                    n = len(atoms("6_0")) + 1
                    sel = re.findall(r"GTSTRESS: enselect \d+ ok=1 .* natoms=(\d+)", log)
                    check("selecting a protomer shows all %d atoms, proton included" % n,
                          len(sel) == 2 and all(int(v) == n for v in sel), sel or log[-3000:])
                    # The calculation's final structure is a protomer too (#920).
                    bad = re.findall(r".*coordinates left unchanged", log)
                    check("the final structure takes the protomer's atoms", not bad, bad)
                    return
                kind = re.search(r"GTSTRESS: ensemble kind=(.*) rows=(\d+)", log)
                check("the panel is the Conformers panel with %d rows" % len(FRAMES),
                      kind and kind.group(1) == "Conformers" and
                      int(kind.group(2)) == len(FRAMES), kind and kind.groups() or log[-6000:])
                for rt, poses in kinds:
                    want = {"Conformer_Search": "Conformers", "Conformer_search_(simple)": "Conformers",
                            "Protonation_Sites": "Protonation Sites", "Tautomers": "Tautomers",
                            "Solvation_Shell": "Clusters", "Docking_(aISS)": "Poses",
                            "Other": "Poses"}[rt]
                    check("run type %s is titled %s" % (rt, want),
                          "enkind %s = %s\n" % (rt, want) in log)
                rows = [r.split(" | ") for r in
                        re.findall(r"GTSTRESS: enrow (.*)$", log, re.M)]
                check("ranks 1..n in order", [r[0] for r in rows] ==
                      [str(i) for i in range(1, len(FRAMES) + 1)], rows)
                de = [float(r[1]) for r in rows]
                check("lowest energy first", de == sorted(de) and
                      de == sorted(r for _t, r in FRAMES), de)
                pop = [float(r[2]) for r in rows]
                check("populations sum to 100 percent", abs(sum(pop) - 100) < 0.3 and
                      pop == sorted(pop, reverse=True), pop)
                order = sorted(range(len(FRAMES)), key=lambda i: FRAMES[i][1])
                for k, i in enumerate(order, 1):
                    tag = FRAMES[i][0]
                    row = rows[k - 1]
                    check("row %d has pattern %s" % (k, WANT[tag]),
                          len(row) == 4 and row[3] == WANT[tag], row)
                    ax = atoms(tag)[0]
                    m = re.search(r"GTSTRESS: enselect %d ok=1 atom1 (\S+) (\S+) (\S+) plot=(\d+)" % k, log)
                    check("selecting row %d shows frame %d (%s)" % (k, i + 1, tag),
                          m and all(abs(float(m.group(j + 1)) - float(ax[j + 1])) < 1e-4
                                    for j in range(3)) and int(m.group(4)) == k,
                          m and m.groups())
                # New Calculation from Selected (#708): ranks 1 and 3.
                for k in (1, 3):
                    ax = atoms(FRAMES[order[k - 1]][0])
                    m = re.search(r"GTSTRESS: encalc %d type=(\S+) run=(.*?) mult=(\d+) name=(\S+) atoms=(\d+) first (\S+) (\S+) (\S+) "
                                  r"last (\S+) (\S+) (\S+)" % k, log)
                    want = [float(v) for v in ax[0][1:]] + [float(v) for v in ax[-1][1:]]
                    check("new calculation from rank %d: name ends _conf%d" % (k, k),
                          m and m.group(4).endswith("_conf%d" % k), m and m.groups())
                    check("new calculation from rank %d: code orca_es, run type Geometry, mult carried" % k,
                          m and m.group(1) == "orca_es" and m.group(2) == "Geometry" and m.group(3) == "1",
                          m and m.groups())
                    check("new calculation from rank %d has that frame's geometry" % k,
                          m and int(m.group(5)) == len(ax) and
                          all(abs(float(m.group(6 + j)) - want[j]) < 1e-4 for j in range(6)),
                          m and m.groups())
                made = re.search(r"GTSTRESS: encalc made=(\d+) why=(.*)", log)
                check("two calculations created, no error",
                      made and made.group(1) == "2" and not made.group(2), made and made.groups())
                if os.path.exists(csvout):
                    lines = open(csvout).read().splitlines()
                    check("CSV has a header and %d rows" % len(FRAMES),
                          len(lines) == len(FRAMES) + 1 and lines[0].startswith("rank,"), lines)
                else:
                    check("CSV written", False, log[-3000:])
                stored_superposition(tmp, display, state)
            finally:
                restore()
                fx.remove("na-ensemble")
        finally:
            apps.stopServices(display)


def stored_superposition(tmp, display, state):
    """Compare against an SVD Kabsch fit, independent of the quaternion fit
    used by the viewer, on the stored CREST n-butane search."""
    import numpy as np
    from PIL import Image
    import apps
    import fixture as fx

    src = os.path.join(tmp, "stored-butane")
    shutil.copytree(BASE, src, symlinks=True)
    trace = ET.parse(os.path.join(BASE, "Props", "GEOMTRACE")).getroot()
    heavy = [i for i, label in enumerate(trace.attrib["rowLabels"].split())
             if label.split("-", 1)[1] not in ("H", "X")]
    frames = [np.array([float(x) for x in s.text.split()]).reshape(-1, 3)[heavy]
              for s in trace.findall("step")]
    energies = [float(s.text) for s in ET.parse(
        os.path.join(BASE, "Props", "CONFREL")).getroot().findall("step")]
    ranked = [frames[i] for i in sorted(range(len(frames)), key=lambda i: energies[i])]

    def rmsd(reference, moving):
        a = reference - reference.mean(axis=0)
        b = moving - moving.mean(axis=0)
        u, _s, vt = np.linalg.svd(b.T @ a)
        correction = np.diag([1.0, 1.0, np.linalg.det(u @ vt)])
        return np.sqrt(np.sum((b @ (u @ correction @ vt) - a) ** 2) / len(a))

    png = os.path.join(tmp, "stored-table.png")
    csvpath = os.path.join(tmp, "stored-fits.csv")
    scene = os.path.join(tmp, "stored-scene")
    with open(scene, "w") as f:
        f.write("gtfocus\ngtenselect 3 1 5\ngtenfit\ngtencsv %s\ngtensort 1 down\ngtenfit\n"
                "gtenpng %s\ngtenthumbs\ngtenpng %s\n"
                "gtenselect 1\ngtenfit\ngtenselect 5 3 1\ngtenfit\n"
                "gtenviewer off\ngtenfit\ngtclose\nhold 1\n" % (csvpath, png, png + ".png"))
    args = ("-context", "file://" + src + "/")
    if os.path.realpath(state) != os.path.realpath(os.path.expanduser("~")):
        auth = fx.authFile(os.path.join(state, ".ECCE", "auth.pipe"), user=fx.USER)
        args = ("-pipe", auth) + args
    result = apps.run(display, "builder", args=args, windowTimeout=120, settle=120,
                      env={"ECCE_VIEWER_SCENE": scene, "ECCE_VIEWER_SCENE_OUT": tmp,
                           "ECCE_REALUSER": fx.USER, "GTK_THEME": "Adwaita",
                           "ECCE_OPEN_PANEL": "Conformers"})
    log = result.log or ""
    check("stored CREST superposition scene completes without a crash",
          not result.crashed and not os.path.exists(os.path.join(tmp, "FAILED")), log[-5000:])
    rows = re.findall(r"GTSTRESS: enfit (\d+) \| ([0-9.]+) \| (#[0-9A-Fa-f]{6})", log)
    for group, ranks in enumerate(([3, 1, 5], [3, 1, 5], [5, 3, 1])):
        actual = rows[3*group:3*group+3]
        check("fit %d keeps selection order (including after sorting)" % group,
              [int(r[0]) for r in actual] == ranks, actual)
        for rank, value, _colour in actual:
            expected = rmsd(ranked[ranks[0]-1], ranked[int(rank)-1])
            check("fit %d member %s RMSD matches independent SVD to 0.0001 Å" % (group, rank),
                  abs(float(value) - expected) < 1e-4, (value, expected))
        check("fit %d uses distinct shared palette colours" % group,
              len({r[2].lower() for r in actual}) == 3 and
              [r[2].lower() for r in actual] == ["#0072b2", "#d55e00", "#cc79a7"], actual)
    states = re.findall(r"GTSTRESS: enfit members=(\d+) visible=(\d+)", log)
    check("multi-select draws overlay; one row and closing restore normal view",
          states == [("3", "1"), ("3", "1"), ("0", "0"), ("3", "1"), ("3", "0")], states)
    thumbs = [int(x) for x in re.findall(r"GTSTRESS: enthumb (\d+)", log)]
    check("every stored ensemble member has a cached thumbnail",
          thumbs == list(range(1, len(frames) + 1)), thumbs)
    if os.path.exists(csvpath):
        with open(csvpath) as f:
            exported = list(csv.DictReader(f))
        fitted = [r for r in exported if r["reference_rank"]]
        check("CSV exports the selected members' reference, RMSD and colour",
              len(fitted) == 3 and all(r["reference_rank"] == "3" and
              abs(float(r["heavy_atom_rmsd_angstrom"]) - rmsd(ranked[2], ranked[int(r["rank"])-1])) < 1e-4
              and r["colour"].startswith("#") for r in fitted), fitted)
    else:
        check("CSV with RMSDs was written", False)
    if os.path.exists(png):
        with Image.open(png) as image:
            check("PNG table contains every row", image.height == 32 + 62*len(frames), image.size)
            for i in range(len(frames)):
                crop = image.crop((5, 35 + 62*i, 77, 91 + 62*i)).convert("RGB")
                check("thumbnail %d contains rendered atoms and bonds" % (i+1),
                      len(crop.getcolors(10000) or []) > 3)
        check("export reuses the cached pictures",
              open(png, "rb").read() == open(png + ".png", "rb").read())
    else:
        check("PNG table was written", False, log[-5000:])


def main():
    tmp = tempfile.mkdtemp(prefix="patterns-")
    try:
        cli(tmp)
        if "--cli-only" not in sys.argv:
            panel(tmp)
            panel(tmp, proton=True)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
