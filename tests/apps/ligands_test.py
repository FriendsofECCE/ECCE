#!/usr/bin/env python3
"""Structure Library > Ligands attach to a metal with an open site (headless).

    tests/apps/ligands_test.py                own Xvfb, services, local data

Each .mvm under data/client/StructureLibrary/Ligands must carry a Nub on
every donor atom (the file is read here, not by the Builder).  The Builder's
ECCE_BUILDER_SCRIPT then adds an octahedral Fe, picks its first nub, picks
the ligand's first nub in Add Structure mode and clicks: the Fe must end up
bonded to the donor element, and to nothing else of the ligand.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
LIGANDS = os.path.join(ROOT, "data", "client", "StructureLibrary", "Ligands")

# ligand -> (donor element, number of nubs)
EXPECT = {
    "PPh3": ("P", 1), "PMe3": ("P", 1), "CO": ("C", 1), "CN": ("C", 1),
    "NH3": ("N", 1), "H2O": ("O", 1), "Pyridine": ("N", 1),
    "Bipyridine": ("N", 2), "Phenanthroline": ("N", 2),
    "Ethylenediamine": ("N", 2), "Acetylacetonate": ("O", 2),
    "Oxalate": ("O", 2),
}
# pi ligands carry no nubs (bonded with the Metal order, builder_metal_test)
PI = ("Cyclopentadienyl",)

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-12:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def readMvm(name):
    """(symbols, [(a, b)] bonds, 1-based) of a library file."""
    text = open(os.path.join(LIGANDS, name + ".mvm")).read().splitlines()
    i = text.index("atom_list:") + 1
    n = int(re.search(r"num_atoms:\s*(\d+)", "\n".join(text)).group(1))
    symbols = [text[i + k].split()[0] for k in range(n)]
    j = text.index("bond_list:") + 1
    bonds = [tuple(int(x) for x in l.split()[:2]) for l in text[j:] if l.strip()]
    return symbols, bonds


def nubFile(name):
    """Static check of the file: nubs, each bonded once, to the donor."""
    symbols, bonds = readMvm(name)
    donor, count = EXPECT[name]
    nubs = [k + 1 for k, s in enumerate(symbols) if s == "Nub"]
    if len(nubs) != count:
        return None, "%d nubs, expected %d" % (len(nubs), count)
    for n in nubs:
        parents = [a if b == n else b for a, b in bonds if n in (a, b)]
        if len(parents) != 1 or symbols[parents[0] - 1] != donor:
            return None, "nub %d is not bonded once to a %s" % (n, donor)
    return nubs[0] - 1, ""


def script(path, jobs):
    """One Builder session: a fresh Fe for every ligand, the Fe's first nub
    and the ligand's first nub picked, then a click."""
    lines = ["wait 3000"]
    for i, (name, nubIndex, donor) in enumerate(jobs):
        lines += ["select -1", "add Fe Octahedral %d 0 0" % (8 * i), "selectnub Fe",
                  "library Ligands " + name, "libatom %d" % nubIndex, "click",
                  "bonds Fe " + donor, "bonds Fe Nub"]
    lines += ["info", "quit"]
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def attach(display, work, calcUrl, jobs):
    path = os.path.join(work, "ligands.script")
    script(path, jobs)
    result = apps.run(display, "builder", args=("-context", calcUrl),
                      windowTimeout=60, settle=120,
                      env={"ECCE_BUILDER_SCRIPT": path})
    log = result.log or ""
    if result.crashed:
        return fail("builder", "CRASHED (%s)" % result.signalName, log)
    bad = [l for l in log.splitlines()
           if re.search(r"ECCE_BUILDER_SCRIPT: .*: (?!ok)\S", l)]
    if bad:
        fail("script", "step failed: %s" % bad[0], log)
    counts = re.findall(r"BUILDER: bonds Fe-(\w+)=(\d+)", log)
    if len(counts) != 2 * len(jobs):
        return fail("script", "%d bond counts for %d ligands"
                    % (len(counts), len(jobs)), log)
    for i, (name, nubIndex, donor) in enumerate(jobs):
        (d, bonded), (n, nubs) = counts[2 * i], counts[2 * i + 1]
        if d != donor or int(bonded) != 1:
            fail(name, "Fe bonded to %s %s times, expected 1" % (donor, bonded))
        elif int(nubs) != 5:
            fail(name, "Fe holds %s open nubs, expected 5" % nubs)
        else:
            print("ok    %s: Fe-%s bond made, 5 open sites left"
                  % (name, donor), flush=True)


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    nubIndex = {}
    for name in (sys.argv[1:] or EXPECT):
        if not os.path.exists(os.path.join(LIGANDS, name + ".mvm")):
            fail(name, "no such file in the Ligands library")
            continue
        idx, why = nubFile(name)
        if idx is None:
            fail(name, why)
        else:
            nubIndex[name] = idx
    for name in PI:
        if "Nub" in readMvm(name)[0]:
            fail(name, "a pi ligand has nubs")
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings), flush=True)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    os.environ.pop("DISPLAY", None)
    work = tempfile.mkdtemp(prefix="ligands-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            commands = os.path.join(work, "commands")
            with open(commands, "w") as handle:
                handle.write("newproject ligs\nnewcalc ligs c NWChem\n")
            root = os.path.join(data, "users", "local")
            apps.run(display, "organizer", windowTimeout=60, settle=5,
                     env={"ECCE_TEST_ORGANIZER": commands},
                     inspect=lambda d: time.sleep(5))
            calc = os.path.join(root, "ligs", "c")
            if not os.path.isdir(calc):
                fail("setup", "no calculation made under " + root)
            else:
                attach(display, work, "file://" + calc,
                       [(n, nubIndex[n], EXPECT[n][0]) for n in nubIndex])
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
