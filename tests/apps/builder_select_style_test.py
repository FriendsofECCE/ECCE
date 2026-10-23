#!/usr/bin/env python3
"""Builder: Select Solvent / Ions / Molecule and a style on a selection (#479).

    tests/apps/builder_select_style_test.py     own Xvfb and services

Headless, through ECCE_BUILDER_SCRIPT on a GROMACS .gro (a four-residue
peptide, 41 atoms, then 48 SOL waters, then one NA and one CL), with the
Edit and Render menu items chosen as a click sends them:

  - Select Solvent selects the 144 water atoms, Select Ions the 2 ions;
  - Select Molecule selects the peptide with nothing selected, and the one
    water whose atom is selected;
  - Wireframe on the selected solvent changes only the waters; the peptide
    and the ions keep their style.
"""
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402

STRUCTURE = os.path.join(HERE, "fixtures", "solvated-peptide.gro")
PEPTIDE, WATER, IONS = (0, 40), (41, 184), (185, 186)

SCRIPT = [
    "wait 2000",
    "atomstyles",
    "menu Edit|Select Solvent", "selection",
    "menu Edit|Clear Selection", "menu Edit|Select Ions", "selection",
    "menu Edit|Clear Selection", "menu Edit|Select Molecule", "selection",
    "menu Edit|Clear Selection", "select 100", "menu Edit|Select Molecule",
    "selection",
    "menu Edit|Clear Selection", "menu Edit|Select Solvent",
    "menu Render|Wireframe", "atomstyles",
    "quit",
]


def run(display):
    work = os.path.join(fixture.stateHome(), "builder-select-style")
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


def styleOf(runs, index):
    for first, last, style in runs:
        if first <= index <= last:
            return style.split(":")[-1]
    return None


def problemsIn(log):
    problems = []

    def expect(ok, what):
        print("%s  builder select/style: %s" % ("ok  " if ok else "FAIL", what))
        if not ok:
            problems.append(what)

    for line in log.splitlines():
        if "ECCE_BUILDER_SCRIPT" in line and "FAIL" in line:
            expect(False, line.strip())
    counts = [int(m) for m in re.findall(r"BUILDER: selection atoms=(\d+)", log)]
    styles = [[(int(a), int(b), s) for a, b, s in
               re.findall(r"(\d+)-(\d+) (\S+)", m)]
              for m in re.findall(r"BUILDER: atomstyles(.*)", log)]
    expect(len(counts) == 4 and len(styles) == 2,
           "every step reported (%d selections, %d style dumps)"
           % (len(counts), len(styles)))
    if len(counts) == 4:
        expect(counts[0] == 144, "Select Solvent: %d atoms, want 144" % counts[0])
        expect(counts[1] == 2, "Select Ions: %d atoms, want 2" % counts[1])
        expect(counts[2] == 41,
               "Select Molecule, nothing selected: %d atoms, want the 41 of "
               "the peptide" % counts[2])
        expect(counts[3] == 3,
               "Select Molecule on a water atom: %d atoms, want 3" % counts[3])
    if len(styles) == 2:
        before, after = styles
        bad = []
        for i in range(IONS[1] + 1):
            want = "Wireframe" if WATER[0] <= i <= WATER[1] \
                else styleOf(before, i)
            if styleOf(after, i) != want:
                bad.append("%d %s (want %s)" % (i, styleOf(after, i), want))
        expect(not bad and styleOf(before, 0) != "Wireframe",
               "Wireframe on the solvent: only the waters change (%s)"
               % ("; ".join(bad[:4]) or "ok"))
    return problems


def check(display, results):
    results.checks += 1
    problems = problemsIn(run(display))
    if problems:
        results.fail("builder-select-style", "; ".join(problems))


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
        print("FAIL  builder select/style")
        return 1
    print("PASS  builder select/style")
    return 0


if __name__ == "__main__":
    sys.exit(main())
