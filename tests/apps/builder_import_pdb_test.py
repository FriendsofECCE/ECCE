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
    "wait 2000", "import " + STRUCTURE, "wait 1000", "resatoms", "info",
    "select_ligand", "selection", "menu Edit|Clear Selection", "ligands",
    "ligand 2", "menu Edit|Clear Selection",
    "menu Build|Add Hydrogen", "wait 3000", "resatoms", "info",
    "menu Build|Add Hydrogen", "wait 1000", "info",
    "quit",
]


#  Heavy atoms only, no residues: benzene and ethanol, 8 A apart (#1251).
HEAVY_XYZ = """9
benzene and ethanol, heavy atoms
C 1.400 0.000 0.000
C 0.700 1.212 0.000
C -0.700 1.212 0.000
C -1.400 0.000 0.000
C -0.700 -1.212 0.000
C 0.700 -1.212 0.000
C 8.000 0.000 0.000
C 9.520 0.000 0.000
O 9.998 1.349 0.000
"""
SCRIPT_HEAVY = ["wait 2000", "import @XYZ@", "wait 1000", "info",
                "menu Build|Add Hydrogen", "wait 2000", "coords", "info",
                "menu Build|Add Hydrogen", "wait 1000", "info", "quit"]
#  A drawn atom with nubs next to a protein that has none (#1251).
SCRIPT_MIXED = ["wait 2000", "import " + STRUCTURE, "wait 1000",
                "add C Tetrahedral 40 40 40", "wait 500", "info",
                "menu Build|Add Hydrogen", "wait 5000", "info", "quit"]


def run(display, script=None, name="builder-import-pdb"):
    work = os.path.join(fixture.stateHome(), name)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    path = os.path.join(work, "import.script")
    xyz = os.path.join(work, "heavy.xyz")
    with open(xyz, "w") as handle:
        handle.write(HEAVY_XYZ)
    with open(path, "w") as handle:
        handle.write("\n".join(script or SCRIPT).replace("@XYZ@", xyz) + "\n")
    try:
        #  A private state directory has no saved login: without -pipe the
        #  builder stops at the "ECCE Authentication" dialog.
        auth = fixture.authFile(os.path.join(work, "auth.pipe"),
                                user=fixture.realUser())
        result = apps.run(display, "builder", args=("-pipe", auth),
                          windowTimeout=60, settle=90,
                          env={"ECCE_BUILDER_SCRIPT": path,
                               "ECCE_FEEDBACK_STDERR": "1",
                               "ECCE_TEST_XYZ_UNITS": "angstrom"})
        if "BUILDER: atoms=" not in (result.log or ""):
            print("builder: windows=%s rc=%s crashed=%s note=%s"
                  % (result.windows, result.returncode, result.crashed,
                     result.note))
            print("\n".join("      | " + l for l in
                            (result.log or "").splitlines()[-25:]))
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
    if len(counts) == 2:
        second = dict(kv.split("=") for kv in counts[1].split(","))
        if second.get("BNZ") != "12":
            problems.append("BNZ atoms after Add Hydrogen: %s, want 12 (6 C + 6 H)"
                            % second.get("BNZ"))
    sel = re.findall(r"BUILDER: selection atoms=(\d+)", log)
    ligs = re.findall(r"BUILDER: ligands (.*)", log)
    if not ligs or not ligs[0].startswith("BNZ 400 (6 atoms)") or "HED" not in ligs[0] \
            or ligs[0].index("HED") > ligs[0].index("CL"):
        problems.append("ligand menu %s: want BNZ first, additive HED before ions" % ligs)
    if len(sel) < 2 or sel[1] != "8":
        problems.append("ligand 2 selected %s atoms, want 8 (HED)" % sel[1:])
    sel = sel[:1]
    if sel != ["6"]:
        problems.append("Select Ligand selected %s atoms, want [6] (BNZ, not HED)" % sel)
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


def heavyProblems(log):
    problems = []
    for line in log.splitlines():
        if "ECCE_BUILDER_SCRIPT" in line and "FAIL" in line:
            problems.append(line.strip())
    atoms = [int(m) for m in re.findall(r"BUILDER: atoms=(\d+)", log)]
    if atoms != [9, 21, 21]:
        problems.append("atom counts %s, want [9, 21, 21] (+6 +6 H, then no change)" % atoms)
    heavy, hyd = [], []
    for m in re.finditer(r"BUILDER: atom \d+ (\S+) (\S+) (\S+) (\S+)", log):
        (hyd if m.group(1) == "H" else heavy).append(
            (m.group(1), float(m.group(2)), float(m.group(3)), float(m.group(4))))
    ring = [h for h in hyd if h[1] < 4]
    if len(hyd) != 12 or len(ring) != 6:
        problems.append("%d H, %d near the ring, want 12 and 6" % (len(hyd), len(ring)))
    for h in hyd:
        d, atom = min((sum((a - b) ** 2 for a, b in zip(h[1:], x[1:])) ** 0.5, x)
                      for x in heavy)
        if atom[0] == "C" and not 1.08 <= d <= 1.10:
            problems.append("C-H %.3f A, want 1.08-1.10" % d)
        if h[1] < 4 and abs(h[3]) > 0.02:
            problems.append("ring H out of plane: z=%.3f" % h[3])
    print("%s  heavy-atom XYZ: atoms %s" % ("FAIL" if problems else "PASS", atoms))
    return problems


def mixedProblems(log):
    problems = []
    for line in log.splitlines():
        if "ECCE_BUILDER_SCRIPT" in line and "FAIL" in line:
            problems.append(line.strip())
    atoms = [int(m) for m in re.findall(r"BUILDER: atoms=(\d+)", log)]
    nubs = [int(m) for m in re.findall(r"nubs=(\d+)", log)]
    if len(atoms) != 2 or len(nubs) != 2:
        problems.append("atoms %s nubs %s, want two of each" % (atoms, nubs))
    else:
        if nubs[0] != 4 or nubs[1] != 0:
            problems.append("nubs before/after %s, want [4, 0]" % nubs)
        #  protein ~1314 H + 4 for the drawn atom, minus its 4 nubs
        if not 1250 <= atoms[1] - atoms[0] <= 1500:
            problems.append("atoms %s: protein did not get its hydrogens" % atoms)
    print("%s  181L + drawn atom with nubs: atoms %s nubs %s" %
          ("FAIL" if problems else "PASS", atoms, nubs))
    return problems


def check(display, results):
    results.checks += 1
    problems = problemsIn(run(display))
    if problems:
        results.fail("builder-import-pdb", "; ".join(problems))
    results.checks += 1
    problems = heavyProblems(run(display, SCRIPT_HEAVY, "builder-heavy"))
    if problems:
        results.fail("builder-heavy-xyz", "; ".join(problems))
    results.checks += 1
    problems = mixedProblems(run(display, SCRIPT_MIXED, "builder-mixed"))
    if problems:
        results.fail("builder-mixed-hydrogen", "; ".join(problems))


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
                problems += heavyProblems(run(display, SCRIPT_HEAVY, "builder-heavy"))
                problems += mixedProblems(run(display, SCRIPT_MIXED, "builder-mixed"))
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
