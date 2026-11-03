#!/usr/bin/env python3
"""
The MO Diagram panel on a single atom (#813): the aufbau view, headless.

The stored genchem atom examples O (ROKS), Ne and Ar are opened with the MO
Diagram panel; the Builder scene commands `modiagram <name> aufbaupng 0|1`
draw it as boxes (B) and to scale (A), write the pictures and list what was
drawn.  Checked: the configuration, the arrows of each subshell (Hund's rule
in O 2p), the core flag in drawing A, and that a click on a level computes
the orbital.  ECCE_AUFBAU_PNG=<dir> also keeps the pictures.

Same installed tree and isolation as modiagram_panel_test.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps                   # noqa: E402
import isolate                # noqa: E402
import orca_mo_test as base   # noqa: E402
import xdisplay               # noqa: E402
from ecceqm_builder_test import installLocal   # noqa: E402

ATOMS = os.path.join(REPO, "data", "client", "examples", "genchem", "atoms")
# name -> (configuration, {subshell: (up arrows, down arrows)})
CASES = {
    "O": ("O: 1s2 2s2 2p4", {"1s": (1, 1), "2s": (1, 1), "2p": (3, 1)}),
    "Ne": ("Ne: 1s2 2s2 2p6", {"1s": (1, 1), "2p": (3, 3)}),
    "Ar": ("Ar: 1s2 2s2 2p6 3s2 3p6", {"3s": (1, 1), "3p": (3, 3)}),
}


def parse(text):
    """{subshell: (core, up, down, [eV])} from the scene's listing."""
    shells, current = {}, None
    for line in text.splitlines():
        m = re.match(r"shell (\S+).* core=(\d) electrons=(\d+)", line)
        if m:
            current = m.group(1)
            shells[current] = [int(m.group(2)), 0, 0, []]
            continue
        m = re.match(r"\s+orbital (\d+) eV=(\S+) up=(\d) down=(\d)", line)
        if m and current:
            shells[current][1] += int(m.group(3))
            shells[current][2] += int(m.group(4))
            shells[current][3].append(float(m.group(2)))
    return shells


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="aufbau-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x768x24"
    failures = []
    keep = os.environ.get("ECCE_AUFBAU_PNG")

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    urls = installLocal(data, {name: os.path.join(ATOMS, "ecceqm-" + name,
                                                  "stored", name)
                               for name in CASES})
    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            for name, (config, arrows) in CASES.items():
                if os.environ.get("ECCE_AUFBAU_ONLY") not in (None, name):
                    continue
                out = os.path.join(work, "run-" + name)
                scene = "hold 3\nmodiagram B aufbaupng 0\nmodiagram A aufbaupng 1\n"
                if name == "O":
                    scene += "modiagram click click 3\n"
                result = base.runBuilder(display, urls[name], out, scene,
                                         panel="MO Diagram")
                failed = os.path.join(out, "FAILED")
                check(not result.crashed, "%s: the Builder did not crash" % name)
                check(not os.path.exists(failed), "%s: the scene ran to the end %s" % (
                    name, open(failed).read() if os.path.exists(failed) else ""))
                for view in ("A", "B"):
                    path = os.path.join(out, view + ".txt")
                    text = open(path).read() if os.path.exists(path) else ""
                    check(text.startswith("config " + config + "\n"),
                          "%s %s: configuration %s" % (name, view, config))
                    shells = parse(text)
                    for label, (up, down) in arrows.items():
                        got = shells.get(label)
                        check(got is not None and (got[1], got[2]) == (up, down),
                              "%s %s: %s has %d up and %d down arrows (%s)" % (
                                  name, view, label, up, down,
                                  None if got is None else got[1:3]))
                    check(any(re.search(r"shell %s .*electrons=0" % re.escape(label), text)
                              for label in shells),
                          "%s %s: an empty subshell is drawn" % (name, view))
                    png = os.path.join(out, view + ".png")
                    check(os.path.exists(png) and os.path.getsize(png) > 2000,
                          "%s %s: the picture is written" % (name, view))
                    if keep and os.path.exists(path):
                        os.makedirs(keep, exist_ok=True)
                        shutil.copy(path, os.path.join(keep, "%s-%s.txt" % (name, view)))
                    if keep and os.path.exists(png):
                        os.makedirs(keep, exist_ok=True)
                        shutil.copy(png, os.path.join(keep, "%s-%s.png" % (name, view)))
                shells = parse(open(os.path.join(out, "A.txt")).read())
                valence = {"O": "2p", "Ne": "2p", "Ar": "3p"}[name]
                check(shells.get("1s", [0])[0] == 1 and shells.get(valence, [1])[0] == 0,
                      "%s: 1s is core, %s is not" % (name, valence))
                if name == "O":
                    ev = sorted(shells["2p"][3])
                    check(ev[0] < ev[1] - 1.0 or ev[1] < ev[2] - 1.0,
                          "O: the ROKS 2p orbitals are split in energy (%s)" % ev)
                    click = os.path.join(out, "click.txt")
                    check(os.path.exists(click) and "grid yes" in open(click).read(),
                          "O: a click on an orbital computes it")
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
            if not os.environ.get("ECCE_AUFBAU_KEEP"):
                shutil.rmtree(work, ignore_errors=True)
            else:
                print("kept " + work)
    if failures:
        print("%d failed" % len(failures))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
