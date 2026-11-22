#!/usr/bin/env python3
"""Builder: Select Solvent, Delete and Undo bring the water back; Ctrl+Z
undoes; the 3-D view has the focus after open and Select (#1247, #1248, #1240)."""
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402

STRUCTURE = os.path.join(HERE, "..", "fragreaders", "data", "181L.pdb")
SCRIPT = [
    "wait 2000", "import " + STRUCTURE, "wait 1000", "focus", "info", "resatoms",
    "menu Edit|Select Solvent", "wait 500", "focus", "selection",
    "menu Edit|Delete Selection", "wait 500", "info", "resatoms",
    "undo", "wait 500", "info", "resatoms",
    "menu Edit|Select Protein", "menu Edit|Invert Selection", "wait 500", "selection",
    "menu Edit|Delete Selection", "wait 500", "info",
    "ctrlz", "wait 500", "info",
    "quit",
]


def run(display):
    work = os.path.join(fixture.stateHome(), "builder-delete-undo")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    path = os.path.join(work, "import.script")
    with open(path, "w") as handle:
        handle.write("\n".join(SCRIPT) + "\n")
    try:
        #  A private state directory has no saved login: without -pipe the
        #  builder stops at the "ECCE Authentication" dialog.
        auth = fixture.authFile(os.path.join(work, "auth.pipe"),
                                user=fixture.realUser())
        result = apps.run(display, "builder", args=("-pipe", auth),
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
    """Select Solvent + Delete + Undo restores the water (#1248); the 3-D
    view has the focus after open and Select (#1240); the Ctrl+Z key hook
    undoes (#1247)."""
    problems = []
    for line in log.splitlines():
        if "ECCE_BUILDER_SCRIPT" in line and "FAIL" in line:
            problems.append(line.strip())
    atoms = [int(m) for m in re.findall(r"BUILDER: atoms=(\d+)", log)]
    res = re.findall(r"BUILDER: resatoms (\S+)", log)
    if len(atoms) != 5 or len(res) != 3:
        problems.append("atoms %s resatoms %s: want 5 and 3 lines" % (atoms, res))
        return problems
    if not atoms[1] < atoms[0]:
        problems.append("Delete of the solvent removed nothing %s" % atoms)
    if atoms[2] != atoms[0] or res[2] != res[0]:
        problems.append("Undo of the solvent delete: atoms %s residues %s -> %s"
                        % (atoms[:3], res[0], res[2]))
    if atoms[3] >= atoms[2] or atoms[4] != atoms[2]:
        problems.append("invert delete / Ctrl+Z hook: atoms %s" % atoms[2:])
    focus = re.findall(r"BUILDER: focus (\S+)", log)
    if len(focus) != 2 or any("RenderArea" not in f for f in focus):
        problems.append("focus after open / Select Solvent: %s" % focus)
    print("%s  solvent delete/undo: atoms %s" %
          ("FAIL" if problems else "PASS", atoms))
    return problems


def check(display, results):
    results.checks += 1
    problems = problemsIn(run(display))
    if problems:
        results.fail("builder-delete-undo", "; ".join(problems))


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
        print("FAIL  builder delete undo")
        return 1
    print("PASS  builder delete undo")
    return 0


if __name__ == "__main__":
    sys.exit(main())
