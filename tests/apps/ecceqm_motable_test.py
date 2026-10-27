#!/usr/bin/env python3
"""
The MOs table on ECCE-QM ethane and ethene (#533), canonical and localised,
at the pane's default docked size, headless.

The Builder scene command `plotshot <name> <w> <h> MOs:dockedtable` (or
`dockedloctable`) leaves the pane where the layout put it, prints the table
(PLOTSHOT-MOTABLE: header, rows, header height, the controls' rectangles and
how many overlap) and saves a picture.  Checked: orbitals run 1..N in one
direction (highest at the top, the lowest at the bottom), the HOMO and the
LUMO above it are in view, the columns say what each orbital is, the header is one line high,
and nothing overlaps the table.

    ECCE_MOTABLE_PNG=<dir>   also copy the pictures there
    ECCE_MOTABLE_TAG=<tag>   picture name prefix (default "after")
    ECCE_MOTABLE_REPORT=1    print the table state but do not fail

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import xdisplay   # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402

EXAMPLES = os.path.join(REPO, "data", "client", "examples", "organic", "bonding-in-hydrocarbons")
CALCS = {"ethane": os.path.join(EXAMPLES, "ecceqm-ethane", "stored", "ethane"),
         "ethene": os.path.join(EXAMPLES, "ecceqm-ethene", "stored", "ethene"),
         "o2": os.path.join(REPO, "tests", "ecceqm", "fixtures", "o2-roks-loc")}
VIEWS = (("canonical", "dockedtable"), ("localised", "dockedloctable"),
         ("locsorted", "dockedlocsort"), ("spin", "dockedspin"))
SCREEN = os.environ.get("ECCE_MOTABLE_SCREEN", "1366x768x24")


def parseState(log, name):
    """The lines PLOTSHOT-MOTABLE <name> prints, as a dict plus the rows."""
    state = {"row": []}
    lines = log.splitlines()
    for i, line in enumerate(lines):
        if line.startswith("PLOTSHOT-MOTABLE %s" % name):
            for item in lines[i + 1:]:
                if item.startswith("PLOTSHOT"):
                    break
                key, _, value = item.partition(" ")
                if key == "row":
                    state["row"].append([c.strip() for c in value.split("|")])
                elif key == "header":
                    state[key] = [c.strip() for c in value.split("|")]
                else:
                    state[key] = value
            break
    return state


def check(state, localised, view=""):
    problems = []
    if view == "spin":
        text = state.get("spinView", "")
        if "singly occupied" not in text:
            problems.append("no SOMO list in the spin density view")
        if "Mulliken atomic spin populations" not in text:
            problems.append("no atomic spin populations (or no note why)")
        # O2 triplet: two SOMOs, about one unpaired electron on each O.
        if text.count("\\n") < 5:
            problems.append("spin view too short: %r" % text)
        # Tables: "MO | Spin | ..." rows, then "Atom | Spin population" rows.
        lines = text.split("\\n")
        somo = [l for l in lines if l.split(" | ")[0].strip().isdigit()
                and len(l.split(" | ")) == 4]
        pops = []
        for l in lines:
            c = l.split(" | ")
            if len(c) == 3 and c[0] != "Atom":
                try:
                    pops.append((float(c[1]), float(c[2])))
                except ValueError:
                    pass
        if len(somo) != 2:
            problems.append("O2 should list 2 SOMOs, got %r" % somo)
        if len(pops) != 2 or abs(sum(p[0] for p in pops) - 2.0) > 0.05:
            problems.append("O2 spin populations should be 2 atoms summing to 2: %r" % pops)
        if "Spin population (e) | Share (%)" not in text:
            problems.append("population headings lack the unit and the share")
        if abs(sum(p[1] for p in pops) - 100.0) > 0.2:
            problems.append("shares should add up to 100 %%: %r" % pops)
        if [abs(p[0]) for p in pops] != sorted((abs(p[0]) for p in pops), reverse=True):
            problems.append("atoms not sorted by |spin|: %r" % pops)
        return problems
    header = state.get("header")
    if not header:
        return ["no table state in the log"]
    rows = state["row"]
    # An open-shell table starts with a Type column and lists alpha, then beta.
    off = 1 if header[0] == "Type" else 0
    sets = {}
    for r in rows:
        sets.setdefault(r[0] if off else "", []).append(r)
    if off and set(sets) != {"alpha", "beta"}:
        problems.append("open-shell table lacks an alpha or a beta set: %s" % sorted(sets))
    for spin, part in sets.items():
        mos = [int(r[off]) for r in part]
        energies = [float(r[off + 1]) for r in part]
        if view == "locsorted":
            # sorted by Bond type: equal types are together, the sets stay whole
            col = header.index("Bond type")
            kinds = [r[col] for r in part]
            runs = [k for i, k in enumerate(kinds) if i == 0 or kinds[i - 1] != k]
            if sorted(kinds) != kinds or len(runs) != len(set(kinds)):
                problems.append("%s not sorted by Bond type: %s" % (spin, kinds[:12]))
            if sorted(mos) != list(range(1, len(mos) + 1)):
                problems.append("%s sort lost or doubled an orbital" % spin)
            continue
        if not localised and mos != list(range(len(mos), 0, -1)):
            problems.append("%s MO numbers are not N..1 descending: %s" % (spin, mos[:12]))
        # Lowest energy at the bottom, in the localised table too (H1).
        if energies != sorted(energies, reverse=True):
            problems.append("%s energies do not fall down the table: %s" % (spin, energies[:8]))
    if view != "locsorted" and state.get("selectedVisible") != "1":
        problems.append("the HOMO row is not in view")
    if not localised and state.get("aboveSelectedVisible") != "1":
        problems.append("the LUMO row is not in view")
    wanted = ["Bond type", "Atoms"] if localised else ["Main atoms"]
    for w in wanted:
        if w not in header:
            problems.append("no %r column (%s)" % (w, header))
        else:
            col = header.index(w)
            if not all(r[col] for r in rows if len(r) > col):
                problems.append("empty %r cells" % w)
    if int(state.get("headerHeight", 999)) > 40:
        problems.append("header %s px high" % state.get("headerHeight"))
    if int(state.get("overlaps", 1)) != 0:
        problems.append("%s controls overlap" % state.get("overlaps"))
    return problems


def main():
    xdisplay.SCREEN = SCREEN
    tag = os.environ.get("ECCE_MOTABLE_TAG", "after")
    report = bool(os.environ.get("ECCE_MOTABLE_REPORT"))
    pngdir = os.environ.get("ECCE_MOTABLE_PNG")

    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "motable-localdata")
        urls = installLocal(folder, CALCS)
        os.environ["ECCE_LOCAL_DATA"] = folder
        restore = fixture.settleUpgradeNotices()
        bad = []
        try:
            for calc, url in urls.items():
                for view, mode in VIEWS:
                    if view == "spin" and calc != "o2":
                        continue
                    shot = "%s-%s-%s" % (tag, calc, view)
                    out = os.path.join(folder, "shots")
                    os.makedirs(out, exist_ok=True)
                    scene = os.path.join(folder, "scene")
                    with open(scene, "w") as h:
                        h.write("plotshot %s 0 0 MOs:%s\nhold 1\n" % (shot, mode))
                    result = apps.run(display, "builder",
                                      args=("-context", url), windowTimeout=120,
                                      settle=300,
                                      env={"ECCE_PANEL_FULLSCREEN": "1", "ECCE_VIEWER_SCENE": scene,
                                           "ECCE_VIEWER_SCENE_OUT": out})
                    log = result.log or ""
                    state = parseState(log, shot)
                    if report:
                        print("---- %s" % shot)
                        print("\n".join("%s %s" % (k, v) for k, v in state.items()
                                        if k != "row"))
                        for r in state["row"]:
                            print("   ", " | ".join(r))
                    problems = check(state, view != "canonical", view)
                    png = os.path.join(out, shot + ".png")
                    if not os.path.exists(png):
                        problems.append("no picture")
                    elif pngdir:
                        os.makedirs(pngdir, exist_ok=True)
                        shutil.copy(png, pngdir)
                    print("%s  %s%s" % ("ok  " if not problems else "FAIL", shot,
                                        "" if not problems else ": " + "; ".join(problems)))
                    if problems:
                        bad.append(shot)
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if bad and not report:
            results.fail("motable", "wrong: %s" % ", ".join(bad))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
