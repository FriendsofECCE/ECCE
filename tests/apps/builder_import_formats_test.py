#!/usr/bin/env python3
"""Builder: opening an SDF, MOL2, PDBQT and a SMILES file (through the Open
Babel bridge) gives the structure in the file, not an empty Builder (#1343).
Uses the ECCE_BUILDER_SCRIPT `import` command, which is the Open path of the
File menu.  The SMILES case skips (not fails) when obabel is not installed."""
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402

REAL = os.path.join(HERE, "..", "dock", "formats", "real")
ASPIRIN_ATOMS = 21


def pdbqtAtoms(path):
    with open(path) as handle:
        return sum(1 for l in handle if l.startswith(("ATOM", "HETATM")))


def cases():
    return [
        ("aspirin.sdf", os.path.join(REAL, "aspirin_pubchem3d.sdf"), ASPIRIN_ATOMS),
        ("aspirin.mol2", os.path.join(REAL, "aspirin_pubchem3d.mol2"), ASPIRIN_ATOMS),
        ("aspirin.pdbqt", os.path.join(REAL, "aspirin_pubchem3d_ecce.pdbqt"),
         pdbqtAtoms(os.path.join(REAL, "aspirin_pubchem3d_ecce.pdbqt"))),
    ]


def runOne(display, name, src, tmp):
    path = os.path.join(tmp, name)
    if src is None:
        with open(path, "w") as handle:
            handle.write("c1ccccc1O phenol\n")
    else:
        shutil.copy(src, path)
    script = os.path.join(tmp, name + ".script")
    with open(script, "w") as handle:
        handle.write("wait 2000\nimport %s\nwait 1500\ninfo\nquit\n" % path)
    auth = fixture.authFile(os.path.join(tmp, name + ".pipe"), user=fixture.realUser())
    result = apps.run(display, "builder", args=("-pipe", auth),
                      windowTimeout=60, settle=60,
                      env={"ECCE_BUILDER_SCRIPT": script,
                           "ECCE_FEEDBACK_STDERR": "1"})
    log = result.log or ""
    m = re.findall(r"BUILDER: atoms=(\d+)", log)
    return int(m[-1]) if m else None, log


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

    todo = cases()
    if shutil.which("obabel") or os.environ.get("ECCE_OBABEL"):
        todo.append(("phenol.smi", None, 13))
    else:
        print("note  obabel not installed: .smi case not run")
    problems = []
    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        try:
            if not all(apps.serviceState(display).values()):
                print("FAIL  services did not come up: %s" % log)
                return 1
            fixture.ensureRealUserAccount()
            restore = fixture.settleUpgradeNotices()
            tmp = os.path.join(fixture.stateHome(), "builder-formats")
            shutil.rmtree(tmp, ignore_errors=True)
            os.makedirs(tmp)
            try:
                for name, src, want in todo:
                    atoms, out = runOne(display, name, src, tmp)
                    ok = atoms == want and atoms
                    print("%s  %s: %s atoms, want %d" %
                          ("PASS" if ok else "FAIL", name, atoms, want))
                    if not ok:
                        problems.append("%s: %s atoms, want %d" % (name, atoms, want))
                        print("\n".join("      | " + l for l in out.splitlines()[-15:]))
            finally:
                restore()
                shutil.rmtree(tmp, ignore_errors=True)
        finally:
            apps.stopServices(display)
    if problems:
        print("FAIL  builder import formats: %s" % "; ".join(problems))
        return 1
    print("PASS  builder import formats")
    return 0


if __name__ == "__main__":
    sys.exit(main())
