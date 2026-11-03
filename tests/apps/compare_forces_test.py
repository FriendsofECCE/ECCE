#!/usr/bin/env python3
"""Stored final gradients: shared CLI, Organizer dialog and Copy results."""
import json
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import apps
import run_tests
from compare_energies_test import makeCalc, PROJECT_META, check, failures


def gradient(calc, values, units="Hartree/Bohr"):
    path = os.path.join(calc, "Props", "EGRADVEC")
    tree = ET.parse(path)
    tree.getroot().text = " ".join(str(v) for v in values)
    tree.getroot().set("units", units)
    tree.write(path)


def run(display):
    with tempfile.TemporaryDirectory(prefix="ecce-forces-") as work:
        data = os.path.join(work, "data")
        project = os.path.join(data, "users", "local", "forces")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        a = makeCalc(project, "A", -10, "NWChem")
        b = makeCalc(project, "B", -9.99, "NWChem")
        c = makeCalc(project, "C", -9.99, "CP2K")
        # The fixture's atom sequence is O H H; norms are 5, 2, 4 eV/A.
        gradient(a, [0]*9)
        vector = [0,3,4, 0,0,2, 0,0,4]
        gradient(b, [v/51.42208619 for v in vector])
        gradient(c, vector, "eV/Angstrom")
        env = dict(os.environ, ECCE_HOME=apps.INSTALL,
                   HOME=os.path.join(work, "home"),
                   ECCE_REALUSERHOME=os.path.join(work, "home"),
                   ECCE_LOCAL_DATA=data)
        os.makedirs(env["HOME"])
        exe = os.path.join(apps.INSTALL, "bin", "ecce-si-export")

        def cli(x, y, as_json=True):
            return subprocess.run([exe, "--compare-forces", x, y] +
                                  (["--json"] if as_json else []),
                                  env=env, text=True, capture_output=True, timeout=60)

        result = cli(a, b)
        check(result.returncode == 0, "CLI succeeds", result.stderr + result.stdout)
        r = json.loads(result.stdout)
        rows = {v["element"]: v for v in r["rows"]}
        check(rows["H"]["n"] == 2 and abs(rows["H"]["max"]-4) < 1e-8 and
              abs(rows["H"]["rms"]-math.sqrt(10)) < 1e-8, "CLI H max/rms")
        check(abs(rows["O"]["max"]-5) < 1e-8 and
              abs(rows["all atoms"]["rms"]-math.sqrt(15)) < 1e-8, "CLI O and all atoms")
        check(abs(r["delta_e_ev"]-0.27211386245988) < 1e-9 and
              abs(r["delta_e_mev_per_atom"]-90.70462081996) < 1e-8, "CLI energy units and sign")
        other = json.loads(cli(a, c).stdout)
        check(other["delta_e_ev"] is None and other["rows"] == r["rows"] and
              "No energy difference: the two calculations use different methods." in other["notes"],
              "different code keeps forces and suppresses energy")
        plain = cli(a, b, False).stdout
        check("eV/cell" in plain and "meV/atom" in plain and "kJ/mol" not in plain,
              "plain table has only requested units")
        # Change only the final trace, leaving the starting geometry identical.
        path = os.path.join(b, "Props", "GEOMTRACE")
        tree = ET.parse(path)
        step = list(tree.getroot())[-1]
        values = step.text.split()
        values[0] = str(float(values[0])+0.03)
        step.text = " ".join(values)
        tree.write(path)
        moved = json.loads(cli(a, b).stdout)
        check(moved["ok"] and moved["geometry_rms_angstrom"] > 0.01 and
              any("Warning:" in n for n in moved["notes"]), "last geometry step triggers warning")
        cmds = os.path.join(work, "commands")
        png = os.path.join(work, "forces.png")
        with open(cmds, "w") as handle:
            handle.write("compareforcesdialog file://%s file://%s %s\n" % (a,b,png))
        result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": "file://" + a,
                               "ECCE_LOCAL_DATA": data})
        check(not result.crashed, "Organizer opens force dialog", result.log or "")
        answers = re.findall(r"ECCE_TEST_ORGANIZER: compareforcesdialog .*?: (ok.*)", result.log or "")
        text = answers[0] if answers else ""
        check("H\t2\t4\t3.16227766" in text and "O\t1\t5\t5" in text,
              "visible dialog rows contain force statistics", text)
        check("H\t2\t4\t3.16227766017" in text and "all atoms\t3\t5\t3.87298334621" in text,
              "dialog Copy has tab-separated force table", text)
        check("eV/cell" in text and "meV/atom" in text and "Warning:" in text,
              "dialog includes energy and geometry warning", text)
        check(os.path.isfile(png) and os.path.getsize(png) > 1000, "dialog screenshot captured")
        os.remove(os.path.join(b, "Props", "EGRADVEC"))
        missing = cli(a, b)
        check(missing.returncode == 1 and not json.loads(missing.stdout)["ok"], "missing forces refused")


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("compare forces", ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
