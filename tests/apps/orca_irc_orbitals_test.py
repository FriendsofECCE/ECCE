#!/usr/bin/env python3
"""The orbitals of an ORCA IRC belong to the TS point only (#751), headless.

    tests/apps/orca_irc_orbitals_test.py [--build build-cmake] [--keep]

Stores what the real orca.mo returns for the MOLECULAR ORBITALS block of
tests/parsers/fixtures/orca/hcn_irc_mos.out (an IRC run; the block is the one
orca.mo emits as MO and as the one-table PTMO/PTORB) together with the
path's energies and structures, opens the MOs and IRC Path panels and selects
the TS (point 11 of 21) and two other points.  Checked: at the TS the MOs
panel lists the stored orbital energies and its note names the TS point; at
the other points it lists no orbitals and the note says ORCA gives none
there.  Exit status 77 (skip) without Xvfb, import or the binaries.
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
TS = 10


def moBlock():
    text = open(FIXTURE).read()
    block = text.split("MOLECULAR ORBITALS (RHF, ROHF)")[1].split("MULLIKEN POPULATION")[0]
    return "\n".join(block.splitlines()[2:])


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
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
    for key in ("MO", "ORBENG", "ORBOCC", "PTMO", "PTORB"):
        check(len(rec.get(key, [])) == 1, "orca.mo emitted %s once" % key)
    eng = numbers(rec["ORBENG"][0])
    nbf = len(eng)
    energy, frames = ip.orcaPath()
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
                           [numbers(rec["PTORB"][0])], "NA"),
            "PTMO": tsDoc("PTMO", nbf, nbf, labels(nbf, "BasFun"),
                          [numbers(rec["PTMO"][0])], "Electrons"),
        }
        for name, text in docs.items():
            with open(os.path.join(props, name), "w") as f:
                f.write(text)

        lines = ["wait 6000", "panel IRC Path", "wait 1500", "ircclick 3", "wait 1000",
                 "panel MOs", "wait 1500", "mostate",
                 "panel IRC Path", "ircclick %d" % TS, "wait 1000",
                 "panel MOs", "wait 1500", "mostate",
                 "panel IRC Path", "ircclick 17", "wait 1000",
                 "panel MOs", "wait 1500", "mostate", "quit"]
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", "file://" + calc + "/"],
            lines, png, "orcaircmo", {"ECCE_PANEL_MODE": "detail"})
        check("segmentation" not in text.lower(), "no crash in the Builder log")
        notes = re.findall(r"BUILDER: monote (.*)", text)
        if len(notes) != 3 or text.count("BUILDER: mostate\n") != 3:
            say(text[-3000:])
            check(False, "three MOs panel states were reported")
            return 1
        say("notes: %s" % " / ".join(notes))
        other1, at_ts, other2 = (rowEnergies(text, i) for i in range(3))
        check("only at the TS" in notes[0] and not other1,
              "point 4: no orbitals listed, note says ORCA gives none")
        check("TS point (11)" in notes[1] and listed(at_ts, eng),
              "TS: the stored energies and a note naming the TS point")
        check("only at the TS" in notes[2] and not other2,
              "point 18: no orbitals listed, note says ORCA gives none")
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
