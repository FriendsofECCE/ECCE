#!/usr/bin/env python3
"""Builder on a PDB opened as its file argument, as the file manager's Open
With does: the Build menu is enabled, Add Hydrogen adds atoms, deleting the
solvent undoes and redoes, and File > Save leaves the original file alone
(#1259, #1248).  181L has HEADER/REMARK records, which ECCE cannot write."""
import hashlib
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
    "wait 2500", "info", "resatoms",
    "menuenabled Build|Add Hydrogen",
    "menu Edit|Select Solvent", "wait 500",
    "menu Edit|Delete Selection", "wait 500", "info", "resatoms",
    "undo", "wait 500", "info", "resatoms",
    "redo", "wait 500", "info",
    "ctrlz", "wait 500", "info",
    "menu Build|Add Hydrogen", "wait 3000", "info",
    "savemenu", "wait 500", "savestate",
    "quit",
]


def digest(path):
    with open(path, "rb") as handle:
        return hashlib.sha256(handle.read()).hexdigest()


def run(display):
    """The Builder log and whether the opened copy kept its bytes and mtime."""
    work = os.path.join(fixture.stateHome(), "builder-pdb-open")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    pdb = os.path.join(work, "181L.pdb")
    shutil.copy2(STRUCTURE, pdb)
    before = (digest(pdb), os.stat(pdb).st_mtime_ns)
    path = os.path.join(work, "open.script")
    with open(path, "w") as handle:
        handle.write("\n".join(SCRIPT) + "\n")
    try:
        auth = fixture.authFile(os.path.join(work, "auth.pipe"),
                                user=fixture.realUser())
        #  Quit asks about the unsaved edits; the dialog hook closes it
        #  without saving.
        result = apps.run(display, "builder", args=("-pipe", auth, pdb),
                          windowTimeout=60, settle=90,
                          env={"ECCE_BUILDER_SCRIPT": path,
                               "ECCE_FEEDBACK_STDERR": "1",
                               "ECCE_TEST_DIALOG_CLOSE": "1"})
        if "BUILDER:" not in (result.log or ""):
            print("builder: windows=%s rc=%s crashed=%s note=%s"
                  % (result.windows, result.returncode, result.crashed,
                     result.note))
        kept = (digest(pdb), os.stat(pdb).st_mtime_ns) == before
        return result.log or "", kept
    finally:
        shutil.rmtree(work, ignore_errors=True)


def problemsIn(log, kept):
    problems = []
    for line in log.splitlines():
        if "ECCE_BUILDER_SCRIPT" in line and "FAIL" in line:
            problems.append(line.strip())
    if "cannot write back" not in log:
        problems.append("no note that Save asks for a new name")
    m = re.search(r"BUILDER: menuenabled Build\|Add Hydrogen top=(\d) item=(\d)", log)
    if not m or m.groups() != ("1", "1"):
        problems.append("Build > Add Hydrogen not enabled: %s"
                        % (m.group(0) if m else "no line"))
    atoms = [int(n) for n in re.findall(r"BUILDER: atoms=(\d+)", log)]
    res = re.findall(r"BUILDER: resatoms (\S+)", log)
    if len(atoms) != 6 or len(res) != 3:
        problems.append("atoms %s resatoms %s: want 6 and 3 lines" % (atoms, res))
    else:
        if not atoms[1] < atoms[0] or "HOH=" in res[1]:
            problems.append("Delete of the solvent: atoms %s, %s" % (atoms[:2], res[1]))
        if atoms[2] != atoms[0] or res[2] != res[0]:
            problems.append("Undo of the solvent delete: atoms %s residues %s -> %s"
                            % (atoms[:3], res[0], res[2]))
        if atoms[3] != atoms[1]:
            problems.append("Redo of the solvent delete: atoms %s" % atoms[:4])
        if atoms[4] != atoms[0]:
            problems.append("Ctrl+Z after redo: atoms %s" % atoms)
        if not atoms[5] > atoms[4]:
            problems.append("Add Hydrogen added nothing: atoms %s" % atoms[4:])
    if "BUILDER: save goes to Save As" not in log:
        problems.append("File > Save did not go to Save As")
    if not kept:
        problems.append("the opened PDB file was changed")
    print("%s  181L opened as a file: atoms %s, original kept %s" %
          ("FAIL" if problems else "PASS", atoms, kept))
    return problems


def check(display, results):
    results.checks += 1
    problems = problemsIn(*run(display))
    if problems:
        results.fail("builder-pdb-open", "; ".join(problems))


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
                out, kept = run(display)
                problems = problemsIn(out, kept)
            finally:
                restore()
        finally:
            apps.stopServices(display)
    if problems:
        print("\n".join("      | " + l for l in out.splitlines()[-30:]))
        #  Last, so the reasons survive when only the tail is kept.
        print("\n".join("      problem: " + p for p in problems))
        print("FAIL  builder pdb open")
        return 1
    print("PASS  builder pdb open")
    return 0


if __name__ == "__main__":
    sys.exit(main())
