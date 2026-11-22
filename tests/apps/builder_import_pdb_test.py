#!/usr/bin/env python3
"""Builder: File > Import of a PDB keeps its ligand and waters; Build > Add
Hydrogen on a structure without nubs adds residue hydrogens and says what
it could not do (#1244)."""
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
    "wait 2000", "import " + STRUCTURE, "wait 1000", "resatoms", "info", "bondorders",
    "menu Build|Add Hydrogen", "wait 3000", "resatoms", "info",
    "menu Build|Add Hydrogen", "wait 1000", "info",
    "quit",
]


def run(display):
    work = os.path.join(fixture.stateHome(), "builder-import-pdb")
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
    problems = []
    for line in log.splitlines():
        if "ECCE_BUILDER_SCRIPT" in line and "FAIL" in line:
            problems.append(line.strip())
    counts = re.findall(r"BUILDER: resatoms (\S+)", log)
    if len(counts) != 2:
        problems.append("%d resatoms lines, want 2" % len(counts))
    else:
        first = dict(kv.split("=") for kv in counts[0].split(","))
        if first.get("BNZ") != "6":
            problems.append("BNZ atoms after import: %s, want 6" % first.get("BNZ"))
        if first.get("HOH") != "136":
            problems.append("HOH atoms after import: %s, want 136" % first.get("HOH"))
    atoms = [int(m) for m in re.findall(r"BUILDER: atoms=(\d+)", log)]
    if len(atoms) != 3 or atoms[1] <= atoms[0]:
        problems.append("atom counts around Add Hydrogen %s: none added" % atoms)
    elif atoms[2] != atoms[1]:
        problems.append("second Add Hydrogen changed the count %s" % atoms)
    if "Added " not in log or "residues" not in log:
        problems.append("no 'Added N hydrogens to M of K residues' message")
    #  The ligand's bonds as drawn (no CONECT records for BNZ in the file).
    for m in re.finditer(r"BUILDER: bond (\d+)-(\d+) (\S+) order=(\S+) drawn=(\d+) len=(\S+)", log):
        if int(m.group(1)) >= 1290 and int(m.group(2)) >= 1290:
            print("      bond %s-%s %s order=%s drawn=%s len=%s" % m.groups())
    print("%s  import 181L: %s atoms %s" %
          ("FAIL" if problems else "PASS", counts[:1], atoms))
    return problems


def check(display, results):
    results.checks += 1
    problems = problemsIn(run(display))
    if problems:
        results.fail("builder-import-pdb", "; ".join(problems))


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
