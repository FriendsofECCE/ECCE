#!/usr/bin/env python3
"""The orbitals of an ORCA IRC belong to the TS and the two end points (#751), headless.

    tests/apps/orca_irc_orbitals_test.py [--build build-cmake] [--keep]

Stores what the real orca.mo returns for the MOLECULAR ORBITALS block of
tests/parsers/fixtures/orca/hcn_irc_mos.out (an IRC run; the block is the one
orca.mo emits as MO and as the one-table PTMO/PTORB) together with the
path's energies and structures (PTMO/PTORB hold the TS, then the backward
and forward end), opens the MOs and IRC Path panels and selects the TS
(point 4 of 7), both ends (points 1 and 7) and two points between.  Checked:
at those three the MOs panel lists that point's stored orbital energies; at
the others it lists none and the note says ORCA gives none there.  Exit status 77 (skip) without Xvfb, import or the binaries.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))
import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
import irc_panel_test as ip  # noqa: E402
from ecceqm_point_orbitals_test import (  # noqa: E402
    labels, numbers, rowEnergies, listed, tableDoc, tsDoc, vectorDoc)
from ecceqm_scan_panel_test import parsed  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
FIXTURE = os.path.join(REPO, "tests", "parsers", "fixtures", "orca", "hcn_irc_mos.out")
PARSE = os.path.join(REPO, "scripts", "parsers", "orca.mo")
PTMO = os.path.join(REPO, "scripts", "parsers", "orca.ptmo")
TS = 3


def moBlock():
    text = open(FIXTURE).read()
    block = text.split("MOLECULAR ORBITALS (RHF, ROHF)")[1].split("MULLIKEN POPULATION")[0]
    return "\n".join(block.splitlines()[2:])


def endTables():
    """PTORB/PTMO records of the TS and the two end blocks, as orca.ptmo gives."""
    out = []
    for part in open(FIXTURE).read().split("ECCE-IRC-END-ORBITALS")[1:]:
        block = "\n".join(part.split("MULLIKEN POPULATION")[0].splitlines()[2:])
        run = subprocess.run(["perl", PTMO, ".", "IRC", "Hartree Fock", "RHF", "0"],
                             input=block, capture_output=True, text=True)
        check(run.returncode == 0, "orca.ptmo parsed a block")
        out.append(parsed(run.stdout))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "orcaircmo", {}, local=True, keep=args.keep)
    png = os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)

    run = subprocess.run(["perl", PARSE, ".", "IRC", "Hartree Fock", "RHF", "0"],
                         input=moBlock(), capture_output=True, text=True)
    check(run.returncode == 0, "orca.mo parsed the block")
    rec = parsed(run.stdout)
    for key in ("MO", "ORBENG", "ORBOCC"):
        check(len(rec.get(key, [])) == 1, "orca.mo emitted %s once" % key)
    ends = endTables()
    check(len(ends) == 3, "the fixture has the TS and two end-point blocks")
    ptorb = [numbers(e["PTORB"][0]) for e in ends]
    ptmo = [numbers(e["PTMO"][0]) for e in ends]
    eng = numbers(rec["ORBENG"][0])
    nbf = len(eng)
    # The run's own summary energies; its trajectory is not kept, so the
    # structures are the middle 7 of the hcn_irc.out path.
    summary = open(FIXTURE).read().split("IRC PATH SUMMARY")[1].split("SUGGESTED CITATIONS")[0]
    energy = [float(m) for m in re.findall(
        r"^\s+\d+\s+(-?\d+\.\d+)\s+-?\d+\.\d+\s+\d+\.\d+\s+\d+\.\d+", summary, re.M)]
    frames = ip.orcaPath()[1][7:14]
    check(len(energy) == len(frames) == 7, "7 IRC points")
    coord = [float(i - TS) for i in range(len(energy))]

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        project = os.path.join(s.localData(), "users", "local", "qm")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "calc-orca-irc-mo")
        shutil.rmtree(calc, ignore_errors=True)
        shutil.copytree(ip.BASE, calc, symlinks=True)
        props = os.path.join(calc, "Props")
        for name in os.listdir(props):
            if name != "ESCF":
                os.unlink(os.path.join(props, name))
        with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as f:
            f.write(ip.HCN)
        docs = {
            "IRCCOORD": ip.vector("IRCCOORD", coord, "Point"),
            "IRCENERGY": ip.vector("IRCENERGY", energy, "Hartree"),
            "IRCGEOM": ip.tsvector(frames),
            "ORBENG": vectorDoc("ORBENG", eng, "Hartree"),
            "ORBOCC": vectorDoc("ORBOCC", numbers(rec["ORBOCC"][0]), "electrons"),
            "MO": tableDoc("MO", nbf, nbf, numbers(rec["MO"][0])),
            "PTORB": tsDoc("PTORB", nbf, 2, "Energy Occupation",
                           ptorb, "NA"),
            "PTMO": tsDoc("PTMO", nbf, nbf, labels(nbf, "BasFun"),
                          ptmo, "Electrons"),
        }
        for name, text in docs.items():
            with open(os.path.join(props, name), "w") as f:
                f.write(text)

        steps = [1, TS, 0, 6, 5]
        lines = ["wait 6000", "panel IRC Path", "wait 1500"]
        for k in steps:
            lines += ["ircclick %d" % k, "wait 1000", "panel MOs", "wait 1500", "mostate",
                      "panel IRC Path"]
        lines.append("quit")
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + calc + "/"],
            lines, png, "orcaircmo", {"ECCE_PANEL_MODE": "detail"})
        check("segmentation" not in text.lower(), "no crash in the Builder log")
        notes = re.findall(r"BUILDER: monote (.*)", text)
        if len(notes) != 5 or text.count("BUILDER: mostate\n") != 5:
            say(text[-3000:])
            check(False, "five MOs panel states were reported")
            return 1
        say("notes: %s" % " / ".join(notes))
        rows = [rowEnergies(text, i) for i in range(5)]
        ends_eng = [ptorb[t][0::2] for t in range(3)]
        for i, (name, want) in enumerate([("point 2", None), ("TS", ends_eng[0]),
                                          ("backward end", ends_eng[1]),
                                          ("forward end", ends_eng[2]), ("point 6", None)]):
            if want is None:
                check("at the TS and the two end points" in notes[i] and not rows[i],
                      "%s: no orbitals listed, note says where ORCA gives them" % name)
            else:
                check("Orbitals and density of point" in notes[i] and listed(rows[i], want),
                      "%s: its own stored energies and the point note" % name)
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
