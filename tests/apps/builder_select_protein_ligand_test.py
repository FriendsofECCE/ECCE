#!/usr/bin/env python3
"""Builder Edit > Select Protein / Select Ligand, menu and script hooks (#967)."""
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402

STRUCTURE = os.path.join(HERE, "fixtures", "select-protein-ligand.pdb")
#  ALA-GLY (9 atoms), ligand LIG (6), smaller LG2 (3), glycerol (additive),
#  two waters and a sodium ion.
SCRIPT = [
    #  A known start style: an earlier test in the suite may have saved
    #  Wireframe as the default, which would make the Wireframe check below
    #  pass without the ligand selection doing anything.
    "wait 2000", "expect atoms 27", "menu Render|Ball And Stick", "atomstyles",
    "select_protein", "selection",
    "menu Edit|Clear Selection", "select_ligand", "selection",
    "menu Edit|Clear Selection", "menu Edit|Select Protein", "selection",
    "menu Edit|Clear Selection", "menu Edit|Select Ligand", "selection",
    #  Replace, not add: ligand then protein is the protein alone.
    "menu Edit|Clear Selection", "select_ligand", "selection",
    "select_protein", "selection",
    "menu Edit|Clear Selection", "select_ligand", "menu Render|Wireframe",
    "atomstyles",
    "quit",
]


def run(display):
    work = os.path.join(fixture.stateHome(), "builder-select-protein-ligand")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    path = os.path.join(work, "select.script")
    with open(path, "w") as handle:
        handle.write("\n".join(SCRIPT) + "\n")
    try:
        #  A private state directory has no saved login: without -pipe the
        #  builder stops at the "ECCE Authentication" dialog.
        auth = fixture.authFile(os.path.join(work, "auth.pipe"),
                                user=fixture.realUser())
        result = apps.run(display, "builder", args=("-pipe", auth, STRUCTURE),
                          windowTimeout=60, settle=90,
                          env={"ECCE_BUILDER_SCRIPT": path,
                               "ECCE_FEEDBACK_STDERR": "1"})
        if "BUILDER:" not in (result.log or ""):
            print("builder: windows=%s rc=%s crashed=%s note=%s"
                  % (result.windows, result.returncode, result.crashed,
                     result.note))
        return result.log or ""
    finally:
        shutil.rmtree(work, ignore_errors=True)


def problemsIn(log):
    problems = []
    for line in log.splitlines():
        if "ECCE_BUILDER_SCRIPT" in line and "FAIL" in line:
            problems.append(line.strip())
    counts = [int(m) for m in re.findall(r"BUILDER: selection atoms=(\d+)", log)]
    expected = [9, 6, 9, 6, 6, 9]
    if counts != expected:
        problems.append("selected atom counts %s, want %s" % (counts, expected))
    styles = [[(int(a), int(b), s) for a, b, s in
               re.findall(r"(\d+)-(\d+) (\S+)", m)]
              for m in re.findall(r"BUILDER: atomstyles(.*)", log)]
    if len(styles) != 2:
        problems.append("%d style dumps, want 2" % len(styles))
    else:
        def styleOf(runs, i):
            return next((s for a, b, s in runs if a <= i <= b), None)
        for i in range(27):
            want = "Wireframe" if 9 <= i <= 14 else styleOf(styles[0], i)
            if i in (0, 9) and styleOf(styles[0], i).endswith("Wireframe"):
                problems.append("fixture already Wireframe: weak test")
            if not styleOf(styles[1], i) or \
                    styleOf(styles[1], i).split(":")[-1] != \
                    want.split(":")[-1]:
                problems.append("atom %d style %s, want %s"
                                % (i, styleOf(styles[1], i), want))
                break
    if "Selected ligand LIG 101 (6 atoms)" not in log or "1 other" not in log:
        problems.append("ligand message does not name LIG and the other residue")
    print("%s  select protein/ligand: atoms and message: %s" %
          ("FAIL" if problems else "PASS", counts))
    return problems


def check(display, results):
    results.checks += 1
    problems = problemsIn(run(display))
    if problems:
        results.fail("builder-select-protein-ligand", "; ".join(problems))


def main():
    import isolate
    import xdisplay

    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"

    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        try:
            if not all(apps.serviceState(display).values()):
                print("FAIL  services did not come up: %s" % log)
                return 1
            fixture.ensureRealUserAccount()
            restore = fixture.settleUpgradeNotices()
            try:
                out = run(display)
                problems = problemsIn(out)
            finally:
                restore()
        finally:
            apps.stopServices(display)
    if problems:
        print("\n".join("      | " + l for l in out.splitlines()[-25:]))
        print("FAIL  builder select within")
        return 1
    print("PASS  builder select within")
    return 0


if __name__ == "__main__":
    sys.exit(main())
